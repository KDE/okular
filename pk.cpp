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
 *
 * NOTE:
 *	xdvi is based on prior work as noted in the modification history, below.
 */

/*
 * DVI previewer for X.
 *
 * Eric Cooper, CMU, September 1985.
 *
 * Code derived from dvi-imagen.c.
 *
 * Modification history:
 * 1/1986	Modified for X.10	--Bob Scheifler, MIT LCS.
 * 7/1988	Modified for X.11	--Mark Eichin, MIT
 * 12/1988	Added 'R' option, toolkit, magnifying glass
 *					--Paul Vojta, UC Berkeley.
 * 2/1989	Added tpic support	--Jeffrey Lee, U of Toronto
 * 4/1989	Modified for System V	--Donald Richardson, Clarkson Univ.
 * 3/1990	Added VMS support	--Scott Allendorf, U of Iowa
 * 7/1990	Added reflection mode	--Michael Pak, Hebrew U of Jerusalem
 * 1/1992	Added greyscale code	--Till Brychcy, Techn. Univ. Muenchen
 *					  and Lee Hetherington, MIT
 * 4/1994	Added DPS support, bounding box
 *					--Ricardo Telichevesky
 *					  and Luis Miguel Silveira, MIT RLE.
 */

#include <kdebug.h>
#include <klocale.h>

#include "font.h"
#include "dviwin.h"
#include "kdvi.h"  // This is where debugging flags are set.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glyph.h"
#include "xdvi.h"

extern void oops(QString message);
extern void alloc_bitmap(bitmap *bitmap);


// This table is used for changing the bit order in a byte. The
// expression bitflp[byte] takes a byte in big endian and gives the
// little endian equivalent of that.
static const uchar bitflip[256] = {
  0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
  8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
  4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
  12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
  2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
  10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
  6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
  14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
  1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
  9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
  5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
  13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
  3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
  11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
  7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
  15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255
};                                                                              

BMUNIT	bit_masks[33] = {
	0x0,		0x1,		0x3,		0x7,
	0xf,		0x1f,		0x3f,		0x7f,
	0xff,		0x1ff,		0x3ff,		0x7ff,
	0xfff,		0x1fff,		0x3fff,		0x7fff,
	0xffff,		0x1ffff,	0x3ffff,	0x7ffff,
	0xfffff,	0x1fffff,	0x3fffff,	0x7fffff,
	0xffffff,	0x1ffffff,	0x3ffffff,	0x7ffffff,
	0xfffffff,	0x1fffffff,	0x3fffffff,	0x7fffffff,
	0xffffffff
};


/***
 ***	PK font reading routines.
 ***	Public routines are read_PK_index and read_PK_char.
 ***/

#define PK_ID      89
#define PK_CMD_START 240
#define PK_X1     240
#define PK_X2     241
#define PK_X3     242
#define PK_X4     243
#define PK_Y      244
#define PK_POST   245
#define PK_NOOP   246
#define PK_PRE    247

static	int	PK_flag_byte;
static	unsigned PK_input_byte;
static	int	PK_bitpos;
static	int	PK_dyn_f;
static	int	PK_repeat_count;

int font::PK_get_nyb(FILE *fp)
{
#ifdef DEBUG_PK
  kdDebug() << "PK_get_nyb" << endl;
#endif
	unsigned temp;
	if (PK_bitpos < 0) {
	    PK_input_byte = one(fp);
	    PK_bitpos = 4;
	}
	temp = PK_input_byte >> PK_bitpos;
	PK_bitpos -= 4;
	return (temp & 0xf);
}


int font::PK_packed_num(FILE *fp)
{
#ifdef DEBUG_PK
  kdDebug() << "PK_packed_num" << endl;
#endif

  int	i,j;

  if ((i = PK_get_nyb(fp)) == 0) {
    do {
      j = PK_get_nyb(fp);
      ++i;
    }
    while (j == 0);
    while (i > 0) {
      j = (j << 4) | PK_get_nyb(fp);
      --i;
    }
    return (j - 15 + ((13 - PK_dyn_f) << 4) + PK_dyn_f);
  }
  else {
    if (i <= PK_dyn_f) return i;
    if (i < 14)
      return (((i - PK_dyn_f - 1) << 4) + PK_get_nyb(fp)
	      + PK_dyn_f + 1);
    if (i == 14) PK_repeat_count = PK_packed_num(fp);
    else PK_repeat_count = 1;
    return PK_packed_num(fp);
  }
}


