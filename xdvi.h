/*
 *	Written by Eric C. Cooper, CMU
 */

/********************************
 *	The C environment	*
 *******************************/

/* Avoid name clashes with kpathsea.  */
#define xmalloc xdvi_xmalloc
#define xfopen xdvi_xfopen

/* For wchar_t et al., that the X files might want. */
extern "C" {
#include <kpathsea/systypes.h>
}

/* See kpathsea/INSTALL for the purpose of the FOIL...  */
#ifdef FOIL_X_WCHAR_T
#define wchar_t foil_x_defining_wchar_t
#define X_WCHAR
#endif

#include <X11/Xlib.h>	/* include Xfuncs.h, if available */
#include <X11/Xutil.h>	/* needed for XDestroyImage */
#include <X11/Xos.h>
#undef wchar_t

#ifndef	XlibSpecificationRelease
#define	XlibSpecificationRelease 0
#endif

#if	XlibSpecificationRelease >= 5
#include <X11/Xfuncs.h>
#endif

#define	XtNumber(arr)	(sizeof(arr)/sizeof(arr[0]))
typedef	unsigned long	Pixel;
typedef	char		Boolean;
typedef	unsigned int	Dimension;
#undef	BUTTONS

#define	True	1
#define	False	0

#if	defined(_SVR4) || defined(_SVR4_SOURCE) || defined(_SYSTYPE_SVR4)
#undef	SVR4
#define	SVR4
#endif

#if	defined(SVR3) || defined(SVR4)
#undef	SYSV
#define	SYSV
#endif

#ifndef	BSD
#if	defined(SYSV) || defined(VMS) || defined(linux)
#define	BSD	0
#else
#define	BSD	1
#endif
#endif

#ifndef	OPEN_MODE
#define OPEN_MODE FOPEN_R_MODE
#endif	/* OPEN_MODE */

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

#ifndef	_Xconst
#ifdef	__STDC__
#define	_Xconst	const
#else	/* STDC */
#define	_Xconst
#endif	/* STDC */
#endif	/* _Xconst */

#ifndef	VOLATILE
#if	defined(__STDC__) || (defined(__stdc__) && defined(__convex__))
#define	VOLATILE	volatile
#else
#define	VOLATILE	/* nothing */
#endif
#endif

#ifndef	NORETURN
#ifdef	__GNUC__
#define	NORETURN	volatile
#else
#define	NORETURN	/* nothing */
#endif
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

#if	NeedWidePrototypes
typedef	int		wide_bool;
#define	WIDENINT	(int)
#else
typedef	Boolean		wide_bool;
#define	WIDENINT
#endif

/*
 *	pixel_conv is currently used only for converting absolute positions
 *	to pixel values; although normally it should be
 *		((int) ((x) / shrink_factor + (1 << 15) >> 16)),
 *	the rounding is achieved instead by moving the constant 1 << 15 to
 *	PAGE_OFFSET in dvi_draw.c.
 */
//@@@
//#define	pixel_conv(x)		((int) ((x) / (int)shrink_factor >> 16))
//#define	pixel_round(x)		((int) ROUNDUP(x, (int)shrink_factor << 16))
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


typedef	void	(*set_char_proc)(unsigned int, unsigned int);


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

/*
 * Mechanism for reducing repeated warning about specials, lost characters, etc.
 */
extern	Boolean	hush_spec_now;



#define	FONT_IN_USE	1	/* used for housekeeping */
#define	FONT_LOADED	2	/* if font file has been read */
#define	FONT_VIRTUAL	4	/* if font is virtual */

//#define	TNTABLELEN	30	/* length of TeXnumber array (dvi file) */
//#define	VFTABLELEN	5	/* length of TeXnumber array (virtual fonts) */

extern QIntDict<struct font> tn_table;
//@@@extern	struct font	*tn_table[TNTABLELEN];
//extern	struct tn	*tn_head	INIT(NULL);
extern	struct font	*font_head	INIT(NULL);

extern	unsigned char		maxchar;
extern	unsigned short	current_timestamp INIT(0);

/*
 *	Command line flags.
 */

extern	char	*debug_arg;
extern	int	_density;

