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
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <qpainter.h>
#include <qbitmap.h> 
#include <qimage.h> 
#include <qpaintdevice.h>
#include <qfileinfo.h>

extern QPainter foreGroundPaint;



/** Routine to print characters.  */

void dviWindow::set_char(unsigned int cmd, unsigned int ch)
{
#ifdef DEBUG_RENDER
  kdDebug() << "set_char #" << ch << endl;
#endif

  glyph *g = currinf.fontp->glyphptr(ch);
  if (g == NULL)
    return;

  long dvi_h_sav = DVI_H;
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
      if (line_boundary_encountered == true) {
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
    // hyperlinks enabled?
    if (source_href != 0) {
      // Now set up a rectangle which is checked against every mouse
      // event.
      if (line_boundary_encountered == true) {
	// Set up source hyperlinks
	sourceHyperLinkList[num_of_used_source_hyperlinks].baseline = PXL_V;
	sourceHyperLinkList[num_of_used_source_hyperlinks].box.setRect(x, y, pix.width(), pix.height());
	if (source_href != NULL) 
	  sourceHyperLinkList[num_of_used_source_hyperlinks].linkText = *source_href;
	else
	  sourceHyperLinkList[num_of_used_source_hyperlinks].linkText = "";
	if (num_of_used_source_hyperlinks < MAX_HYPERLINKS-1)
	  num_of_used_source_hyperlinks++;
	else
	  kdError(4300) << "Used more than " << MAX_HYPERLINKS << " source-hyperlinks on a page. This is currently not supported." << endl;
      } else {
	QRect dshunion = sourceHyperLinkList[num_of_used_source_hyperlinks-1].box.unite(QRect(x, y, pix.width(), pix.height())) ;
	sourceHyperLinkList[num_of_used_source_hyperlinks-1].box = dshunion;
      }
    }

    // Code for DVI -> text functions (e.g. marking of text, full text
    // search, etc.). Set up the textLinkList.
    if (line_boundary_encountered == true) {
      // Set up source hyperlinks
      DVI_Hyperlink link;
      link.baseline = PXL_V;
      link.box.setRect(x, y, pix.width(), pix.height());
      link.linkText = "";

      textLinkList.push_back(link);
    } else { // line boundary encountered
      QRect dshunion = textLinkList[textLinkList.size()-1].box.unite(QRect(x, y, pix.width(), pix.height())) ;
      textLinkList[textLinkList.size()-1].box = dshunion;
    }

    switch(ch) {
    case 0x0b:
      textLinkList[textLinkList.size()-1].linkText += "ff";
      break;
    case 0x0c:
      textLinkList[textLinkList.size()-1].linkText += "fi";
      break;
    case 0x0d:
      textLinkList[textLinkList.size()-1].linkText += "fl";
      break;
    case 0x0e:
      textLinkList[textLinkList.size()-1].linkText += "ffi";
      break;
    case 0x0f:
      textLinkList[textLinkList.size()-1].linkText += "ffl";
      break;

    case 0x7b:
      textLinkList[textLinkList.size()-1].linkText += "-";
      break;
    case 0x7c:
      textLinkList[textLinkList.size()-1].linkText += "---";
      break;
    case 0x7d:
      textLinkList[textLinkList.size()-1].linkText += "\"";
      break;
    case 0x7e:
      textLinkList[textLinkList.size()-1].linkText += "~";
      break;
    case 0x7f:
      textLinkList[textLinkList.size()-1].linkText += "@@"; // @@@ check!
      break;
      
    default:
      if ((ch >= 0x21) && (ch <= 0x7a))
	textLinkList[textLinkList.size()-1].linkText += QChar(ch);
      else
	textLinkList[textLinkList.size()-1].linkText += "?";
      break;
    }
  }

  if (cmd == PUT1)
    DVI_H = dvi_h_sav;
  else
    DVI_H += g->dvi_adv;

  word_boundary_encountered = false;
  line_boundary_encountered = false;
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

  static unsigned char   c;
  macro *m = &currinf.fontp->macrotable[ch];
  if (m->pos == NULL) {
    kdError() << "Character " << ch << " not defined in font" << currinf.fontp->fontname << endl;
    m->pos = m->end = &c;
    return;
  }

  long dvi_h_sav = DVI_H;
  if (PostScriptOutPutString == NULL) {
    struct drawinf oldinfo = currinf;
    WW         = 0;
    XX         = 0;
    YY         = 0;
    ZZ         = 0;

    currinf.fonttable         = currinf.fontp->vf_table;
    currinf._virtual          = currinf.fontp;
    Q_UINT8 *command_ptr_sav  = command_pointer;
    Q_UINT8 *end_ptr_sav      = end_pointer;
    command_pointer           = m->pos;
    end_pointer               = m->end;
    draw_part(currinf.fontp->dimconv, true);
    command_pointer           = command_ptr_sav;
    end_pointer               = end_ptr_sav;
    currinf = oldinfo;
  }
  if (cmd == PUT1)
    DVI_H = dvi_h_sav;
  else
    DVI_H += m->dvi_adv;
}


