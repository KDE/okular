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

//#define DEBUG_RENDER 0

#include <stdlib.h>
#include <string.h>

#include "dviwin.h"
#include "glyph.h"
#include "oconfig.h"
#include "dvi.h"
#include "fontpool.h"

#include <kdebug.h>
#include <kprocess.h>
#include <qpainter.h>
#include <qbitmap.h> 
#include <qimage.h> 
#include <qpaintdevice.h>
#include <qfileinfo.h>

extern char *xmalloc (unsigned, const char *);
extern FILE *xfopen(const char *filename, char *type);

#ifndef	DVI_BUFFER_LEN
#define	DVI_BUFFER_LEN	512
#endif

extern QPainter foreGroundPaint;
unsigned char	dvi_buffer[DVI_BUFFER_LEN];

#define	DIR	currinf.dir



extern	off_t	lseek();


#ifndef	SEEK_SET	/* if <unistd.h> is not provided (or for <X11R5) */
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif



/*
 *	Byte reading routines for dvi file.
 */

#define	xtell(pos)	((long) (lseek(fileno(dviFile->file), 0L, SEEK_CUR) - (currinf.end - (pos))))

unsigned char dviWindow::xxone()
{
  if (currinf._virtual) {
    ++currinf.pos;
    return EOP;
  }
  currinf.end = dvi_buffer +
    read(fileno(dviFile->file), (char *) (currinf.pos = dvi_buffer),
	 DVI_BUFFER_LEN);
  return currinf.end > dvi_buffer ? *(currinf.pos)++ : EOF;
}

#define	xone()  (currinf.pos < currinf.end ? *(currinf.pos)++ : xxone())

unsigned long dviWindow::xnum(unsigned char size)
{
  register long x = 0;

  while (size--) x = (x << 8) | xone();
  return x;
}

long dviWindow::xsnum(unsigned char size)
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

void dviWindow::xskip(long offset)
{
  currinf.pos += offset;
  if (!currinf._virtual && currinf.pos > currinf.end)
    (void) lseek(fileno(dviFile->file), (long) (currinf.pos - currinf.end), SEEK_CUR);
}


static	void tell_oops(const QString & message)
{
  dvi_oops_msg = (message.utf8()), longjmp(dvi_env, 1); /* dvi_oops */
  exit(1);
}


static	const	char	*dvi_table2[] = {
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
    tell_oops(QString("non-existent font #%1").arg(n) );
  currinf.set_char_p = currinf.fontp->set_char_p;
}



/** Routine to print characters.  */

void dviWindow::set_char(unsigned int cmd, unsigned int ch)
{
#ifdef DEBUG_RENDER
  kdDebug() << "set_char" << endl;
#endif

  glyph *g = currinf.fontp->glyphptr(ch);
  if (g == NULL)
    return;

  long dvi_h_sav = DVI_H;
  if (currinf.dir < 0) 
    DVI_H -= g->dvi_adv;
  if (PostScriptOutPutString == NULL) {
    QPixmap pix = g->shrunkCharacter();
    int x = PXL_H - g->x2 - currwin.base_x;
    int y = PXL_V - g->y2 - currwin.base_y;

    // Draw the character.
    foreGroundPaint.drawPixmap(x, y, pix);
   
    // Are we drawing text for a hyperlink? And are hyperlinks
    // enabled?
    if (HTML_href != NULL && _showHyperLinks != 0) {
      // Now set up a rectangle which is checked against every mouse
      // event.
      if (word_boundary_encountered == true) {
	// Set up hyperlink
	hyperLinkList[num_of_used_hyperlinks].baseline = PXL_V;
	hyperLinkList[num_of_used_hyperlinks].box.setRect(x, y, pix.width(), pix.height());
	hyperLinkList[num_of_used_hyperlinks].linkText = *HTML_href;
	if (num_of_used_hyperlinks < MAX_HYPERLINKS-1)
	  num_of_used_hyperlinks++;
	else
	  kdError(4300) << "Used more than " << MAX_HYPERLINKS << " hyperlinks on a page. This is currently not supported." << endl;
      } else {
       	QRect dshunion = hyperLinkList[num_of_used_hyperlinks-1].box.unite(QRect(x, y, pix.width(), pix.height())) ;
	hyperLinkList[num_of_used_hyperlinks-1].box = dshunion;
      }
    }


    // Are we drawing text for a source hyperlink? And are source
    // hyperlinks enabled? @@@@
    if (source_href != NULL) {
      // Now set up a rectangle which is checked against every mouse
      // event.
      if (word_boundary_encountered == true) {
	// Set up source hyperlinks
	sourceHyperLinkList[num_of_used_source_hyperlinks].baseline = PXL_V;
	sourceHyperLinkList[num_of_used_source_hyperlinks].box.setRect(x, y, pix.width(), pix.height());
	sourceHyperLinkList[num_of_used_source_hyperlinks].linkText = *source_href;
	if (num_of_used_source_hyperlinks < MAX_HYPERLINKS-1)
	  num_of_used_source_hyperlinks++;
	else
	  kdError(4300) << "Used more than " << MAX_HYPERLINKS << " source-hyperlinks on a page. This is currently not supported." << endl;
      } else {

       	QRect dshunion = sourceHyperLinkList[num_of_used_source_hyperlinks-1].box.unite(QRect(x, y, pix.width(), pix.height())) ;
	sourceHyperLinkList[num_of_used_source_hyperlinks-1].box = dshunion;

      }
    }


  }
  if (cmd == PUT1)
    DVI_H = dvi_h_sav;
  else
    if (currinf.dir > 0)
      DVI_H += g->dvi_adv;

  word_boundary_encountered = false;
}

