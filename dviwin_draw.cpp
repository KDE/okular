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

#define DEBUG 0

/* The stuff from the path searching library.  */
#include "dviwin.h"

#include <kpathsea/config.h>
#include <kpathsea/c-ctype.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-vararg.h>
#include "glyph.h"
#include "oconfig.h"
#include "dvi.h"

#include <kdebug.h>
#include <qpainter.h>
#include <qbitmap.h> 
#include <qimage.h> 
#include <qkeycode.h>
#include <qpaintdevice.h>
#include <qfileinfo.h>

extern char *xmalloc (unsigned, _Xconst char *);
extern FILE *xfopen(char *filename, char *type);
extern Boolean check_dvi_file(void);

Boolean	_hush_chars;



static	struct frame	frame0;		/* dummy head of list */
static	struct frame	*scan_frame;	/* head frame for scanning */

#ifndef	DVI_BUFFER_LEN
#define	DVI_BUFFER_LEN	512
#endif

extern QPainter *dcp;
static	unsigned char	dvi_buffer[DVI_BUFFER_LEN];
static	struct frame	*current_frame;

#define	DIR	currinf.dir


/*
 *	Explanation of the following constant:
 *	offset_[xy]   << 16:	margin (defaults to one inch)
 *	shrink_factor << 16:	one pixel page border
 *	shrink_factor << 15:	rounding for pixel_conv
 */
#define OFFSET_X	(offset_x << 16) + ((int)shrink_factor * 3 << 15)
#define OFFSET_Y	(offset_y << 16) + ((int)shrink_factor * 3 << 15)

#ifndef	BMLONG
#ifndef	BMSHORT
BMUNIT	bit_masks[9] = {
	0x0,	0x1,	0x3,	0x7,
	0xf,	0x1f,	0x3f,	0x7f,
	0xff
};
#else	/* BMSHORT */
BMUNIT	bit_masks[17] = {
	0x0,	0x1,	0x3,	0x7,
	0xf,	0x1f,	0x3f,	0x7f,
	0xff,	0x1ff,	0x3ff,	0x7ff,
	0xfff,	0x1fff,	0x3fff,	0x7fff,
	0xffff
};
#endif	/* BMSHORT */
#else	/* BMLONG */
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
#endif	/* BMLONG */

#ifdef	VMS
#define	off_t	int
#endif
extern	off_t	lseek();

#ifndef	SEEK_SET	/* if <unistd.h> is not provided (or for <X11R5) */
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif

static	void	draw_part(struct frame *, double);



static void put_border(int x, int y, unsigned int width, unsigned int height)
{
	--width;
	--height;
	dcp->drawRect( x, y, width, height );
}


/*
 *	Byte reading routines for dvi file.
 */

#define	xtell(pos)	((long) (lseek(fileno(dvi_file), 0L, SEEK_CUR) - \
			    (currinf.end - (pos))))

static unsigned char xxone()
{
  if (currinf._virtual) {
    ++currinf.pos;
    return EOP;
  }
  currinf.end = dvi_buffer +
    read(fileno(dvi_file), (char *) (currinf.pos = dvi_buffer),
	 DVI_BUFFER_LEN);
  return currinf.end > dvi_buffer ? *(currinf.pos)++ : EOF;
}

#define	xone()  (currinf.pos < currinf.end ? *(currinf.pos)++ : xxone())

static unsigned long xnum(unsigned char size)
{
  register long x = 0;

  while (size--) x = (x << 8) | xone();
  return x;
}

static long xsnum(unsigned char size)
{
  register long x;

#ifdef	__STDC__
  x = (signed char) xone();
#else
  x = xone();
  if (x & 0x80) x -= 0x100;
#endif
  while (--size) x = (x << 8) | xone();
  return x;
}

#define	xsfour()	xsnum(4)

static	void xskip(long offset)
{
  currinf.pos += offset;
  if (!currinf._virtual && currinf.pos > currinf.end)
    (void) lseek(fileno(dvi_file), (long) (currinf.pos - currinf.end), SEEK_CUR);
}


static	NORETURN void tell_oops(_Xconst char *message, ...)
{
	va_list	args;

	kDebugError("%s: ", prog);
	va_start(args, message);
	(void) vfprintf(stderr, message, args);
	va_end(args);
	if (currinf._virtual)
	  kDebugError(" in virtual font %s", currinf._virtual->fontname);
	else
	  kDebugError(", offset %ld", xtell(currinf.pos - 1));
	dvi_oops_msg = (message), longjmp(dvi_env, 1); /* dvi_oops */
	exit(1);
}


