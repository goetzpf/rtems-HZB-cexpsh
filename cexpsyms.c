/* $Id$ */

/* generic symbol table handling */

/* Author/Copyright: Till Straumann <Till.Straumann@TU-Berlin.de>
 * License: GPL, see http://www.gnu.org for the exact terms
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <spencer_regexp.h>

#include "cexpsymsP.h"
#include "cexpmod.h"
/* NOTE: DONT EDIT 'cexp.tab.h'; it is automatically generated by 'bison' */
#include "cexp.tab.h"

#ifndef LINKER_VERSION_SEPARATOR
#define LINKER_VERSION_SEPARATOR '@'
#endif

/* compare the name of two symbols 
 * (in a not very fancy way)
 * The clue is to handle the symbol
 * versioning in an appropriate way...
 */
int
_cexp_namecomp(const void *key, const void *cmp)
{
CexpSym				sa=(CexpSym)key;
CexpSym				sb=(CexpSym)cmp;
#if	LINKER_VERSION_SEPARATOR
register const char	*k=sa->name, *c=sb->name;

	while (*k) {
		register int	rval;
		if ((rval=*k++-*c++))
			return rval; /* this also handles the case where *c=='\0' */
		
	}
	/* key string is exhausted */
	return !*c || LINKER_VERSION_SEPARATOR==*c ? 0 : -1;
#else
	return strcmp(sa->name, sb->name);
#endif
}

/* compare the 'values' of two symbols, i.e. the addresses
 * they represent.
 */
int
_cexp_addrcomp(const void *a, const void *b)
{
	CexpSym *sa=(CexpSym*)a;
	CexpSym *sb=(CexpSym*)b;
	return (*sa)->value.ptv-(*sb)->value.ptv;
}

CexpSym
cexpSymTblLookup(const char *name, CexpSymTbl t)
{
CexpSymRec key;
	key.name = name;
	return (CexpSym)bsearch((void*)&key,
				t->syms,
				t->nentries,
				sizeof(*t->syms),
				_cexp_namecomp);
}

#define USE_ELF_MEMORY

#ifdef HAVE_RCMD
#include <sys/time.h>
#include <errno.h>
/* avoid pulling in networking headers under __rtems
 * until BSP stuff is separated out from the core
 */
#if !defined(RTEMS_TODO_DONE) && defined(__rtems)
#define	AF_INET	2
extern char *inet_ntop();
extern int	socket();
extern int  select();
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#define RSH_PORT 514

static char *
handleInput(int fd, int errfd, unsigned long *psize)
{
long	n=0,size,avail;
fd_set	r,w,e;
char	errbuf[1000],*ptr,*buf;
struct  timeval timeout;

register long ntot=0,got;

	if (n<fd)		n=fd;
	if (n<errfd)	n=errfd;

	n++;

	buf=ptr=0;
	size=avail=0;

	while (fd>=0 || errfd>=0) {
		FD_ZERO(&r);
		FD_ZERO(&w);
		FD_ZERO(&e);

		timeout.tv_sec=5;
		timeout.tv_usec=0;
		if (fd>=0) 		FD_SET(fd,&r);
		if (errfd>=0)	FD_SET(errfd,&r);
		if ((got=select(n,&r,&w,&e,&timeout))<=0) {
				if (got) {
					fprintf(stderr,"rsh select() error: %s.\n",
							strerror(errno));
				} else {
					fprintf(stderr,"rsh timeout\n");
				}
				goto cleanup;
		}
		if (errfd>=0 && FD_ISSET(errfd,&r)) {
				got=read(errfd,errbuf,sizeof(errbuf));
				if (got<0) {
					fprintf(stderr,"rsh error (reading stderr): %s.\n",
							strerror(errno));
					goto cleanup;
				}
				if (got)
					write(2,errbuf,got);
				else {
					errfd=-1; 
				}
		}
		if (fd>=0 && FD_ISSET(fd,&r)) {
				if (avail < LOAD_CHUNK) {
					size+=LOAD_CHUNK; avail+=LOAD_CHUNK;
					if (!(buf=realloc(buf,size))) {
						fprintf(stderr,"out of memory\n");
						goto cleanup;
					}
					ptr = buf + (size-avail);
				}
				got=read(fd,ptr,avail);
				if (got<0) {
					fprintf(stderr,"rsh error (reading stdout): %s.\n",
							strerror(errno));
					goto cleanup;
				}
				if (got) {
					ptr+=got;
					ntot+=got;
					avail-=got;
				} else {
					fd=-1;
				}
		}
	}
	if (psize) *psize=ntot;
	return buf;
cleanup:
	free(buf);
	return 0;
}

char *
rshLoad(char *host, char *user, char *cmd)
{
char	*chpt=host,*buf=0;
int		fd,errfd;
long	ntot;
extern  int rcmd();

	fd=rcmd(&chpt,RSH_PORT,user,user,cmd,&errfd);
	if (fd<0) {
		fprintf(stderr,"rcmd: got no remote stdout descriptor\n");
		goto cleanup;
	}
	if (errfd<0) {
		fprintf(stderr,"rcmd: got no remote stderr descriptor\n");
		goto cleanup;
	}

	if (!(buf=handleInput(fd,errfd,&ntot))) {
		goto cleanup; /* error message has already been printed */
	}

	fprintf(stderr,"0x%lx (%li) bytes read\n",ntot,ntot);

cleanup:
	if (fd>=0)		close(fd);
	if (errfd>=0)	close(errfd);
	return buf;
}
#endif