void dviWindow::set_empty_char(unsigned int, unsigned int)
{
  return;
}

void dviWindow::set_vf_char(unsigned int cmd, unsigned int ch)
{
#ifdef DEBUG_RENDER
  kdDebug() << "set_vf_char" << endl;
#endif

  macro *m;
  struct drawinf	 oldinfo;
  static unsigned char   c;
  long	                 dvi_h_sav;

  if ((m = &currinf.fontp->macrotable[ch])->pos == NULL) {
    kdError() << "Character " << ch << " not defined in font" << currinf.fontp->fontname << endl;
    m->pos = m->end = &c;
    return;
  }

  dvi_h_sav = DVI_H;
  if (currinf.dir < 0)
    DVI_H -= m->dvi_adv;
  if (PostScriptOutPutString == NULL) {
    oldinfo    = currinf;
    WW         = 0;
    XX         = 0;
    YY         = 0;
    ZZ         = 0;

    currinf.fonttable = currinf.fontp->vf_table;
    currinf.pos       = m->pos;
    currinf.end       = m->end;
    currinf._virtual  = currinf.fontp;
    draw_part(currinf.fontp->dimconv, true);
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


void dviWindow::set_no_char(unsigned int cmd, unsigned int ch)
{
#ifdef DEBUG_RENDER
  kdDebug() << "set_no_char" << endl;
#endif

  if (currinf._virtual) {
    currinf.fontp = currinf._virtual->first_font;
    if (currinf.fontp != NULL) {
      currinf.set_char_p = currinf.fontp->set_char_p;
      (this->*currinf.set_char_p)(cmd, ch);
      return;
    }
  }
  tell_oops("attempt to set character of unknown font");
  /* NOTREACHED */
}


/** Set rule. Arguments are coordinates of lower left corner.  */

static	void set_rule(int h, int w)
{
  foreGroundPaint.fillRect( PXL_H - (currinf.dir < 0 ? w - 1 : 0)-currwin.base_x,
			    PXL_V - h + 1 -currwin.base_y, 
			    w?w:1, h?h:1, Qt::black );
}

void dviWindow::special(long nbytes)
{
  char	*cmd	= NULL;
  long	cmdlen	= -1;
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
    memcpy(p, (char *) currinf.pos, i);
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

void dviWindow::draw_part(double current_dimconv, bool is_vfmacro)
{
#ifdef DEBUG_RENDER
  kdDebug() << "draw_part" << endl;
#endif

  unsigned char  ch;

  currinf.fontp        = NULL;
  currinf.set_char_p   = &dviWindow::set_no_char;
  currinf.dir          = 1;

  for (;;) {
    ch = xone();
    if (ch <= (unsigned char) (SETCHAR0 + 127))
      (this->*currinf.set_char_p)(ch, ch);
    else
      if (FNTNUM0 <= ch && ch <= (unsigned char) (FNTNUM0 + 63))
	change_font((unsigned long) (ch - FNTNUM0));
      else {
	long a, b;
	
	switch (ch) {
	case SET1:
	case PUT1:
	  (this->*currinf.set_char_p)(ch, xone());
	  break;
	  
	case SETRULE:
	  if (is_vfmacro == false)
	    word_boundary_encountered = true;
	  /* Be careful, dvicopy outputs rules with height =
	     0x80000000. We don't want any SIGFPE here. */
	  a = xsfour();
	  b = xspell_conv(xsfour());
	  if (a > 0 && b > 0 && PostScriptOutPutString == NULL)
	    set_rule(pixel_round(xspell_conv(a)), pixel_round(b));
	  DVI_H += DIR * b;
	  break;

	case PUTRULE:
	  if (is_vfmacro == false)
	    word_boundary_encountered = true;
	  a = xspell_conv(xsfour());
	  b = xspell_conv(xsfour());
	  if (a > 0 && b > 0 && PostScriptOutPutString == NULL)
	    set_rule(pixel_round(a), pixel_round(b));
	  break;

	case NOP:
	  break;
	  
	case BOP:
	  if (is_vfmacro == false)
	    word_boundary_encountered = true;
	  xskip((long) 11 * 4);
	  DVI_H = basedpi << 16; // Reminder: DVI-coordinates start at (1",1") from top of page
	  DVI_V = basedpi << 16;
	  PXL_V = pixel_conv(DVI_V);
	  WW = XX = YY = ZZ = 0;
	  break;

	case EOP:
	  // Check if we are just at the end of a virtual font macro.
	  if (is_vfmacro == false) {
	    // This is really the end of a page, and not just the end
	    // of a macro. Mark the end of the current word.
	    word_boundary_encountered = true;
	    // Sanity check for the dvi-file: The DVI-standard asserts
	    // that at the end of a page, the stack should always be
	    // empty.
	    if (!stack.isEmpty()) 
	      tell_oops("stack not empty at EOP");
	  }
	  return;

	case PUSH:
	  stack.push(currinf.data);
	  break;

	case POP:
	  if (stack.isEmpty())
	    tell_oops("more POPs than PUSHes");
	  else
	    currinf.data = stack.pop();
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
	  if (is_vfmacro == false)
	    word_boundary_encountered = true;
	  DVI_V += xspell_conv(xsnum(ch - DOWN1 + 1));
	  PXL_V = pixel_conv(DVI_V);
	  break;

	case Y1:
	case Y2:
	case Y3:
	case Y4:
	  YY = xspell_conv(xsnum(ch - Y0));
	case Y0:
	  word_boundary_encountered = true;
	  DVI_V += YY;
	  PXL_V = pixel_conv(DVI_V);
	  break;

	case Z1:
	case Z2:
	case Z3:
	case Z4:
	  ZZ = xspell_conv(xsnum(ch - Z0));
	case Z0:
	  if (is_vfmacro == false)
	    word_boundary_encountered = true;
	  DVI_V += ZZ;
	  PXL_V = pixel_conv(DVI_V);
	  break;
	  
	case FNT1:
	case FNT2:
	case FNT3:
	case FNT4:
	  if (is_vfmacro == false)
	    word_boundary_encountered = true;
	  change_font(xnum(ch - FNT1 + 1));
	  break;

	case XXX1:
	case XXX2:
	case XXX3:
	case XXX4:
	  if (is_vfmacro == false)
	    word_boundary_encountered = true;
	  a = xnum(ch - XXX1 + 1);
	  if (a > 0)
	    special(a);
	  break;

	case FNTDEF1:
	case FNTDEF2:
	case FNTDEF3:
	case FNTDEF4:
	  if (is_vfmacro == false)
	    word_boundary_encountered = true;
	  xskip((long) (12 + ch - FNTDEF1 + 1));
	  xskip((long) xone() + (long) xone());
	  break;
	  
	case PRE:
	case POST:
	case POSTPOST:
	  if (is_vfmacro == false)
	    word_boundary_encountered = true;
	  tell_oops(QString("shouldn't happen: %1 encountered").arg(dvi_table2[ch - (FNTNUM0 + 64)]));
	  break;
	  
	default:
	  if (is_vfmacro == false)
	    word_boundary_encountered = true;
	  tell_oops(QString("unknown op-code %1").arg(ch));
	} /* end switch*/
      } /* end else (ch not a SETCHAR or FNTNUM) */
  } /* end for */
}

#undef	xspell_conv

void dviWindow::draw_page(void)
{
  // Check if all the fonts are loaded. If that is not the case, we
  // return and do not draw anything. The font_pool will later emit
  // the signal "fonts_are_loaded" and thus trigger a redraw of the
  // page.
  if (font_pool->check_if_fonts_are_loaded() == -1)
    return;

#ifdef DEBUG_RENDER
  kdDebug() <<"draw_page" << endl;
#endif

  // Render the PostScript background, if there is one.
  foreGroundPaint.fillRect(pixmap->rect(), Qt::white );
  if (_postscript) {
    QPixmap *pxm = PS_interface->graphics(current_page);
    if (pxm != NULL) {
      foreGroundPaint.drawPixmap(0, 0, *pxm);
      delete pxm;
    }
  }
  
  // Now really write the text
  (void) lseek(fileno(dviFile->file), dviFile->page_offset[current_page], SEEK_SET);
  memset((char *) &currinf.data, 0, sizeof(currinf.data));
  currinf.fonttable      = tn_table;
  currinf.end            = dvi_buffer;
  currinf.pos            = dvi_buffer;
  currinf._virtual       = NULL;
  HTML_href              = NULL;
  num_of_used_hyperlinks = 0;
  num_of_used_source_hyperlinks = 0;
  draw_part(dviFile->dimconv, false);
  if (HTML_href != NULL) {
    delete HTML_href;
    HTML_href = NULL;
  }

  // Mark hyperlinks in blue. We draw a blue line under the
  // character whose width is equivalent to 0.5 mm, but at least
  // one pixel.
  int h = (int)(basedpi*0.05/(2.54*shrink_factor) + 0.5);
  h = (h < 1) ? 1 : h;
  for(int i=0; i<num_of_used_hyperlinks; i++) {
    int x = hyperLinkList[i].box.left();
    int w = hyperLinkList[i].box.width();
    int y = hyperLinkList[i].baseline;
    foreGroundPaint.fillRect(x, y, w, h, Qt::blue);
  }
}