static	_Xconst	char	*dvi_table2[] = {
	"FNT1", "FNT2", "FNT3", "FNT4", "XXX1", "XXX2", "XXX3", "XXX4",
	"FNTDEF1", "FNTDEF2", "FNTDEF3", "FNTDEF4", "PRE", "POST", "POSTPOST",
	"SREFL", "EREFL", NULL, NULL, NULL, NULL};


/*
 *	Find font #n.
 */

static void change_font(unsigned long n)
{
  currinf.fontp = currinf.fonttable[n];
  if (currinf.fontp == NULL)
    tell_oops("non-existent font #%d", n);
  currinf.set_char_p = currinf.fontp->set_char_p;
}



/** Routine to print characters.  */

void set_char(unsigned int cmd, unsigned int ch)
{
  kDebugInfo(DEBUG, 4300, "set_char");

  glyph *g = currinf.fontp->glyphptr(ch);
  if (g == NULL)
    return;

  long dvi_h_sav = DVI_H;
  if (currinf.dir < 0) 
    DVI_H -= g->dvi_adv;
  if (scan_frame == NULL)
    dcp->drawPixmap(PXL_H - g->x2 - currwin.base_x, PXL_V - g->y2 - currwin.base_y, g->shrunkCharacter());
  if (cmd == PUT1)
    DVI_H = dvi_h_sav;
  else
    if (currinf.dir > 0)
      DVI_H += g->dvi_adv;
}


static	void set_empty_char(unsigned int cmd, unsigned int ch)
{
  return;
}


void load_n_set_char(unsigned int cmd, unsigned int ch)
{
  kDebugInfo(DEBUG, 4300, "load_n_set_char");

  if ( currinf.fontp->load_font() ) {	/* if not found */
    kDebugError("Character(s) will be left blank.");
    currinf.set_char_p = currinf.fontp->set_char_p = set_empty_char;
    return;
  }
  currinf.set_char_p = currinf.fontp->set_char_p;
  (*currinf.set_char_p)(cmd, ch);
  return;
}


void set_vf_char(unsigned int cmd, unsigned int ch)
{
  kDebugInfo(DEBUG, 4300, "set_vf_char");

  macro *m;
  struct drawinf	 oldinfo;
  static unsigned char   c;
  long	                 dvi_h_sav;

  if ((m = &currinf.fontp->macrotable[ch])->pos == NULL) {
    if (!hush_chars)
      kDebugError(1, 4300, "Character %d not defined in font %s\n", ch, currinf.fontp->fontname);
    m->pos = m->end = &c;
    return;
  }

  dvi_h_sav = DVI_H;
  if (currinf.dir < 0)
    DVI_H -= m->dvi_adv;
  if (scan_frame == NULL) {
    oldinfo    = currinf;
    WW         = 0;
    XX         = 0;
    YY         = 0;
    ZZ         = 0;

    currinf.fonttable = currinf.fontp->vf_table;
    currinf.pos       = m->pos;
    currinf.end       = m->end;
    currinf._virtual  = currinf.fontp;
    draw_part(current_frame, currinf.fontp->dimconv);
    if (currinf.pos != currinf.end + 1)
      tell_oops("virtual character macro does not end correctly");
    currinf = oldinfo;
  }
  if (cmd == PUT1)
    DVI_H = dvi_h_sav;
  else
    if (currinf.dir > 0)
      DVI_H += m->dvi_adv;
}


static	void set_no_char(unsigned int cmd, unsigned int ch)
{
  kDebugInfo(DEBUG, 4300, "set_no_char");

  if (currinf._virtual) {
    currinf.fontp = currinf._virtual->first_font;
    if (currinf.fontp != NULL) {
      currinf.set_char_p = currinf.fontp->set_char_p;
      (*currinf.set_char_p)(cmd, ch);
      return;
    }
  }
  tell_oops("attempt to set character of unknown font");
  /* NOTREACHED */
}


/** Set rule. Arguments are coordinates of lower left corner.  */

static	void set_rule(int h, int w)
{
  dcp->fillRect( PXL_H - (currinf.dir < 0 ? w - 1 : 0)-currwin.base_x,
		 PXL_V - h + 1 -currwin.base_y, 
		 w?w:1, h?h:1, Qt::black );
}