void font::PK_skip_specials(void)
{
#ifdef DEBUG_PK
  kdDebug() << "PK_skip_specials" << endl;
#endif

  int i,j;
  register FILE *fp = file;

  do {
    PK_flag_byte = one(fp);
    if (PK_flag_byte >= PK_CMD_START) {
      switch (PK_flag_byte) {
      case PK_X1 :
      case PK_X2 :
      case PK_X3 :
      case PK_X4 :
	i = 0;
	for (j = PK_CMD_START; j <= PK_flag_byte; ++j)
	  i = (i << 8) | one(fp);
	while (i--) (void) one(fp);
	break;
      case PK_Y :
	(void) four(fp);
      case PK_POST :
      case PK_NOOP :
	break;
      default :
	oops(i18n("Unexpected %1 in PK file %2").arg(PK_flag_byte).arg(filename) );
	break;
      }
    }
  }
  while (PK_flag_byte != PK_POST && PK_flag_byte >= PK_CMD_START)
    ;
}

/*
 *	Public routines
 */

void font::read_PK_char(unsigned int ch)
{
#ifdef DEBUG_PK
  kdDebug() << "read_PK_char" << endl;
#endif

  int	i, j;
  int	n;
  int	row_bit_pos;
  bool	paint_switch;
  BMUNIT	*cp;
  register struct glyph *g;
  register FILE *fp = file;
  long	fpwidth;
  BMUNIT	word = 0;
  int	word_weight, bytes_wide;
  int	rows_left, h_bit, count;

  g = glyphtable + ch;
  PK_flag_byte = g->x2;
  PK_dyn_f = PK_flag_byte >> 4;
  paint_switch = ((PK_flag_byte & 8) != 0);
  PK_flag_byte &= 0x7;
  if (PK_flag_byte == 7)
    n = 4;
  else 
    if (PK_flag_byte > 3)
      n = 2;
    else
      n = 1;
  
#ifdef DEBUG_PK
  kdDebug() << "loading pk char " << ch << ", char type " << n << endl;
#endif

  /*
   * now read rest of character preamble
   */
  if (n != 4)
    fpwidth = num(fp, 3);
  else {
    fpwidth = sfour(fp);
    (void) four(fp);	/* horizontal escapement */
  }
  (void) num(fp, n);	/* vertical escapement */
  {
    unsigned long w, h;

    w = num(fp, n);
    h = num(fp, n);
    if (w > 0x7fff || h > 0x7fff)
      oops(i18n("The character %1 is too large in file %2").arg(ch).arg(fontname));
    g->bitmap.w = w;
    g->bitmap.h = h;
  }
  g->x = snum(fp, n);
  g->y = snum(fp, n);

  g->dvi_adv = (int)(dimconv * fpwidth + 0.5);
  
  alloc_bitmap(&g->bitmap);
  cp = (BMUNIT *) g->bitmap.bits;

  /*
   * read character data into *cp
   */
  bytes_wide = ROUNDUP((int) g->bitmap.w, BITS_PER_BMUNIT) * BYTES_PER_BMUNIT;
  PK_bitpos = -1;

  // The routines which read the character depend on the byte
  // ordering. In principle, the byte order should be detected at
  // compile time and the proper routing chosen. For the moment, as
  // autoconf is somewhat complicated for the author, we prefer a
  // simpler -even if somewhat slower approach and detect the ordering
  // at runtime. That should of course be changed in the future.

  int wordSize;
  bool bigEndian;
  qSysInfo (&wordSize, &bigEndian);

  if (bigEndian) { 
    // Routine for big Endian machines. Applies e.g. to Motorola and
    // (Ultra-)Sparc processors.

#ifdef DEBUG_PK
    kdDebug() << "big Endian byte ordering" << endl;
#endif

    if (PK_dyn_f == 14) {	/* get raster by bits */
      memset(g->bitmap.bits, 0, (int) g->bitmap.h * bytes_wide);
      for (i = 0; i < (int) g->bitmap.h; i++) {	/* get all rows */
	cp = ADD(g->bitmap.bits, i * bytes_wide);
	row_bit_pos = BITS_PER_BMUNIT;
	for (j = 0; j < (int) g->bitmap.w; j++) {    /* get one row */
	  if (--PK_bitpos < 0) {
	    word = one(fp);
	    PK_bitpos = 7;
	  }
	  if (--row_bit_pos < 0) {
	    cp++;
	    row_bit_pos = BITS_PER_BMUNIT - 1;
	  }
	  if (word & (1 << PK_bitpos)) 
	    *cp |= 1 << row_bit_pos;
	}
      }
    } else {		/* get packed raster */
      rows_left = g->bitmap.h;
      h_bit = g->bitmap.w;
      PK_repeat_count = 0;
      word_weight = BITS_PER_BMUNIT;
      word = 0;
      while (rows_left > 0) {
	count = PK_packed_num(fp);
	while (count > 0) {
	  if (count < word_weight && count < h_bit) {
	    h_bit -= count;
	    word_weight -= count;
	    if (paint_switch)
	      word |= bit_masks[count] << word_weight;
	    count = 0;
	  } else 
	    if (count >= h_bit && h_bit <= word_weight) {
	      if (paint_switch)
		word |= bit_masks[h_bit] << (word_weight - h_bit);
	      *cp++ = word;
	      /* "output" row(s) */
	      for (i = PK_repeat_count * bytes_wide /
		     BYTES_PER_BMUNIT; i > 0; --i) {
		*cp = *SUB(cp, bytes_wide);
		++cp;
	      }
	      rows_left -= PK_repeat_count + 1;
	      PK_repeat_count = 0;
	      word = 0;
	      word_weight = BITS_PER_BMUNIT;
	      count -= h_bit;
	      h_bit = g->bitmap.w;
	    } else {
	      if (paint_switch)
		word |= bit_masks[word_weight];
	      *cp++ = word;
	      word = 0;
	      count -= word_weight;
	      h_bit -= word_weight;
	      word_weight = BITS_PER_BMUNIT;
	    }
	}
	paint_switch = 1 - paint_switch;
      }
      if (cp != ((BMUNIT *) (g->bitmap.bits + bytes_wide * g->bitmap.h)))
	oops(i18n("Wrong number of bits stored:  char. %1, font %2").arg(ch).arg(fontname));
      if (rows_left != 0 || h_bit != g->bitmap.w)
	oops(i18n("Bad pk file (%1), too many bits").arg(fontname));
    }

    // The data in the bitmap is now in the processor's bit order,
    // that is, big endian. Since XWindows needs little endian, we
    // need to change the bit order now.
    register unsigned char* bitmapData = (unsigned char*) g->bitmap.bits;
    register unsigned char* endOfData  = bitmapData + g->bitmap.bytes_wide*g->bitmap.h;
    while(bitmapData < endOfData) {
      *bitmapData = bitflip[*bitmapData];
      bitmapData++;
    }

  } else {

    // Routines for small Endian start here. This applies e.g. to
    // Intel and Alpha processors.

#ifdef DEBUG_PK
    kdDebug() << "small Endian byte ordering" << endl;
#endif

    if (PK_dyn_f == 14) {	/* get raster by bits */
      memset(g->bitmap.bits, 0, (int) g->bitmap.h * bytes_wide);
      for (i = 0; i < (int) g->bitmap.h; i++) {	/* get all rows */
	cp = ADD(g->bitmap.bits, i * bytes_wide);
	row_bit_pos = -1;
	for (j = 0; j < (int) g->bitmap.w; j++) {    /* get one row */
	  if (--PK_bitpos < 0) {
	    word = one(fp);
	    PK_bitpos = 7;
	  }
	  if (++row_bit_pos >= BITS_PER_BMUNIT) {
	    cp++;
	    row_bit_pos = 0;
	  }
	  if (word & (1 << PK_bitpos)) 
	    *cp |= 1 << row_bit_pos;
	}
      }
    } else {		/* get packed raster */
      rows_left = g->bitmap.h;
      h_bit = g->bitmap.w;
      PK_repeat_count = 0;
      word_weight = BITS_PER_BMUNIT;
      word = 0;
      while (rows_left > 0) {
	count = PK_packed_num(fp);
	while (count > 0) {
	  if (count < word_weight && count < h_bit) {
	    if (paint_switch)
	      word |= bit_masks[count] << (BITS_PER_BMUNIT - word_weight);
	    h_bit -= count;
	    word_weight -= count;
	    count = 0;
	  } else 
	    if (count >= h_bit && h_bit <= word_weight) {
	      if (paint_switch)
		word |= bit_masks[h_bit] << (BITS_PER_BMUNIT - word_weight);
	      *cp++ = word;
	      /* "output" row(s) */
	      for (i = PK_repeat_count * bytes_wide /
		     BYTES_PER_BMUNIT; i > 0; --i) {
		*cp = *SUB(cp, bytes_wide);
		++cp;
	      }
	      rows_left -= PK_repeat_count + 1;
	      PK_repeat_count = 0;
	      word = 0;
	      word_weight = BITS_PER_BMUNIT;
	      count -= h_bit;
	      h_bit = g->bitmap.w;
	    } else {
	      if (paint_switch)
		word |= bit_masks[word_weight] << (BITS_PER_BMUNIT - word_weight);
	      *cp++ = word;
	      word = 0;
	      count -= word_weight;
	      h_bit -= word_weight;
	      word_weight = BITS_PER_BMUNIT;
	    }
	}
	paint_switch = 1 - paint_switch;
      }
      if (cp != ((BMUNIT *) (g->bitmap.bits + bytes_wide * g->bitmap.h)))
	oops(i18n("Wrong number of bits stored:  char. %1, font %2").arg(ch).arg(fontname));
      if (rows_left != 0 || h_bit != g->bitmap.w)
	oops(i18n("Bad pk file (%1), too many bits").arg(fontname));
    }
  } // endif: big or small Endian?
}