extern	int	_pixels_per_inch;
extern	char	*sidemargin;
extern	char	*topmargin;
extern	char	*xoffset;
extern	char	*yoffset;
extern	_Xconst char	*_paper;
extern	Boolean	reverse;
extern	Boolean	_hush_spec;
extern	Boolean	_hush_chars;
extern	Boolean	_hush_chk;
extern	char	*fore_color;
extern	char	*back_color;
extern	char	*brdr_color;
extern	char	*high_color;
extern	char	*curs_color;
extern	Pixel	_fore_Pixel;
extern	Pixel	_back_Pixel;
extern	char	*icon_geometry;
extern	Boolean	keep_flag;
extern	Boolean	_postscript;
extern	Boolean	useGS;
extern	Boolean	version_flag;
extern	char	*mg_arg[5];

/* As a convenience, we define the field names without leading underscores
 * to point to the field of the above record.  Here are the global ones;
 * the local ones are defined in each module.  */


#define	density		_density
#define	pixels_per_inch	_pixels_per_inch
#define	hush_spec	_hush_spec
#define	hush_chars	_hush_chars
#define	hush_chk	_hush_chk

extern	Pixel		brdr_Pixel;

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

#ifndef	BDPI
#define	BDPI	300
#endif

extern	int		offset_x, offset_y;
extern	unsigned int	unshrunk_paper_w, unshrunk_paper_h;
extern	unsigned int	unshrunk_page_w, unshrunk_page_h;

extern	char	*dvi_name	INIT(NULL);
extern	FILE	*dvi_file;			/* user's file */
extern	char	*prog;
extern	double	bak_shrink;			/* last shrink factor != 1 */
extern	Dimension	window_w, window_h;
extern	XImage	*image;
extern	int	backing_store;
extern	int	home_x, home_y;
extern	Display	*DISP;
extern	Screen	*SCRN;



extern	Cursor	redraw_cursor, ready_cursor;


extern	Boolean	allow_can	INIT(True);

extern	struct WindowRec mane, alt, currwin;
extern	int		min_x, max_x, min_y, max_y;

#define	shrink_factor	currwin.shrinkfactor

extern	Window	top_level;

#define	BAR_WID		12	/* width of darkened area */
#define	BAR_THICK	15	/* gross amount removed */


extern	jmp_buf	dvi_env;	/* mechanism to communicate dvi file errors */
extern	const char *dvi_oops_msg;	/* error message */

extern	struct psprocs	{
	void	(*toggle) ARGS((void));
	void	(*destroy) ARGS((void));
	void	(*interrupt) ARGS((void));
	void	(*endpage) ARGS((void));
	void	(*drawbegin) ARGS((int, int, char *));
	void	(*drawraw) ARGS((char *));
	void	(*drawfile) ARGS((char *));
	void	(*drawend) ARGS((char *));
}	psp, no_ps_procs;

/********************************
 *	   Procedures		*
 *******************************/

_XFUNCPROTOBEGIN
extern	void	reconfig ARGS((void));
extern	void	read_events ARGS((wide_bool));
extern	void	redraw_page ARGS((void));
extern	void	do_pages ARGS((void));
extern	void	open_dvi_file ARGS((void));
extern	void	put_border ARGS((int, int, unsigned int, unsigned int));
extern	void	set_char ARGS((unsigned int, unsigned int));
extern	void	load_n_set_char ARGS((unsigned int, unsigned int));
extern	void	set_vf_char ARGS((unsigned int, unsigned int));
extern	void	init_font_open ARGS((void));
extern	void	applicationDoSpecial ARGS((char *));
extern	NORETURN void	oops(_Xconst char *, ...);
extern	void	alloc_bitmap ARGS((struct bitmap *));
extern	int	xpipe ARGS((int *));
extern	unsigned long	num ARGS((FILE *, int));
extern	long	snum ARGS((FILE *, int));
extern	void	read_PK_index ARGS((struct font *, wide_bool));
extern	void	read_GF_index ARGS((struct font *, wide_bool));
extern	void	read_VF_index ARGS((struct font *, wide_bool));


extern	void	drawbegin_none ARGS((int, int, char *));
extern	void	draw_bbox ARGS((void));
extern	void	NullProc ARGS((void));
extern	Boolean	initGS ARGS((void));


_XFUNCPROTOEND

#define one(fp)		((unsigned char) getc(fp))
#define sone(fp)	((long) one(fp))
#define two(fp)		num (fp, 2)
#define stwo(fp)	snum(fp, 2)
#define four(fp)	num (fp, 4)
#define sfour(fp)	snum(fp, 4)
