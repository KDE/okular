#ifndef _xdvi_h
#define _xdvi_h
// Define the following flags to generate more debugging output
  

#include <setjmp.h>

/*
 *	Written by Eric C. Cooper, CMU
 */

/********************************
 *	The C environment	*
 *******************************/

/* For wchar_t et al., that the X files might want. */
extern "C" {
#include <sys/types.h>
}

/* See kpathsea/INSTALL for the purpose of the FOIL...  */
#ifdef FOIL_X_WCHAR_T
#define wchar_t foil_x_defining_wchar_t
#define X_WCHAR
#endif

#include <X11/Xlib.h>	/* include Xfuncs.h, if available */
#include <X11/Xutil.h>	/* needed for XDestroyImage */
#include <X11/Xos.h>
#include <X11/Intrinsic.h>
#undef wchar_t

typedef	unsigned long	Pixel;
typedef	char		Boolean;
#undef	BUTTONS
#undef	Unsorted

#define	True	1
#define	False	0

#define	OPEN_MODE_ARGS	_Xconst char *


#ifndef	NeedFunctionPrototypes
#ifdef	__STDC__
#define	NeedFunctionPrototypes	1
#else	/* STDC */
#define	NeedFunctionPrototypes	0
#endif	/* STDC */
#endif	/* NeedFunctionPrototypes */

#define	ARGS(x)	x

#ifndef	NeedWidePrototypes
#define	NeedWidePrototypes	NeedFunctionPrototypes
#endif

#ifndef	NeedVarargsPrototypes
#define	NeedVarargsPrototypes	NeedFunctionPrototypes
#endif

#ifndef	_XFUNCPROTOBEGIN
#define	_XFUNCPROTOBEGIN
#define	_XFUNCPROTOEND
#endif


#define	Printf	(void) printf
#define	Puts	(void) puts
#define	Fprintf	(void) fprintf
#define	Sprintf	(void) sprintf
#define	Fseek	(void) fseek
#define	Fread	(void) fread
#define	Fputs	(void) fputs
#define	Putc	(void) putc
#define	Putchar	(void) putchar
#define	Fclose	(void) fclose
#define	Fflush	(void) fflush
#define	Strcpy	(void) strcpy

/********************************
 *	 Types and data		*
 *******************************/

#define	INIT(x)


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

extern	int	current_page;
extern	int	total_pages;
extern	long	magnification;
extern	double	dimconv;
extern	double	tpic_conv;
extern	int	n_files_left;	/* for LRU closing of fonts */
extern	unsigned int	page_w, page_h;

#define	PS	1

/*
 * Table of page offsets in DVI file, indexed by page number - 1.
 * Initialized in prepare_pages().
 */
extern	long	*page_offset;



extern QIntDict<struct font> tn_table;
extern	struct font	*font_head	INIT(NULL);

extern	unsigned char		maxchar;
extern	unsigned short	current_timestamp INIT(0);

/*
 *	Command line flags.
 */

//extern	char	*debug_arg;
extern	int	_pixels_per_inch;
//extern	char	*sidemargin;
//extern	char	*topmargin;
extern	_Xconst char	*_paper;
extern	char	*mg_arg[5];

extern  unsigned long   num ARGS((FILE *, int));     
extern  long    snum ARGS((FILE *, int));


#define	pixels_per_inch	_pixels_per_inch



extern	struct	mg_size_rec {
	int	w;
	int	h;
}
	mg_size[5];

extern	int	_debug	INIT(0);

#define	DBG_BITMAP	0x1
#define	DBG_DVI		0x2
#define	DBG_PK		0x4
#define	DBG_BATCH	0x8
#define	DBG_EVENT	0x10	/* 16 */
#define	DBG_OPEN	0x20	/* 32 */
#define	DBG_PS		0x40	/* 64 */
#define	DBG_STAT	0x80	/* 128 */
#define	DBG_HASH	0x100	/* 256 */
#define	DBG_PATHS	0x200	/* 512 */
#define	DBG_EXPAND	0x400	/* 1024 */
#define	DBG_SEARCH	0x800	/* 2048 */
#define	DBG_ALL		(0xffff & ~DBG_BATCH)


extern	Display	*DISP;
extern	Screen	*SCRN;
extern	struct WindowRec mane, alt, currwin;

#define	shrink_factor	currwin.shrinkfactor

extern	jmp_buf	dvi_env;	/* mechanism to communicate dvi file errors */
extern	const char *dvi_oops_msg;	/* error message */


#define one(fp)		((unsigned char) getc(fp))
#define sone(fp)	((long) one(fp))
#define two(fp)		num (fp, 2)
#define stwo(fp)	snum(fp, 2)
#define four(fp)	num (fp, 4)
#define sfour(fp)	snum(fp, 4)

#endif /* _xdvi_h */
