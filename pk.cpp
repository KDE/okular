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

#include "font.h"
#include "dviwin.h"

#include <stdio.h>
#include <stdlib.h>
#include "glyph.h"
#include "oconfig.h"
extern	char  *xmalloc (unsigned, const char *);
extern void oops(QString message);
extern void alloc_bitmap(bitmap *bitmap);

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
	oops(QString("Unexpected %1 in PK file %2").arg(PK_flag_byte).arg(filename) );
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
  Boolean	paint_switch;
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
      oops(QString("Character %1 too large in file %2").arg(ch).arg(fontname));
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
  bytes_wide = ROUNDUP((int) g->bitmap.w, BITS_PER_BMUNIT)
    * BYTES_PER_BMUNIT;
  PK_bitpos = -1;
  if (PK_dyn_f == 14) {	/* get raster by bits */
    bzero(g->bitmap.bits, (int) g->bitmap.h * bytes_wide);
    for (i = 0; i < (int) g->bitmap.h; i++) {	/* get all rows */
      cp = ADD(g->bitmap.bits, i * bytes_wide);
#ifndef	MSBITFIRST
      row_bit_pos = -1;
#else
      row_bit_pos = BITS_PER_BMUNIT;
#endif
      for (j = 0; j < (int) g->bitmap.w; j++) {    /* get one row */
	if (--PK_bitpos < 0) {
	  word = one(fp);
	  PK_bitpos = 7;
	}
#ifndef	MSBITFIRST
	if (++row_bit_pos >= BITS_PER_BMUNIT) {
	  cp++;
	  row_bit_pos = 0;
	}
#else
	if (--row_bit_pos < 0) {
	  cp++;
	  row_bit_pos = BITS_PER_BMUNIT - 1;
	}
#endif
	if (word & (1 << PK_bitpos)) *cp |= 1 << row_bit_pos;
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
#ifndef	MSBITFIRST
	  if (paint_switch)
	    word |= bit_masks[count] <<
	      (BITS_PER_BMUNIT - word_weight);
#endif
	  h_bit -= count;
	  word_weight -= count;
#ifdef	MSBITFIRST
	  if (paint_switch)
	    word |= bit_masks[count] << word_weight;
#endif
	  count = 0;
	} else 
	  if (count >= h_bit && h_bit <= word_weight) {
	    if (paint_switch)
	      word |= bit_masks[h_bit] <<
#ifndef	MSBITFIRST
		(BITS_PER_BMUNIT - word_weight);
#else
	    (word_weight - h_bit);
#endif
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
#ifndef	MSBITFIRST
	      word |= bit_masks[word_weight] <<
		(BITS_PER_BMUNIT - word_weight);
#else
	    word |= bit_masks[word_weight];
#endif
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
      oops(QString("Wrong number of bits stored:  char. %1, font %2").arg(ch).arg(fontname));
    if (rows_left != 0 || h_bit != g->bitmap.w)
      oops(QString("Bad pk file (%1), too many bits").arg(fontname));
  }
}

void font::read_PK_index(void)
{
#ifdef DEBUG_PK
  kdDebug() << "Reading PK pixel file " << filename << endl;
#endif

  Fseek(file, (long) one(file), 1);	/* skip comment */

  (void) four(file);		/* skip design size */
  long file_checksum = four(file);
  if (checksum && checksum && file_checksum != checksum)
    kdError(1) << "Checksum mismatch (dvi = " << checksum << "pk = " << file_checksum << 
      ") in font file " << filename << endl;

  int hppp = sfour(file);
  int vppp = sfour(file);
  if (hppp != vppp)
    kdDebug() << "Font has non-square aspect ratio " << vppp << ":" << hppp << endl;
  /*
   * Prepare glyph array.
   */
  glyphtable = (struct glyph *) xmalloc(max_num_of_chars_in_font * sizeof(struct glyph), "glyph array");
  bzero((char *) glyphtable, max_num_of_chars_in_font * sizeof(struct glyph));
  /*
   * Read glyph directory (really a whole pass over the file).
   */
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
    Fseek(file, (long) bytes_left, 1);
#ifdef DEBUG_PK
    kdDebug() << "Scanning pk char " << ch << "at " << glyphtable[ch].addr << endl;
#endif
  }
}
