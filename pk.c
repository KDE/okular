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

#include "oconfig.h"

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

static	int
PK_get_nyb(fp)
	register FILE *fp;
{
	unsigned temp;
	if (PK_bitpos < 0) {
	    PK_input_byte = one(fp);
	    PK_bitpos = 4;
	}
	temp = PK_input_byte >> PK_bitpos;
	PK_bitpos -= 4;
	return (temp & 0xf);
}


static	int
PK_packed_num(fp)
	register FILE *fp;
{
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


static	void
PK_skip_specials(fontp)
	register struct font *fontp;
{
	int i,j;
	register FILE *fp = fontp->file;

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
			oops("Unexpected %d in PK file %s", PK_flag_byte,
			    fontp->filename);
			break;
		}
	    }
	}
	while (PK_flag_byte != PK_POST && PK_flag_byte >= PK_CMD_START);
}

/*
 *	Public routines
 */

static	void
#if	NeedFunctionPrototypes
read_PK_char(register struct font *fontp, wide_ubyte ch)
#else	/* !NeedFunctionPrototypes */
read_PK_char(fontp, ch)
	register struct font *fontp;
	ubyte	ch;
#endif	/* NeedFunctionPrototypes */
{
	int	i, j;
	int	n;
	int	row_bit_pos;
	Boolean	paint_switch;
	BMUNIT	*cp;
	register struct glyph *g;
	register FILE *fp = fontp->file;
	long	fpwidth;
	BMUNIT	word;
	int	word_weight, bytes_wide;
	int	rows_left, h_bit, count;

	g = &fontp->glyph[ch];
	PK_flag_byte = g->x2;
	PK_dyn_f = PK_flag_byte >> 4;
	paint_switch = ((PK_flag_byte & 8) != 0);
	PK_flag_byte &= 0x7;
	if (PK_flag_byte == 7) n = 4;
	else if (PK_flag_byte > 3) n = 2;
	else n = 1;

	if (debug & DBG_PK) Printf("loading pk char %d, char type %d ", ch, n);

	/*
	 * now read rest of character preamble
	 */
	if (n != 4) fpwidth = num(fp, 3);
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
		oops("Character %d too large in file %s", ch, fontp->fontname);
	    g->bitmap.w = w;
	    g->bitmap.h = h;
	}
	g->x = snum(fp, n);
	g->y = snum(fp, n);

	g->dvi_adv = fontp->dimconv * fpwidth;

	if (debug & DBG_PK) {
	    if (g->bitmap.w != 0)
		Printf(", size=%dx%d, dvi_adv=%ld", g->bitmap.w, g->bitmap.h,
		    g->dvi_adv);
	    Putchar('\n');
	}

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
	}
	else {		/* get packed raster */
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
		    }
		    else if (count >= h_bit && h_bit <= word_weight) {
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
		    }
		    else {
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
		oops("Wrong number of bits stored:  char. %d, font %s", ch,
		    fontp->fontname);
	    if (rows_left != 0 || h_bit != g->bitmap.w)
		oops("Bad pk file (%s), too many bits", fontp->fontname);
	}
}

void
read_PK_index(fontp, hushcs)
	register struct font	*fontp;
	wide_bool		hushcs;
{
	int	hppp, vppp;
	long	checksum;

	fontp->read_char = read_PK_char;
	if (debug & DBG_PK)
	    Printf("Reading PK pixel file %s\n", fontp->filename);

	Fseek(fontp->file, (long) one(fontp->file), 1);	/* skip comment */

	(void) four(fontp->file);		/* skip design size */
	checksum = four(fontp->file);
	if (!hushcs && checksum && fontp->checksum
	    && checksum != fontp->checksum)
	    Fprintf(stderr,
		"Checksum mismatch (dvi = %lu, pk = %lu) in font file %s\n",
		fontp->checksum, checksum, fontp->filename);
	hppp = sfour(fontp->file);
	vppp = sfour(fontp->file);
	if (hppp != vppp && (debug & DBG_PK))
	    Printf("Font has non-square aspect ratio %d:%d\n", vppp, hppp);
	/*
	 * Prepare glyph array.
	 */
	fontp->glyph = (struct glyph *) xmalloc(256 * sizeof(struct glyph),
	    "glyph array");
	bzero((char *) fontp->glyph, 256 * sizeof(struct glyph));
	/*
	 * Read glyph directory (really a whole pass over the file).
	 */
	for (;;) {
	    int bytes_left, flag_low_bits;
	    unsigned int ch;

	    PK_skip_specials(fontp);
	    if (PK_flag_byte == PK_POST) break;
	    flag_low_bits = PK_flag_byte & 0x7;
	    if (flag_low_bits == 7) {
		bytes_left = four(fontp->file);
		ch = four(fontp->file);
	    } else if (flag_low_bits > 3) {
		bytes_left = ((flag_low_bits - 4) << 16) + two(fontp->file);
		ch = one(fontp->file);
	    } else {
		bytes_left = (flag_low_bits << 8) + one(fontp->file);
		ch = one(fontp->file);
	    }
	    fontp->glyph[ch].addr = ftell(fontp->file);
	    fontp->glyph[ch].x2 = PK_flag_byte;
#ifdef linux
#ifndef SHORTSEEK
#define SHORTSEEK 1024
#endif
          /* A bug in Linux libc (as of 18oct94) makes a short read faster
             than a short forward seek. Totally non-intuitive.  */
          if (bytes_left > 0 && bytes_left < SHORTSEEK) {
            char *dummy = xmalloc (bytes_left, "shortseek");
            Fread (dummy, 1, bytes_left, fontp->file);
            free (dummy);
          } else
            /* seek backward, or long forward */
#endif /* linux */
	    Fseek(fontp->file, (long) bytes_left, 1);
	    if (debug & DBG_PK)
		Printf("Scanning pk char %u, at %ld.\n", ch,
		    fontp->glyph[ch].addr);
	}
}