void font::read_PK_index(void)
{
#ifdef DEBUG_PK
  kdDebug() << "Reading PK pixel file " << filename << endl;
#endif

  fseek(file, (long) one(file), SEEK_CUR);	/* skip comment */

  (void) four(file);		/* skip design size */
  long file_checksum = four(file);
  if (checksum && checksum && file_checksum != checksum)
    kdError(1) << i18n("Checksum mismatch") << " (dvi = " << checksum << "pk = " << file_checksum << 
      ") " << i18n("in font file ") << filename << endl;

  int hppp = sfour(file);
  int vppp = sfour(file);
  if (hppp != vppp)
    kdDebug() << i18n("Font has non-square aspect ratio ") << vppp << ":" << hppp << endl;

  // Prepare glyph array.
  glyphtable = new glyph[max_num_of_chars_in_font];
  if (glyphtable == 0) {
    kdError() << i18n("Could not allocate memory for a glyph table.") << endl;
    exit(0);
  }
    

  // Read glyph directory (really a whole pass over the file).
  for (;;) {
    int bytes_left, flag_low_bits;
    unsigned int ch;
    
    PK_skip_specials();
    if (PK_flag_byte == PK_POST)
      break;
    flag_low_bits = PK_flag_byte & 0x7;
    if (flag_low_bits == 7) {
      bytes_left = four(file);
      ch = four(file);
    } else 
      if (flag_low_bits > 3) {
	bytes_left = ((flag_low_bits - 4) << 16) + two(file);
	ch = one(file);
      } else {
	bytes_left = (flag_low_bits << 8) + one(file);
	ch = one(file);
      }
    glyphtable[ch].addr = ftell(file);
    glyphtable[ch].x2 = PK_flag_byte;
    fseek(file, (long) bytes_left, SEEK_CUR);
#ifdef DEBUG_PK
    kdDebug() << "Scanning pk char " << ch << "at " << glyphtable[ch].addr << endl;
#endif
  }
}
