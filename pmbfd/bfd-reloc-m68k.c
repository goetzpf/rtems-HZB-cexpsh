/* $Id$ */

/* 
 * Authorship
 * ----------
 * This software ('pmbfd' BFD emulation for cexpsh) was created by
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

#include "pmbfdP.h"

bfd_reloc_status_type
pmbfd_perform_relocation(bfd *abfd, pmbfd_arelent *r, asymbol *psym, asection *input_section)
{
Elf32_Word     pc = bfd_get_section_vma(abfd, input_section);

unsigned       sz;
int32_t        val;
int8_t         cval;
int16_t        sval;
uint32_t       pcadd=0;
int32_t        lim;

	if ( bfd_is_und_section(bfd_get_section(psym)) )
		return bfd_reloc_undefined;

	pc += r->rela.r_offset;

	/* I don't have my hands on the 68k ABI document so
	 * I don't know if there are any alignment requirements.
	 * Anyhow, if we loaded the section correctly and
	 * the compiler did the right thing this should
	 * not be an issue.
	 */

	switch ( ELF32_R_TYPE(r->rela.r_info) ) {

		default:
		return bfd_reloc_notsupported;

		case R_68K_PC8:  pcadd = pc; lim   = -0x0080; sz = 1; break;
		case R_68K_8:    pcadd =  0; lim   = -0x0100; sz = 1; break;

		case R_68K_PC16: pcadd = pc; lim   = -0x8000; sz = 2; break;
		case R_68K_16:   pcadd =  0; lim   = -0x1000; sz = 2; break;

		case R_68K_PC32: pcadd = pc; lim   = 0;       sz = 4; break;
		case R_68K_32:   pcadd =  0; lim   = 0;       sz = 4; break;
	}

	if ( r->rela.r_offset + sz > bfd_get_section_size(input_section) )
		return bfd_reloc_outofrange;

	switch (sz) {
		case 1:
			memcpy(&cval, (void*)pc, sizeof(cval));
			val = cval;
		break;

		case 2:
			memcpy(&sval, (void*)pc, sizeof(sval));
			val = sval;
		break;

		case 4:
			memcpy(&val, (void*)pc, sizeof(val));
		break;
	}

	val = val + bfd_asymbol_value(psym) + r->rela.r_addend - pcadd;

	if ( lim && (val < lim || val > -lim - 1) )
		return bfd_reloc_overflow;

	switch (sz) {
		case 1:
			cval = val;
			memcpy((void*)pc, &cval, sizeof(cval));
		break;

		case 2:
			sval = val;
			memcpy((void*)pc, &sval, sizeof(sval));
		break;

		case 4:
			memcpy((void*)pc, &val, sizeof(val));
		break;
	}

	return bfd_reloc_ok;
}

#define namecase(rel)	case rel: return #rel;

const char *
pmbfd_reloc_get_name(bfd *abfd, pmbfd_arelent *r)
{
	switch ( ELF32_R_TYPE(r->rel.r_info) ) {
		namecase( R_68K_NONE )
		namecase( R_68K_32 )
		namecase( R_68K_16 )
		namecase( R_68K_8 )
		namecase( R_68K_PC32 )
		namecase( R_68K_PC16 )
		namecase( R_68K_PC8 )
		namecase( R_68K_GOT32 )
		namecase( R_68K_GOT16 )
		namecase( R_68K_GOT8 )
		namecase( R_68K_GOT320 )
		namecase( R_68K_GOT160 )
		namecase( R_68K_GOT80 )
		namecase( R_68K_PLT32 )
		namecase( R_68K_PLT16 )
		namecase( R_68K_PLT8 )
		namecase( R_68K_PLT320 )
		namecase( R_68K_PLT160 )
		namecase( R_68K_PLT80 )
		namecase( R_68K_COPY )
		namecase( R_68K_GLOB_DAT )
		namecase( R_68K_JMP_SLOT )
		namecase( R_68K_RELATIVE )

		default:
		break;
	}
	return "UNKNOWN";
}