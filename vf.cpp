/*
 * Copyright (c) 1994 Paul Vojta.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include <kdebug.h>
#include <klocale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dvi.h"
#include "dviRenderer.h"
#include "fontpool.h"
#include "xdvi.h"

extern void oops(QString message);

/***
 ***	VF font reading routines.
 ***	Public routine is read_index---because virtual characters are presumed
 ***	to be short, we read the whole virtual font in at once, instead of
 ***	faulting in characters as needed.
 ***/

#define	LONG_CHAR	242

/*
 *	These are parameters which determine whether macros are combined for
 *	storage allocation purposes.  Small macros ( <= VF_PARM_1 bytes) are
 *	combined into chunks of size VF_PARM_2.
 */

#ifndef	VF_PARM_1
#define	VF_PARM_1	20
#endif
#ifndef	VF_PARM_2
#define	VF_PARM_2	256
#endif

/*
 *	The main routine
 */

void TeXFontDefinition::read_VF_index()
{
#ifdef DEBUG_FONTS
  kdDebug(4300) << "font::read_VF_index()" << endl;
#endif
  FILE *VF_file = file;
  unsigned char	cmnd;
  unsigned char	*avail, *availend;	/* available space for macros */

  flags      |= FONT_VIRTUAL;
  set_char_p  = &dviRenderer::set_vf_char;
#ifdef DEBUG_FONTS
  kdDebug(4300) << "TeXFontDefinition::read_VF_index: reading VF pixel file " << filename << endl;
#endif
  // Read preamble.
  fseek(VF_file, (long) one(VF_file), 1);	/* skip comment */
  Q_UINT32 file_checksum = four(VF_file);

  if (file_checksum && checksum && file_checksum != checksum)
    kdError(4300) << i18n("Checksum mismatch") << "(dvi = " << checksum << "u, vf = " << file_checksum << 
      "u)" << i18n(" in font file ") << filename << endl;
  (void) four(VF_file);		/* skip design size */

  // Read the fonts.
  first_font = NULL;
  while ((cmnd = one(VF_file)) >= FNTDEF1 && cmnd <= FNTDEF4) {
    int   TeXnumber = num(VF_file, (int) cmnd - FNTDEF1 + 1);
    Q_UINT32 checksum  = four(VF_file);
    Q_UINT32 scale     = four(VF_file);
    Q_UINT32 design    = four(VF_file);
    Q_UNUSED(design);
    Q_UINT16 len       = one(VF_file) + one(VF_file); /* sequence point in the middle */
    char *fontname  = new char[len + 1];
    fread(fontname, sizeof(char), len, VF_file);
    fontname[len] = '\0';
    
#ifdef DEBUG_FONTS
    kdDebug(4300) << "Virtual font defines subfont \"" << fontname << "\" scale=" << scale << " design=" << design << endl;
#endif

    // According to Knuth's documentation found in the web source code
    // of the "vftovp" program (which seems to be the standard
    // definition of virtual fonts), the "scale" is a fixed point
    // number which describes extra enlargement that the virtual font
    // imposes. One obtains the enlargement by dividing 2^20. 
    double enlargement_factor = double(scale)/(1<<20) * enlargement;

    //    TeXFontDefinition *newfontp = font_pool->appendx(fontname, checksum, (Q_UINT32)(scaled_size_in_DVI_units*enlargement_factor), enlargement_factor);
    TeXFontDefinition *newfontp = font_pool->appendx(fontname, checksum, (Q_UINT32)((double(scale)/(1<<20))*scaled_size_in_DVI_units), enlargement_factor);

    // Insert font in dictionary and make sure the dictionary is big
    // enough.
    if (vf_table.size()-2 <= vf_table.count())
      // Not quite optimal. The size of the dictionary should be a
      // prime. I don't care.
      vf_table.resize(vf_table.size()*2); 
    vf_table.insert(TeXnumber, newfontp);
    
    if (first_font == NULL)
      first_font = newfontp;
  }

  // Prepare macro array.
  macrotable = new macro[max_num_of_chars_in_font];
  if (macrotable == 0) {
    kdError() << i18n("Could not allocate memory for a macro table.") << endl;
    exit(0);
  }

  // Read macros.
  avail = availend = NULL;
  for (; cmnd <= LONG_CHAR; cmnd = one(VF_file)) {
    macro *m;
    int len;
    unsigned long cc;
    long width;

    if (cmnd == LONG_CHAR) {	/* long form packet */
      len = four(VF_file);
      cc = four(VF_file);
      width = four(VF_file);
      if (cc >= 256) {
	kdError() << i18n("Virtual character ") << cc << i18n(" in font ") 
		  << fontname << i18n(" ignored.") << endl;
	fseek(VF_file, (long) len, 1);
	continue;
      }
    } else {	/* short form packet */
      len = cmnd;
      cc = one(VF_file);
      width = num(VF_file, 3);
    }
    m = &macrotable[cc];

    m->dvi_advance_in_units_of_design_size_by_2e20 = width;
    if (len > 0) {
      if (len <= availend - avail) {
	m->pos = avail;
	avail += len;
      } else {
	m->free_me = true;
	if (len <= VF_PARM_1) {
	  m->pos = avail = new unsigned char [VF_PARM_2];
	  availend = avail + VF_PARM_2;
	  avail += len;
	} else 
	  m->pos = new unsigned char[len];
      }
      fread((char *) m->pos, 1, len, VF_file);
      m->end = m->pos + len;
    }
  }
  if (cmnd != POST)
    oops(i18n("Wrong command byte found in VF macro list: %1").arg(cmnd));
  
  fclose (VF_file);
  file = NULL;
}
