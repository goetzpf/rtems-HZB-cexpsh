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
#include "pmelfP.h"

void
pmelf_dump_symtab(FILE *f, Pmelf_Symtab symtab, Pmelf_Shtab shtab, int format)
{
Pmelf_Long  i;
Elf_Sym     *sym;
uint8_t     *p;
uint32_t    symsz = get_symsz(symtab);

	if ( !f )
		f = stdout;

	fprintf(f,"%6s: %8s%6s %-8s%-7s%-9s",
		"Num", "Value", "Size", "Type", "Bind", "Vis");
	if ( shtab )
		fprintf(f,"%-13s", "Section");
	else
		fprintf(f,"%3s", "Ndx");
	fprintf(f," Name\n");
	for ( i=0, p = symtab->syms.p_raw; i<symtab->nsyms; i++, p+=symsz ) {
		sym = (Elf_Sym*)p;
		fprintf(f,"%6lu: ", i);
#ifdef PMELF_CONFIG_ELF64SUPPORT
		if ( ELFCLASS64 == symtab->clss )
			pmelf_dump_sym64(f, &sym->t64, shtab, symtab->strtab, symtab->strtablen, format);
		else
#endif
			pmelf_dump_sym32(f, &sym->t32, shtab, symtab->strtab, symtab->strtablen, format);
	}
}