static	void special(long nbytes)
{
  static	char	*cmd	= NULL;
  static	long	cmdlen	= -1;
  char	*p;

  if (cmdlen < nbytes) {
    if (cmd)
      free(cmd);
    cmd = xmalloc((unsigned) nbytes + 1, "special");
    cmdlen = nbytes;
  }
  p = cmd;
  for (;;) {
    int i = currinf.end - currinf.pos;

    if (i > nbytes) 
      i = nbytes;
    bcopy((char *) currinf.pos, p, i);
    currinf.pos += i;
    p += i;
    nbytes -= i;
    if (nbytes == 0) break;
    (void) xxone();
    --(currinf.pos);
  }
  *p = '\0';
  applicationDoSpecial(cmd);
}

#define	xspell_conv(n)	spell_conv0(n, current_dimconv)

static	void draw_part(struct frame *minframe, double current_dimconv)
{
  kDebugInfo(DEBUG, 4300, "draw_part");

  unsigned char  ch;
  struct drawinf oldinfo;
  off_t	         file_pos;
  int	         refl_count;

  currinf.fontp      = NULL;
  currinf.set_char_p = set_no_char;
  currinf.dir        = 1;
  scan_frame         = NULL;	/* indicates we're not scanning */

  for (;;) {
    ch = xone();
    if (ch <= (unsigned char) (SETCHAR0 + 127))
      (*currinf.set_char_p)(ch, ch);
    else
      if (FNTNUM0 <= ch && ch <= (unsigned char) (FNTNUM0 + 63))
	change_font((unsigned long) (ch - FNTNUM0));
      else {
	long a, b;
	
	switch (ch) {
	case SET1:
	case PUT1:
	  (*currinf.set_char_p)(ch, xone());
	  break;
	  
	case SETRULE:
	  /* Be careful, dvicopy outputs rules with
	     height = 0x80000000.  We don't want any
	     SIGFPE here. */
	  a = xsfour();
	  b = xspell_conv(xsfour());
	  if (a > 0 && b > 0 && scan_frame == NULL)
	    set_rule(pixel_round(xspell_conv(a)),
		     pixel_round(b));
	  DVI_H += DIR * b;
	  break;

	case PUTRULE:
	  a = xspell_conv(xsfour());
	  b = xspell_conv(xsfour());
	  if (a > 0 && b > 0 && scan_frame == NULL)
	    set_rule(pixel_round(a), pixel_round(b));
	  break;

	case NOP:
	  break;
	  
	case BOP:
	  xskip((long) 11 * 4);
	  DVI_H = OFFSET_X;
	  DVI_V = OFFSET_Y;
	  PXL_V = pixel_conv(DVI_V);
	  WW = XX = YY = ZZ = 0;
	  break;

	case EOP:
	  if (current_frame != minframe)
	    tell_oops("stack not empty at EOP");
	  psp.endpage();
	  return;

	case PUSH:
	  if (current_frame->next == NULL) {
	    struct frame *newp = (struct frame *)xmalloc(sizeof(struct frame), "stack frame");
	    current_frame->next = newp;
	    newp->prev = current_frame;
	    newp->next = NULL;
	  }
	  current_frame = current_frame->next;
	  current_frame->data = currinf.data;
	  break;

	case POP:
	  if (current_frame == minframe)
	    tell_oops("more POPs than PUSHes");
	  currinf.data = current_frame->data;
	  current_frame = current_frame->prev;
	  break;

	case SREFL:
	  if (scan_frame == NULL) {
	    /* we're not scanning:  save some info. */
	    oldinfo = currinf;
	    if (!currinf._virtual)
	      file_pos = xtell(currinf.pos);
	    scan_frame = current_frame; /* now we're scanning */
	    refl_count = 0;
	    break;
	  }
	  /* we are scanning */
	  if (current_frame == scan_frame) 
	    ++refl_count;
	  break;

	case EREFL:
	  if (scan_frame != NULL) {	/* if we're scanning */
	    if (current_frame == scan_frame && --refl_count < 0) {
	      /* we've hit the end of our scan */
	      scan_frame = NULL;
	      /* first:  push */
	      if (current_frame->next == NULL) {
		struct frame *newp = (struct frame *)xmalloc(sizeof(struct frame),"stack frame");
		current_frame->next = newp;
		newp->prev = current_frame;
		newp->next = NULL;
	      }
	      current_frame = current_frame->next;
	      current_frame->data = currinf.data;
	      /* next:  restore old file position, XX, etc. */
	      if (!currinf._virtual) {
		off_t bgn_pos = xtell(dvi_buffer);
		
		if (file_pos >= bgn_pos) {
		  oldinfo.pos = dvi_buffer + (file_pos - bgn_pos);
		  oldinfo.end = currinf.end;
		} else {
		  (void) lseek(fileno(dvi_file), file_pos, SEEK_SET);
		  oldinfo.pos = oldinfo.end;
		}
	      }
	      currinf = oldinfo;
	      /* and then:  recover position info. */
	      DVI_H = current_frame->data.dvi_h;
	      DVI_V = current_frame->data.dvi_v;
	      PXL_V = current_frame->data.pxl_v;
	      /* and finally, reverse direction */
	      currinf.dir = -currinf.dir;
	    }
	    break;
	  }
	  /* we're not scanning, */
	  /* so just reverse direction and then pop */
	  currinf.dir   = -currinf.dir;
	  currinf.data  = current_frame->data;
	  current_frame = current_frame->prev;
	  break;

	case RIGHT1:
	case RIGHT2:
	case RIGHT3:
	case RIGHT4:
	  DVI_H += DIR * xspell_conv(xsnum(ch - RIGHT1 + 1));
	  break;

	case W1:
	case W2:
	case W3:
	case W4:
	  WW = xspell_conv(xsnum(ch - W0));
	case W0:
	  DVI_H += DIR * WW;
	  break;
	  
	case X1:
	case X2:
	case X3:
	case X4:
	  XX = xspell_conv(xsnum(ch - X0));
	case X0:
	  DVI_H += DIR * XX;
	  break;

	case DOWN1:
	case DOWN2:
	case DOWN3:
	case DOWN4:
	  DVI_V += xspell_conv(xsnum(ch - DOWN1 + 1));
	  PXL_V = pixel_conv(DVI_V);
	  break;

	case Y1:
	case Y2:
	case Y3:
	case Y4:
	  YY = xspell_conv(xsnum(ch - Y0));
	case Y0:
	  DVI_V += YY;
	  PXL_V = pixel_conv(DVI_V);
	  break;

	case Z1:
	case Z2:
	case Z3:
	case Z4:
	  ZZ = xspell_conv(xsnum(ch - Z0));
	case Z0:
	  DVI_V += ZZ;
	  PXL_V = pixel_conv(DVI_V);
	  break;
	  
	case FNT1:
	case FNT2:
	case FNT3:
	case FNT4:
	  change_font(xnum(ch - FNT1 + 1));
	  break;

	case XXX1:
	case XXX2:
	case XXX3:
	case XXX4:
	  a = xnum(ch - XXX1 + 1);
	  if (a > 0)
	    special(a);
	  break;

	case FNTDEF1:
	case FNTDEF2:
	case FNTDEF3:
	case FNTDEF4:
	  xskip((long) (12 + ch - FNTDEF1 + 1));
	  xskip((long) xone() + (long) xone());
	  break;
	  
	case PRE:
	case POST:
	case POSTPOST:
	  tell_oops("shouldn't happen: %s encountered", dvi_table2[ch - (FNTNUM0 + 64)]);
	  break;
	  
	default:
	  tell_oops("unknown op-code %d", ch);
	} /* end switch*/
      } /* end else (ch not a SETCHAR or FNTNUM) */
  } /* end for */
}

