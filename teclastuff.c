/* $Id$ */

/* Support for tecla features (such as word completion etc.) */

#include <libtecla.h>
#include <spencer_regexp.h>

#include "cexpsyms.h"
#include "cexpmod.h"
#include "ctyps.h"

#define	 MATCH_MAX	100

/* ugly hack - this must match the definition in cexp.y */

/* if the lexer detects an unterminated string constant
 * it returns LEXERR_INCOMPLETE_STRING - offset;
 * the offset is the difference between the current position
 * ( == end of the string) and the opening quote.
 */
#define LEXERR_INCOMPLETE_STRING	(-100)

extern CexpSym _cexpSymLookupRegex();

int
cexpSymComplete(WordCompletion *cpl, void *closure, const char *line, int word_end)
{
int				rval=1;
int 			word_start;
spencer_regexp	*rc=0;
char			*pattern=0;
int				count=MATCH_MAX,i;
CexpSym			s;
CexpModule		m;
CexpParserCtx	ctx = closure;
CexpTypedValRec	dummy;
int				quote;

	cexpResetParserCtx(ctx, line);
	/* try to find an opening quote using the lexer
	 * it returns a magic error code containing the
	 * offset (with respect to the end of the line)
	 * of such an opening quote...
	 */
	while ( (quote=cexplex(&dummy, ctx)) > 0 && '\n' != quote )
		/* nothing else to do */;

	if ( quote <= LEXERR_INCOMPLETE_STRING ) {
		int			rval;
		CplFileConf	*conf = new_CplFileConf();

		/* start position = end + offset returned by cexplex() */
		cfc_file_start(conf, word_end + quote - LEXERR_INCOMPLETE_STRING);
		rval = cpl_file_completions(cpl, conf, line, word_end);
		del_CplFileConf(conf);
		return rval;
	}

	/* search start of the word */
	for (word_start=word_end; word_start>0; word_start--) {
		/* these characters should match the lexer, see cexp.y ISIDENTCHAR() */
		register int ch=(unsigned char)line[word_start-1];
		if (! (isalpha(ch) || '_'==ch || '@'==ch) )
			break;
	}
	if (word_start<word_end) {
		pattern=calloc(word_end-word_start+5,1);
		pattern[0]='^';
		strncpy(pattern+1, line+word_start, word_end-word_start);
		rc=spencer_regcomp(pattern);
		/* the lookup routine returns the last symbol found
		 * looping for 'too_many' instances. Hence, if it still
		 * finds something after too_many matches, we reject...
		 */
		_cexpSymLookupRegex(rc,&count,0,0,0);
	} else {
		count=0;
	}
	if (count<=0) {
		cpl_record_error(cpl,"Refuse to complete: too many matches");
		goto cleanup;
	}

	/* now, the real fun starts.. 
	 *
	 * NOTE the race condition: if another user just loaded 
	 * a huge module, we might still have too many matches here.
	 * However, the damage is minimal: we just don't present
	 * all the choices...
	 */
	for (s=0,m=0,count=0,i=1; count<MATCH_MAX && (s=_cexpSymLookupRegex(rc,&i,s,0,&m)); i=1,count++) {
		cpl_add_completion(	cpl, line, word_start, word_end,
							s->name + word_end-word_start,	
							CEXP_TYPE_FUNQ(s->value.type) ? "()" : "",
							CEXP_TYPE_FUNQ(s->value.type) ? "("  : "");
	}

	rval=0;

cleanup:
	free(rc);
	free(pattern);
	return rval;
}
