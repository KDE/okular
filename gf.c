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

#include "oconfig.h"

/***
 ***	GF font reading routines.
 ***	Public routines are read_GF_index and read_GF_char.
 ***/

#define	PAINT_0		0
#define	PAINT1		64
#define	PAINT2		65
#define	PAINT3		66
#define	BOC		67
#define	BOC1		68
#define	EOC		69
#define	SKIP0		70
#define	SKIP1		71
#define	SKIP2		72
#define	SKIP3		73
#define	NEW_ROW_0	74
#define	NEW_ROW_MAX	238
#define	XXX1		239
#define	XXX2		240
#define	XXX3		241
#define	XXX4		242
#define	YYY		243
#define	NO_OP		244
#define	CHAR_LOC	245
#define	CHAR_LOC0	246
#define	PRE		247
#define	POST		248
#define	POST_POST	249

#define	GF_ID_BYTE	131
#define	TRAILER		223		/* Trailing bytes at end of file */

static	FILE	*GF_file;

static	void
expect(ch)
	ubyte ch;
{
	ubyte ch1 = one(GF_file);

	if (ch1 != ch)
		oops("Bad GF file:  %d expected, %d received.", ch, ch1);
}

static	void
too_many_bits(ch)
	ubyte ch;
{
	oops("Too many bits found when loading character %d", ch);
}

/*
 *	Public routines
 */


static	void
#if	NeedFunctionPrototypes
read_GF_char(register struct font *fontp, wide_ubyte ch)
#else	/* !NeedFunctionPrototypes */
read_GF_char(fontp, ch)
	register struct font *fontp;
	ubyte	ch;
#endif	/* NeedFunctionPrototypes */
{
	register struct glyph *g;
	ubyte	cmnd;
	int	min_m, max_m, min_n, max_n;
	BMUNIT	*cp, *basep, *maxp;
	int	bytes_wide;
	Boolean	paint_switch;
#define	White	False
#define	Black	True
	Boolean	new_row;
	int	count;
	int	word_weight;

	g = &fontp->glyph[ch];
	GF_file = fontp->file;

	if(debug & DBG_PK)
	    Printf("Loading gf char %d", ch);

	for (;;) {
	    switch (cmnd = one(GF_file)) {
		case XXX1:
		case XXX2:
		case XXX3:
		case XXX4:
		    Fseek(GF_file, (long) num(GF_file,
			WIDENINT cmnd - XXX1 + 1), 1);
		    continue;
		case YYY:
		    (void) four(GF_file);
		    continue;
		case BOC:
		    (void) four(GF_file);	/* skip character code */
		    (void) four(GF_file);	/* skip pointer to prev char */
		    min_m = sfour(GF_file);
		    max_m = sfour(GF_file);
		    g->x = -min_m;
		    min_n = sfour(GF_file);
		    g->y = max_n = sfour(GF_file);
		    g->bitmap.w = max_m - min_m + 1;
		    g->bitmap.h = max_n - min_n + 1;
		    break;
		case BOC1:
		    (void) one(GF_file);	/* skip character code */
		    g->bitmap.w = one(GF_file);	/* max_m - min_m */
		    g->x = g->bitmap.w - one(GF_file);	/* ditto - max_m */
		    ++g->bitmap.w;
		    g->bitmap.h = one(GF_file) + 1;
		    g->y = one(GF_file);
		    break;
		default:
		    oops("Bad BOC code:  %d", cmnd);
	    }
	    break;
	}
	paint_switch = White;

	if (debug & DBG_PK)
	    Printf(", size=%dx%d, dvi_adv=%ld\n", g->bitmap.w, g->bitmap.h,
		g->dvi_adv);

	alloc_bitmap(&g->bitmap);
	cp = basep = (BMUNIT *) g->bitmap.bits;
/*
 *	Read character data into *basep
 */
	bytes_wide = ROUNDUP((int) g->bitmap.w, BITS_PER_BMUNIT)
	    * BYTES_PER_BMUNIT;
	maxp = ADD(basep, g->bitmap.h * bytes_wide);
	bzero(g->bitmap.bits, g->bitmap.h * bytes_wide);
	new_row = False;
	word_weight = BITS_PER_BMUNIT;
	for (;;) {
	    count = -1;
	    cmnd = one(GF_file);
	    if (cmnd < 64) count = cmnd;
	    else if (cmnd >= NEW_ROW_0 && cmnd <= NEW_ROW_MAX) {
		count = cmnd - NEW_ROW_0;
		paint_switch = White;	/* it'll be complemented later */
		new_row = True;
	    }
	    else switch (cmnd) {
		case PAINT1:
		case PAINT2:
		case PAINT3:
		    count = num(GF_file, WIDENINT cmnd - PAINT1 + 1);
		    break;
		case EOC:
		    if (cp >= ADD(basep, bytes_wide)) too_many_bits(ch);
		    return;
		case SKIP1:
		case SKIP2:
		case SKIP3:
		    *((char **) &basep) +=
			num(GF_file, WIDENINT cmnd - SKIP0) * bytes_wide;
		case SKIP0:
		    new_row = True;
		    paint_switch = White;
		    break;
		case XXX1:
		case XXX2:
		case XXX3:
		case XXX4:
		    Fseek(GF_file, (long) num(GF_file,
			WIDENINT cmnd - XXX1 + 1), 1);
		    break;
		case YYY:
		    (void) four(GF_file);
		    break;
		case NO_OP:
		    break;
		default:
		    oops("Bad command in GF file:  %d", cmnd);
	    } /* end switch */
	    if (new_row) {
		*((char **) &basep) += bytes_wide;
		if (basep >= maxp || cp >= basep) too_many_bits(ch);
		cp = basep;
		word_weight = BITS_PER_BMUNIT;
		new_row = False;
	    }
	    if (count >= 0) {
		while (count)
		    if (count <= word_weight) {
#ifndef	MSBITFIRST
			if (paint_switch)
			    *cp |= bit_masks[count] <<
				(BITS_PER_BMUNIT - word_weight);
#endif
			word_weight -= count;
#ifdef	MSBITFIRST
			if (paint_switch)
			    *cp |= bit_masks[count] << word_weight;
#endif
			break;
		    }
		    else {
			if (paint_switch)
#ifndef	MSBITFIRST
			    *cp |= bit_masks[word_weight] <<
				(BITS_PER_BMUNIT - word_weight);
#else
			    *cp |= bit_masks[word_weight];
#endif
			cp++;
			count -= word_weight;
			word_weight = BITS_PER_BMUNIT;
		    }
		paint_switch = 1 - paint_switch;
	    }
	} /* end for */
}


