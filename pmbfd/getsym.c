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

#ifdef PMELF_CONFIG_ELF64SUPPORT
int
pmelf_getsym64(Elf_Stream s, Elf64_Sym *psym)
{
	if ( 1 != SREAD(psym, sizeof(*psym), 1, s) ) {
		return -1;
	}
	if ( s->needswap ) {
#ifdef PMELF_CONFIG_NO_SWAPSUPPORT
		return -2;
#else
		elf_swap32( &psym->st_name);
		elf_swap16( &psym->st_shndx);
		elf_swap64( &psym->st_value);
		elf_swap64( &psym->st_size);
#endif
	}
#ifdef PARANOIA_ON
	{
	int x;
	if ( (x = ELF64_ST_TYPE( psym->st_info )) > STT_MAXSUP ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsym - paranoia: unsupported type %i\n", x);
		return -1;
	}
	if ( (x = ELF64_ST_BIND( psym->st_info )) > STB_MAXSUP ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsym - paranoia: unsupported binding %i\n", x);
		return -1;
	}
	}
#endif
	return 0;
}
#endif

int
pmelf_getsym32(Elf_Stream s, Elf32_Sym *psym)
{
	if ( 1 != SREAD(psym, sizeof(*psym), 1, s) ) {
		return -1;
	}
	if ( s->needswap ) {
#ifdef PMELF_CONFIG_NO_SWAPSUPPORT
		return -2;
#else
		elf_swap32( &psym->st_name);
		elf_swap32( &psym->st_value);
		elf_swap32( &psym->st_size);
		elf_swap16( &psym->st_shndx);
#endif
	}
#ifdef PARANOIA_ON
	{
	int x;
	if ( (x = ELF32_ST_TYPE( psym->st_info )) > STT_MAXSUP ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsym - paranoia: unsupported type %i\n", x);
		return -1;
	}
	if ( (x = ELF32_ST_BIND( psym->st_info )) > STB_MAXSUP ) {
		PMELF_PRINTF( pmelf_err, PMELF_PRE"pmelf_getsym - paranoia: unsupported binding %i\n", x);
		return -1;
	}
	}
#endif
	return 0;
}