void dviWindow::set_no_char(unsigned int cmd, unsigned int ch)
{
#ifdef DEBUG_RENDER
  kdDebug() << "set_no_char: #" << ch <<endl;
#endif

  if (currinf._virtual) {
    currinf.fontp = currinf._virtual->first_font;
    if (currinf.fontp != NULL) {
      currinf.set_char_p = currinf.fontp->set_char_p;
      (this->*currinf.set_char_p)(cmd, ch);
      return;
    }
  }

  errorMsg = i18n("The DVI code set a character of unknown font");
  return;
  /* NOTREACHED */
}


/** Set rule. Arguments are coordinates of lower left corner.  */

static	void set_rule(int h, int w)
{
  foreGroundPaint.fillRect( PXL_H - -currwin.base_x, PXL_V - h + 1 -currwin.base_y, w?w:1, h?h:1, Qt::black );
}


#define	xspell_conv(n)	spell_conv0(n, current_dimconv)

void dviWindow::draw_part(double current_dimconv, bool is_vfmacro)
{
#ifdef DEBUG_RENDER
  kdDebug() << "draw_part" << endl;
#endif

  Q_INT32 RRtmp, WWtmp, XXtmp, YYtmp, ZZtmp;
  Q_UINT8 ch;

  currinf.fontp        = NULL;
  currinf.set_char_p   = &dviWindow::set_no_char;

  for (;;) {
    ch = readUINT8();
    if (ch <= (unsigned char) (SETCHAR0 + 127)) {
      (this->*currinf.set_char_p)(ch, ch);
    } else
      if (FNTNUM0 <= ch && ch <= (unsigned char) (FNTNUM0 + 63)) {
	currinf.fontp = currinf.fonttable[ch - FNTNUM0];
	if (currinf.fontp == NULL) {
	  errorMsg = i18n("The DVI code referred to font #%1, which was not previously defined.").arg(ch - FNTNUM0);
	  return;
	}
	currinf.set_char_p = currinf.fontp->set_char_p;
      } else {
	Q_INT32 a, b;
	
	switch (ch) {
	case SET1:
	case PUT1:
	  (this->*currinf.set_char_p)(ch, readUINT8());
	  break;
	  
	case SETRULE:
	  if (is_vfmacro == false) {
	    word_boundary_encountered = true;
	    line_boundary_encountered = true;
	  }
	  /* Be careful, dvicopy outputs rules with height =
	     0x80000000. We don't want any SIGFPE here. */
	  a = readUINT32();
	  b = readUINT32();
	  b = xspell_conv(b); 
	  if (a > 0 && b > 0 && PostScriptOutPutString == NULL)
	    set_rule(pixel_round(xspell_conv(a)), pixel_round(b));
	  DVI_H += b;
	  break;

	case PUTRULE:
	  if (is_vfmacro == false) {
	    word_boundary_encountered = true;
	    line_boundary_encountered = true;
	  }
	  a = readUINT32();
	  b = readUINT32();
	  a = xspell_conv(a);
	  b = xspell_conv(b);
	  if (a > 0 && b > 0 && PostScriptOutPutString == NULL)
	    set_rule(pixel_round(a), pixel_round(b));
	  break;

	case NOP:
	  break;
	  
	case BOP:
	  if (is_vfmacro == false) {
	    word_boundary_encountered = true;
	    line_boundary_encountered = true;
	  }
	  command_pointer += 11 * 4;
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
	    line_boundary_encountered = true;
	    // Sanity check for the dvi-file: The DVI-standard asserts
	    // that at the end of a page, the stack should always be
	    // empty.
	    if (!stack.isEmpty()) {
	      errorMsg = i18n("The stack was not empty when the EOP command was encountered.");
	      return;
	    }
	  }
	  return;

	case PUSH:
	  stack.push(currinf.data);
	  break;

	case POP:
	  if (stack.isEmpty()) {
	    errorMsg = i18n("The stack was empty when a POP command was encountered.");
	    return;
	  } else
	    currinf.data = stack.pop();
	  word_boundary_encountered = true;
	  line_boundary_encountered = true;
	  break;

	case RIGHT1:
	case RIGHT2:
	case RIGHT3:
	case RIGHT4:
	  RRtmp = readINT(ch - RIGHT1 + 1);

	  // A horizontal motion in the range 4 * font space [f] < h <
	  // font space [f] will be treated as a kern that is not
	  // indicated in the printouts that DVItype produces between
	  // brackets. We allow a larger space in the negative
	  // direction than in the positive one, because TEX makes
	  // comparatively large backspaces when it positions
	  // accents. (comments stolen from the source of dvitype)
	  if ((is_vfmacro == false) &&
	      (currinf.fontp != 0) &&
	      ((RRtmp >= currinf.fontp->scaled_size/6) || (RRtmp <= -4*(currinf.fontp->scaled_size/6))) && 
	      (textLinkList.size() > 0))
	    textLinkList[textLinkList.size()-1].linkText += ' ';
	  DVI_H += xspell_conv(RRtmp);
	  break;
	  
	case W1:
	case W2:
	case W3:
	case W4:
	  WWtmp = readINT(ch - W0);
	  WW = xspell_conv(WWtmp);
	case W0:
	  if ((is_vfmacro == false) && 
	      (currinf.fontp != 0) &&
	      ((WWtmp >= currinf.fontp->scaled_size/6) || (WWtmp <= -4*(currinf.fontp->scaled_size/6))) && 
	      (textLinkList.size() > 0) )
	    textLinkList[textLinkList.size()-1].linkText += ' ';
	  DVI_H += WW;
	  break;
	  
	case X1:
	case X2:
	case X3:
	case X4:
	  XXtmp = readINT(ch - X0);
	  XX = xspell_conv(XXtmp);
	case X0:
	  if ((is_vfmacro == false)  && 
	      (currinf.fontp != 0) &&
	      ((XXtmp >= currinf.fontp->scaled_size/6) || (XXtmp <= -4*(currinf.fontp->scaled_size/6))) && 
	      (textLinkList.size() > 0))
	    textLinkList[textLinkList.size()-1].linkText += ' ';
	  DVI_H += XX;
	  break;
	  
	case DOWN1:
	case DOWN2:
	case DOWN3:
	case DOWN4:
	  {
	    Q_INT32 DDtmp = readINT(ch - DOWN1 + 1);
	    if ((is_vfmacro == false) &&
		(currinf.fontp != 0) &&
		(abs(DDtmp) >= 5*(currinf.fontp->scaled_size/6)) && 
		(textLinkList.size() > 0)) {
	      word_boundary_encountered = true;
	      line_boundary_encountered = true;
	      if (abs(DDtmp) >= 10*(currinf.fontp->scaled_size/6)) 
		textLinkList[textLinkList.size()-1].linkText += '\n';
	    }
	    DVI_V += xspell_conv(DDtmp);
	    PXL_V = pixel_conv(DVI_V);
	  }
	  break;
	  
	case Y1:
	case Y2:
	case Y3:
	case Y4:
	  YYtmp = readINT(ch - Y0);
	  YY    = xspell_conv(YYtmp);
	case Y0:
	  if ((is_vfmacro == false) &&
	      (currinf.fontp != 0) &&
	      (abs(YYtmp) >= 5*(currinf.fontp->scaled_size/6)) && 
	      (textLinkList.size() > 0)) {
	    word_boundary_encountered = true;
	    line_boundary_encountered = true;
	    if (abs(YYtmp) >= 10*(currinf.fontp->scaled_size/6)) 
	      textLinkList[textLinkList.size()-1].linkText += '\n';
	  }
	  DVI_V += YY;
	  PXL_V = pixel_conv(DVI_V);
	  break;
	  
	case Z1:
	case Z2:
	case Z3:
	case Z4:
	  ZZtmp = readINT(ch - Z0);
	  ZZ    = xspell_conv(ZZtmp);
	case Z0:
	  if ((is_vfmacro == false) &&
	      (currinf.fontp != 0) &&
	      (abs(ZZtmp) >= 5*(currinf.fontp->scaled_size/6)) && 
	      (textLinkList.size() > 0)) {
	    word_boundary_encountered = true;
	    line_boundary_encountered = true;
	    if (abs(ZZtmp) >= 10*(currinf.fontp->scaled_size/6)) 
	      textLinkList[textLinkList.size()-1].linkText += '\n';
	  }
	  DVI_V += ZZ;
	  PXL_V = pixel_conv(DVI_V);
	  break;
	  
	case FNT1:
	case FNT2:
	case FNT3:
	  currinf.fontp = currinf.fonttable[readUINT(ch - FNT1 + 1)];
	  if (currinf.fontp == NULL) {
	    errorMsg = i18n("The DVI code referred to a font which was not previously defined.");
	    return;
	  }
	  currinf.set_char_p = currinf.fontp->set_char_p;
	  break;

	case FNT4:
	  currinf.fontp = currinf.fonttable[readINT(ch - FNT1 + 1)];
	  if (currinf.fontp == NULL) {
	    errorMsg = i18n("The DVI code referred to a font which was not previously defined.");
	    return;
	  }
	  currinf.set_char_p = currinf.fontp->set_char_p;
	  break;

	case XXX1:
	case XXX2:
	case XXX3:
	case XXX4:
	  if (is_vfmacro == false) {
	    word_boundary_encountered = true;
	    line_boundary_encountered = true;
	  }
	  a = readUINT(ch - XXX1 + 1);
	  if (a > 0) {
	    char	*cmd	= new char[a+1];
	    strncpy(cmd, (char *)command_pointer, a);
	    command_pointer += a;
	    cmd[a] = '\0';
	    applicationDoSpecial(cmd);
	    delete [] cmd;
	  }
	  break;

	case FNTDEF1:
	case FNTDEF2:
	case FNTDEF3:
	case FNTDEF4:
	  command_pointer += 12 + ch - FNTDEF1 + 1;
	  command_pointer += readUINT8() + readUINT8();
	  break;
	  
	case PRE:
	case POST:
	case POSTPOST:
	  errorMsg = i18n("An illegal command was encountered.");
	  return;
	  break;
	  
	default:
	  errorMsg = i18n("The unknown op-code %1 was encountered.").arg(ch);
	  return;
	} /* end switch*/
      } /* end else (ch not a SETCHAR or FNTNUM) */
  } /* end for */
}

#undef	xspell_conv

void dviWindow::draw_page(void)
{
  // Reset a couple of variables
  HTML_href              = 0;
  source_href            = 0;
  num_of_used_hyperlinks = 0;
  textLinkList.clear();
  num_of_used_source_hyperlinks = 0;

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
  if (dviFile->page_offset == 0)
    return;
  if (current_page < dviFile->total_pages) {
    command_pointer = dviFile->dvi_Data + dviFile->page_offset[current_page];
    end_pointer     = dviFile->dvi_Data + dviFile->page_offset[current_page+1];
  } else
    command_pointer = end_pointer = 0;

  memset((char *) &currinf.data, 0, sizeof(currinf.data));
  currinf.fonttable      = tn_table;
  currinf._virtual       = 0;
  draw_part(dviFile->dimconv, false);
  if (HTML_href != 0) {
    delete HTML_href;
    HTML_href = 0;
  }
  if (source_href != 0) {
    delete source_href;
    source_href = 0;
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