void
read_GF_index(fontp, hushcs)
	register struct font	*fontp;
	wide_bool		hushcs;
{
	int	hppp, vppp;
	ubyte	ch, cmnd;
	register struct glyph *g;
	long	checksum;

	fontp->read_char = read_GF_char;
	GF_file = fontp->file;
	if (debug & DBG_PK)
	    Printf("Reading GF pixel file %s\n", fontp->filename);
/*
 *	Find postamble.
 */
	Fseek(GF_file, (long) -4, 2);
	while (four(GF_file) != ((unsigned long) TRAILER << 24 | TRAILER << 16
		| TRAILER << 8 | TRAILER))
	    Fseek(GF_file, (long) -5, 1);
	Fseek(GF_file, (long) -5, 1);
	for (;;) {
	    ch = one(GF_file);
	    if (ch != TRAILER) break;
	    Fseek(GF_file, (long) -2, 1);
	}
	if (ch != GF_ID_BYTE) oops("Bad end of font file %s", fontp->fontname);
	Fseek(GF_file, (long) -6, 1);
	expect(POST_POST);
	Fseek(GF_file, sfour(GF_file), 0);	/* move to postamble */
/*
 *	Read postamble.
 */
	expect(POST);
	(void) four(GF_file);		/* pointer to last eoc + 1 */
	(void) four(GF_file);		/* skip design size */
	checksum = four(GF_file);
	if (!hushcs && checksum && fontp->checksum
            && checksum != fontp->checksum)
	    Fprintf(stderr,
		"Checksum mismatch (dvi = %lu, gf = %lu) in font file %s\n",
		fontp->checksum, checksum, fontp->filename);
	hppp = sfour(GF_file);
	vppp = sfour(GF_file);
	if (hppp != vppp && (debug & DBG_PK))
	    Printf("Font has non-square aspect ratio %d:%d\n", vppp, hppp);
	(void) four(GF_file);		/* skip min_m */
	(void) four(GF_file);		/* skip max_m */
	(void) four(GF_file);		/* skip min_n */
	(void) four(GF_file);		/* skip max_n */
/*
 *	Prepare glyph array.
 */
	fontp->glyph = (struct glyph *) xmalloc(256 * sizeof(struct glyph),
	    "glyph array");
	bzero((char *) fontp->glyph, 256 * sizeof(struct glyph));
/*
 *	Read glyph directory.
 */
	while ((cmnd = one(GF_file)) != POST_POST) {
	    int addr;

	    ch = one(GF_file);			/* character code */
	    g = &fontp->glyph[ch];
	    switch (cmnd) {
		case CHAR_LOC:
		    /* g->pxl_adv = sfour(GF_file); */
		    (void) four(GF_file);
		    (void) four(GF_file);	/* skip dy */
		    break;
		case CHAR_LOC0:
		    /* g->pxl_adv = one(GF_file) << 16; */
		    (void) one(GF_file);
		    break;
		default:
		    oops("Non-char_loc command found in GF preamble:  %d",
			cmnd);
	    }
	    g->dvi_adv = fontp->dimconv * sfour(GF_file);
	    addr = four(GF_file);
	    if (addr != -1) g->addr = addr;
	    if (debug & DBG_PK)
		Printf("Read GF glyph for character %d; dy = %ld, addr = %x\n",
			ch, g->dvi_adv, addr);
	}
}
