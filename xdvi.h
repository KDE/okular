#ifndef _xdvi_h
#define _xdvi_h

/*
 *	Written by Eric C. Cooper, CMU
 */

/*
 *	pixel_conv is currently used only for converting absolute positions
 *	to pixel values; although normally it should be
 *		((int) ((x) / shrink_factor + (1 << 15) >> 16)),
 *	the rounding is achieved instead by moving the constant 1 << 15 to
 *	PAGE_OFFSET in dvi_draw.c.
 */

#define	pixel_conv(x)		((int) ((x) / (shrink_factor * 65536)))
#define	pixel_round(x)		((int) ROUNDUP(x, shrink_factor * 65536))
#define	spell_conv0(n, f)	((long) (n * f))
#define	spell_conv(n)		spell_conv0(n, dimconv)

#ifdef	BMBYTE
#define	BMUNIT			unsigned char
#define	BITS_PER_BMUNIT		8
#define	BYTES_PER_BMUNIT	1
#else	/* !BMBYTE */
#ifdef	BMSHORT
#define	BMUNIT			unsigned short
#define	BITS_PER_BMUNIT		16
#define	BYTES_PER_BMUNIT	2
#else	/* !BMSHORT */
#define	BMLONG
#ifdef	__alpha
#define	BMUNIT			unsigned int
#else
#define	BMUNIT			unsigned long
#endif	/* if __alpha */
#define	BITS_PER_BMUNIT		32
#define	BYTES_PER_BMUNIT	4
#endif	/* !BMSHORT */
#endif	/* !BMBYTE */

#define	ADD(a, b)	((BMUNIT *) (((char *) a) + b))
#define	SUB(a, b)	((BMUNIT *) (((char *) a) - b))

extern	BMUNIT	bit_masks[BITS_PER_BMUNIT + 1];


extern	struct drawinf	currinf;

/* entries below with the characters 'dvi' in them are actually stored in
   scaled pixel units */

#define DVI_H   currinf.data.dvi_h
#define PXL_H   pixel_conv(currinf.data.dvi_h)
#define DVI_V   currinf.data.dvi_v
#define PXL_V   currinf.data.pxl_v
#define WW      currinf.data.w
#define XX      currinf.data.x
#define YY      currinf.data.y
#define ZZ      currinf.data.z
#define ROUNDUP(x,y) (((x)+(y)-1)/(y))

extern QIntDict<struct font> tn_table;


/*
 *	Command line flags.
 */

extern	int	_pixels_per_inch;

extern  unsigned long   num (FILE *, int);
extern  long    snum(FILE *, int);


#define	pixels_per_inch	_pixels_per_inch

extern	struct WindowRec mane, currwin;

#define	shrink_factor	currwin.shrinkfactor

#define one(fp)		((unsigned char) getc(fp))
#define sone(fp)	((long) one(fp))
#define two(fp)		num (fp, 2)
#define stwo(fp)	snum(fp, 2)
#define four(fp)	num (fp, 4)
#define sfour(fp)	snum(fp, 4)

#endif /* _xdvi_h */
