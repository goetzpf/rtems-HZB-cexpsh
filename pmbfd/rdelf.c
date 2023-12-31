/* $Id$ */

/* 
 * Authorship
 * ----------
 * This software ('pmelf' ELF file reader) was created by
 *     Till Straumann <strauman@slac.stanford.edu>, 2008,
 * 	   Stanford Linear Accelerator Center, Stanford University.
 * 
 * Acknowledgement of sponsorship
 * ------------------------------
 * This software was produced by
 *     the Stanford Linear Accelerator Center, Stanford University,
 * 	   under Contract DE-AC03-76SFO0515 with the Department of Energy.
 * 
 * Government disclaimer of liability
 * ----------------------------------
 * Neither the United States nor the United States Department of Energy,
 * nor any of their employees, makes any warranty, express or implied, or
 * assumes any legal liability or responsibility for the accuracy,
 * completeness, or usefulness of any data, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately owned
 * rights.
 * 
 * Stanford disclaimer of liability
 * --------------------------------
 * Stanford University makes no representations or warranties, express or
 * implied, nor assumes any liability for the use of this software.
 * 
 * Stanford disclaimer of copyright
 * --------------------------------
 * Stanford University, owner of the copyright, hereby disclaims its
 * copyright and all other rights in this software.  Hence, anyone may
 * freely use it for any purpose without restriction.  
 * 
 * Maintenance of notices
 * ----------------------
 * In the interest of clarity regarding the origin and status of this
 * SLAC software, this and all the preceding Stanford University notices
 * are to remain affixed to any copy or derivative of this software made
 * or distributed by the recipient and are to be affixed to any copy of
 * software made or distributed by the recipient that contains a copy or
 * derivative of this software.
 * 
 * ------------------ SLAC Software Notices, Set 4 OTT.002a, 2004 FEB 03
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pmelf.h"

#define PARANOIA_ON 0

#define DO_EHDR		1
#define DO_SHDRS	2
#define DO_SYMS		4
#define DO_GROUP	8
#define DO_RELS    16
#define DO_PHDRS   32

#include <getopt.h>

static char *optstr="achSsHglr";

static void usage(char *nm)
{
	fprintf(stderr,"usage: %s [-%s] <elf_file>\n",nm,optstr);
	fprintf(stderr,"   -H: print this information\n");
	fprintf(stderr,"   -h: print file header (ehdr)\n");
	fprintf(stderr,"   -S: print section headers (shdr)\n");
	fprintf(stderr,"   -g: print section groups\n");
	fprintf(stderr,"   -l: print program headers\n");
	fprintf(stderr,"   -s: print symbol table(s)\n");
	fprintf(stderr,"   -r: print relocation entries\n");
	fprintf(stderr,"   -a: enable all of -h, -s, -S, -g, -r\n");
	fprintf(stderr,"   -c: enable 'readelf' compatibility\n");
	fprintf(stderr,"       NOTE: for 64-bit use:\n");
	fprintf(stderr,"               readelf -WS\n");
	fprintf(stderr,"               rdelf   -cS\n");
}

static void
my_perror(const char *msg)
{
	if ( errno )
		fprintf(stderr,"%s: failed: %s\n", msg, strerror(errno));
	else
		fprintf(stderr,"%s: failed.\n",msg);
}

int main(int argc, char ** argv)
{
int          rval = 1;
Elf_Stream   s;
Elf_Ehdr     ehdr;
Pmelf_Shtab  shtab = 0;
Pmelf_Symtab symtab = 0;
Pmelf_Symtab dsymtab = 0;
int          ch;
int          doit=0;
int          compat=0;

	pmelf_set_errstrm(stderr);

	while ( (ch=getopt(argc, argv, optstr)) >= 0 ) {
		switch (ch) {
			case 'c': compat=1;          break;
			case 'h': doit |= DO_EHDR;   break;
			case 'S': doit |= DO_SHDRS;  break;
			case 's': doit |= DO_SYMS;   break;
			case 'g': doit |= DO_GROUP;  break;
			case 'r': doit |= DO_RELS;   break;
			case 'l': doit |= DO_PHDRS;  break;
			case 'a': doit |= -1;        break;
			case 'H': usage(argv[0]);    return 0;
			default:
				break;
		}
	}

	if ( optind >= argc ) {
		fprintf(stderr, "Not enough arguments -- need filename\n");
		usage(argv[0]);
		return 1;
	}

	/* Try mapstrm but fall back to a regular file stream in
	 * case mmap is not supported
	 */
	if ( ! (s = pmelf_mapstrm(argv[optind], 0)) ) {
		if ( ENOTSUP != errno ) {
			my_perror("Creating ELF (mmap) stream");
			return 1;
		}
	}

	if ( !s && ! (s = pmelf_newstrm(argv[optind], 0)) ) {
		my_perror("Creating ELF stream");
		return 1;
	}

	if ( pmelf_getehdr( s, &ehdr ) ) {
		my_perror("Reading ELF header");
		goto bail;
	}

	if ( doit & DO_EHDR )
		pmelf_dump_ehdr( stdout, &ehdr );

	if ( doit & DO_PHDRS ) {
		if ( pmelf_dump_phdrs( stdout, s, compat ? FMT_COMPAT : FMT_SHORT ) ) {
			my_perror("Dumping Program Headers");
			goto bail;
		}
	}

	if ( ! (shtab = pmelf_getshtab(s, &ehdr)) )
		goto bail;
		
	if ( doit & DO_SHDRS )
		pmelf_dump_shtab( stdout, shtab, compat ? FMT_COMPAT : FMT_SHORT );

	symtab  = pmelf_getsymtab(s, shtab);
	dsymtab = pmelf_getdsymtab(s, shtab);

	if ( doit & DO_GROUP )
		pmelf_dump_groups( stdout, s, shtab, symtab);

	if ( doit & DO_SYMS ) {
		if ( dsymtab ) {
			fprintf(stdout, "\nSymbol table '%s' contains %lu entries\n",
			                pmelf_get_section_name(shtab, dsymtab->idx),
			                dsymtab->nsyms);
			pmelf_dump_symtab( stdout, dsymtab, compat ? 0 : shtab, compat ? FMT_COMPAT : FMT_SHORT );
		}
		if ( symtab ) {
			fprintf(stdout, "\nSymbol table '%s' contains %lu entries\n",
			                pmelf_get_section_name(shtab, symtab->idx),
			                symtab->nsyms);
			pmelf_dump_symtab( stdout, symtab, compat ? 0 : shtab, compat ? FMT_COMPAT : FMT_SHORT );
		}
	}

	if ( doit & DO_RELS )
		pmelf_dump_rels( stdout, s, shtab, symtab );

	rval = 0;
bail:
	pmelf_delsymtab(dsymtab);
	pmelf_delsymtab(symtab);
	pmelf_delshtab(shtab);
	pmelf_delstrm(s,0);
	return rval;
}