#undef	xspell_conv

// The need_to_redraw is a dreadful hack to adress the following
// problem.  When the function initGS is called, in some circumstances
// part of the screen is erased. In older versions of kdvi the lead to
// errors in the output: part of the screen was blank. In xdvi, this
// problem was already solved: in case of neew the initGS would call
// longjump(canit_env) to ask the main program to draw the entire
// screen anew. We do the same here, but we avoid longjump. Instead,
// initGS sets need_to_redraw to 1 if it believes that it damaged part
// of the visible screen.
//
// I would be very happy if someone would come forward with a better
// solution.
//
// -- Stefan Kebekus.

int need_to_redraw;

void draw_page()
{
  kDebugInfo(DEBUG, 4300, "draw_page");

  need_to_redraw = 0;

  /* Check for changes in dvi file. */
  if (!check_dvi_file())
    return;

  put_border(-currwin.base_x, -currwin.base_y,
	     ROUNDUP(unshrunk_paper_w, shrink_factor) + 2,
	     ROUNDUP(unshrunk_paper_h, shrink_factor) + 2);

  (void) lseek(fileno(dvi_file), page_offset[current_page], SEEK_SET);

  bzero((char *) &currinf.data, sizeof(currinf.data));

  currinf.fonttable = tn_table;
  currinf.end       = dvi_buffer;
  currinf.pos       = dvi_buffer;
  currinf._virtual  = NULL;
  draw_part(current_frame = &frame0, dimconv);

  if (need_to_redraw != 0)
    draw_page();
}
