#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>


#define BUFMAX	500

#include "cexp.h"
#include "context.h"
#include "cexpsymsP.h"
#include "cexpmodP.h"

#include "dis-asm.h"

static disassembler_ftype	bfdDisassembler=0;
enum bfd_endian				bfdEndian=BFD_ENDIAN_UNKNOWN;

typedef struct DAStreamRec_ {
	char	buf[BUFMAX];	/* buffer to assemble the line 		*/
	int		p;				/* 'cursor'							*/
} DAStreamRec, *DAStream;

static int
daPrintf(DAStream s, char *fmt, ...)
{
va_list ap;
int		written;
	va_start(ap, fmt);

#ifdef HAVE_VSNPRINTF
	written=vsnprintf(s->buf+s->p, BUFMAX - s->p, fmt, ap);
#else
	written=vsprintf(s->buf+s->p, fmt, ap);
#endif

	assert(written >= 0  && (s->p+=written) < BUFMAX);

	va_end(ap);
	return written;
}

static void
printSymAddr(bfd_vma addr, CexpModule mod, CexpSym sym,  disassemble_info *di)
{
	if (!sym) {
		di->fprintf_func(di->stream,"?NULL?");
		return;
	} else {
		long diff=addr - (bfd_vma)sym->value.ptv;
		char diffbuf[30];
		if (diff)
			sprintf(diffbuf," + 0x%x",(unsigned)diff);
		else
			diffbuf[0]=0;

		di->fprintf_func(di->stream,
						"<%s:%s%s>",
						cexpModuleName(mod),
						sym->name,
						diffbuf);
	}
}

static void
printAddr(bfd_vma addr, disassemble_info *di)
{
CexpSym		sym;
CexpModule	mod;
	sym = cexpSymLkAddr((void*)addr, 0, 0, &mod);
	printSymAddr(addr,mod,sym,di);
}

static int
symbolAtAddr(bfd_vma addr, disassemble_info *di)
{
CexpSym	s;
	return (s=cexpSymLkAddr((void*)addr,0,0,0)) &&
			(void*)s->value.ptv == (void*)addr;
}

static int
readMem(bfd_vma vma, bfd_byte *buf, unsigned int length, disassemble_info *di)
{
	/* memory is already holding the data we want to disassemble */
	return 0;
}


void
cexpDisassemblerInit(disassemble_info *di, PTR stream)
{
DAStreamRec dummy;

	dummy.p = 0;
	INIT_DISASSEMBLE_INFO((*di),(PTR)&dummy, (fprintf_ftype)daPrintf);
	/* don't need the buffer_length; just set to a value high enough */
	di->buffer_length			= 100;
	
	di->display_endian 			= di->endian = bfdEndian;
	di->buffer 					= (bfd_byte *)cexpDisassemblerInit;
	di->symbol_at_address_func	= symbolAtAddr;
	di->print_address_func		= printAddr;
	/* disassemble one line to set the bytes_per_line field */
	if (bfdDisassembler) {
		bfdDisassembler((bfd_vma)di->buffer, di);
	}
	/* reset stream */
	di->stream 			= stream;
	di->fprintf_func	= (fprintf_ftype)fprintf;
}

void
cexpDisassemblerInstall(bfd *abfd)
{
	if (bfdDisassembler)
		return; /* has been installed already */

	if (!(bfdDisassembler = disassembler(abfd))) {
		bfd_perror("Warning: no disassembler found");
		return;
	}
	if (bfd_big_endian(abfd))
		bfdEndian = BFD_ENDIAN_BIG;
	else if (bfd_little_endian(abfd))
		bfdEndian = BFD_ENDIAN_LITTLE;
	else {
		fprintf(stderr,
			"UNKNOWN BFD ENDIANNESS; unable to install disassembler\n");
		bfdDisassembler=0;
	}
}

static CexpSym	
getNextSym(int *pindex, CexpModule *pmod, void *addr)
{
	if (*pindex < 0 || *pindex >= (*pmod)->symtbl->nentries-1) {
		/* reached the end of the module's symbol table;
		 * search module list again
		 */
		*pindex = cexpSymLkAddrIdx(addr, 0, 0, pmod);
		if (*pindex < 0)
			return 0;
	} else {
		(*pindex)++;
	}
	return (*pmod)->symtbl->aindex[*pindex];
}

void
cexpDisassemble(void *addr, int n, disassemble_info *di)
{
FILE			*f;
fprintf_ftype	orig_fprintf;
DAStreamRec		b;
CexpSym			currSym,nextSym;
CexpModule		currMod,nextMod;
int				found;

	if (!bfdDisassembler) {
		fprintf(stderr,"No disassembler support\n");
		  return;
	}

	if (!di) {
		CexpContext currentContext = cexpContextGetCurrent();
		assert(currentContext);
		di = &currentContext->dinfo;
	}
	if (addr)
		di->buffer=addr;
	/* redirect the stream */
	orig_fprintf = di->fprintf_func;
	f = di->stream;

	di->stream = (PTR) &b;
	b.p = 0;
	di->fprintf_func = (fprintf_ftype)daPrintf;

	if (n<1)
		n=10;

	found=-1;
	currSym=getNextSym(&found,&currMod,di->buffer);
	nextSym=0;

	while (n-- > 0) {
		int decoded,i,j,k,clip,spaces,bpc,bpl;

		if (currSym) {
			printSymAddr((bfd_vma)di->buffer,currMod, currSym, di);
			b.p=0;
			orig_fprintf(f,"\n%s:\n\n",b.buf);
			currSym=0;
		}

		di->buffer_vma = (bfd_vma)di->buffer;

		decoded = bfdDisassembler((bfd_vma)di->buffer, di);

		bpc = di->bytes_per_chunk;
		if (0==bpc) {
			/* many targets don't set/use this */
			bpc=1;
		}

		bpl = di->bytes_per_line;
		if (0==bpl) {
			/* some targets don't set this; take a wild guess
			 * (same as objdump)
			 */
			bpl = 4;
		}


		/* print the data in 'bytes_per_chunk' chunks
		 * who are endian-converted. Limit one line's
		 * output to  'bytes_per_line'.
		 */
		clip = decoded > bpl ? bpl : decoded;
		for (i=0; i < decoded; i+=clip) {
			/* print address */
			orig_fprintf(f,"0x%08x: ",di->buffer + i);
			for (k=0; k < clip && k+i < decoded; k+=bpc) {
				for (j=0; j<bpc; j++) {
					if (BFD_ENDIAN_LITTLE == di->display_endian)
						orig_fprintf(f,"%02x",di->buffer[i + k + j]);
					else
						orig_fprintf(f,"%02x",di->buffer[i + k + bpc - 1 - j]);
				}
				orig_fprintf(f," ");
			}
			if (i==0) {
				spaces = (bpl - clip);
				spaces = 2 * spaces + spaces/bpc;
				orig_fprintf(f,"  %*s%s\n",spaces,"",b.buf);
			} else {
				orig_fprintf(f,"\n");
			}
		}

		di->buffer+=decoded;

		if (!nextSym) {
			nextMod=currMod;
			nextSym=getNextSym(&found,&nextMod,di->buffer);
		}
		if (nextSym) {
			if (di->buffer >= (bfd_byte *)nextSym->value.ptv) {
				currSym=nextSym; currMod=nextMod;
				nextSym=0;
			}
		}
		b.p=0;
	}

	/* restore the stream */
	di->stream = f;
	di->fprintf_func = orig_fprintf;
}