/* a semi-public routine which takes a precompiled regexp.
 * The reason this is not public is that we try to keep
 * the public from having to deal with/know about the regexp
 * implementation, i.e. which variant, which headers etc.
 */
CexpSym
_cexpSymTblLookupRegex(spencer_regexp *rc, int max, CexpSym s, FILE *f, CexpSymTbl t)
{
CexpSym	found=0;

	if (max<1)	max=25;
	if (!f)		f=stdout;
	if (!s)		s=t->syms;

	while (s->name && max) {
		if (spencer_regexec(rc,s->name)) {
			cexpSymPrintInfo(s,f);
			max--;
			found=s;
		}
		s++;
	}

	return s->name ? found : 0;
}

CexpSym
cexpSymTblLookupRegex(char *re, int max, CexpSym s, FILE *f, CexpSymTbl t)
{
CexpSym			found;
spencer_regexp	*rc;

	if (!(rc=spencer_regcomp(re))) {
		fprintf(stderr,"unable to compile regexp '%s'\n",re);
		return 0;
	}
	found=_cexpSymTblLookupRegex(rc,max,s,f,t);

	if (rc) free(rc);
	return found;
}

CexpSymTbl
cexpCreateSymTbl(void *syms, int symSize, int nsyms, CexpSymFilterProc filter, CexpSymAssignProc assign, void *closure)
{
char		*sp,*dst;
const char	*symname;
CexpSymTbl	rval;
CexpSym		cesp;
int			n,nDstSyms,nDstChars;

	if (!(rval=(CexpSymTbl)malloc(sizeof(*rval))))
			return 0;

	memset(rval,0,sizeof(*rval));
	
	/* count the number of valid symbols */
	for (sp=syms,n=0,nDstSyms=0,nDstChars=0; n<nsyms; sp+=symSize,n++) {
			if ((symname=filter(sp,closure))) {
				nDstChars+=strlen(symname)+1;
				nDstSyms++;
			}
	}

	rval->nentries=nDstSyms;

	/* create our copy of the symbol table - the object format contains
	 * many things we're not interested in and also, it's not
	 * sorted...
	 */
		
	/* allocate all the table space */
	if (!(rval->syms=(CexpSym)malloc(sizeof(CexpSymRec)*(nDstSyms+1))))
		goto cleanup;

	if (!(rval->strtbl=(char*)malloc(nDstChars)) ||
        !(rval->aindex=(CexpSym*)malloc(nDstSyms*sizeof(*rval->aindex))))
		goto cleanup;

	/* now copy the relevant stuff */
	for (sp=syms,n=0,cesp=rval->syms,dst=rval->strtbl; n<nsyms; sp+=symSize,n++) {
		if ((symname=filter(sp,closure))) {
				memset(cesp,0,sizeof(*cesp));
				/* copy the name to the string table and put a pointer
				 * into the symbol table.
				 */
				cesp->name=dst;
				while ((*(dst++)=*(symname++)))
						/* do nothing else */;
				cesp->flags = 0;
				rval->aindex[cesp-rval->syms]=cesp;

				assign(sp,cesp,closure);

				
				cesp++;
		}
	}

	/* mark the last table entry */
	cesp->name=0;
	/* sort the tables */
	qsort((void*)rval->syms,
		rval->nentries,
		sizeof(*rval->syms),
		_cexp_namecomp);
	qsort((void*)rval->aindex,
		rval->nentries,
		sizeof(*rval->aindex),
		_cexp_addrcomp);

	return rval;

cleanup:
	cexpFreeSymTbl(&rval);
	return 0;
}



void
cexpFreeSymTbl(CexpSymTbl *pt)
{
CexpSymTbl st=*pt;
	if (st) {
		free(st->syms);
		free(st->strtbl);
		free(st->aindex);
		free(st);
	}
	*pt=0;
}

int
cexpSymPrintInfo(CexpSym s, FILE *f)
{
CexpType t=s->value.type;
	if (!f) f=stdout;

	/* convert variables (which are internally treated as pointers)
	 * to their base type for display
	 */
	if (!CEXP_TYPE_FUNQ(t))
		t=CEXP_TYPE_PTR2BASE(t);	
	return
		fprintf(f,"%p[%4d]: %s %s\n",
			(void*)s->value.ptv,
			s->size,
			cexpTypeInfoString(t),
			s->name);
}

/* do a binary search for an address returning its aindex number */
int
cexpSymTblLkAddrIdx(void *addr, int margin, FILE *f, CexpSymTbl t)
{
int			lo,hi,mid;

	lo=0; hi=t->nentries-1;
		
	while (lo < hi) {
		mid=(lo+hi+1)>>1; /* round up */
		if (addr < (void*)t->aindex[mid]->value.ptv)
			hi = mid-1;
		else
			lo = mid;
	}
	
	mid=lo;

	if (f) {
		lo=mid-margin; if (lo<0) 		 	lo=0;
		hi=mid+margin; if (hi>=t->nentries)	hi=t->nentries-1;
		while (lo<=hi)
			cexpSymPrintInfo(t->aindex[lo++],f);
	}
	return mid;
}

CexpSym
cexpSymTblLkAddr(void *addr, int margin, FILE *f, CexpSymTbl t)
{
	return t->aindex[cexpSymTblLkAddrIdx(addr,margin,f,t)];
}
