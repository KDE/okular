
// special.cpp

// Methods for dviwin which deal with "\special" commands found in the
// DVI file

// Copyright 2000, Stefan Kebekus (stefan.kebekus@uni-bayreuth.de).


#include <qfile.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <kdebug.h>

#include "dviwin.h"

#include <stdio.h>
#include <stdlib.h>
#include "oconfig.h"


extern QPainter foreGroundPaint;
extern double   xres;

void dviWindow::html_anchor_special(QString cp)
{
  if (PostScriptOutPutString != NULL) { // only during scanning, not during rendering
    cp.truncate(cp.find('"'));
#ifdef DEBUG_SPECIAL
    kdDebug() << "HTML-special, anchor " << cp.latin1() << endl;
    kdDebug() << "page " << current_page << endl;
#endif
    
    AnchorList_String[numAnchors] = cp;
    AnchorList_Page[numAnchors]   = current_page;
    AnchorList_Vert[numAnchors]  = (DVI_V*xres)/(65536*basedpi); // multiply with zoom to get pixel coords
    if (numAnchors < MAX_ANCHORS-2)
      numAnchors++;
  }
}

void dviWindow::html_href_special(QString cp)
{
  cp.truncate(cp.find('"'));

#ifdef DEBUG_SPECIAL
  kdDebug() << "HTML-special, href " << cp.latin1() << endl;
#endif

  if (!PostScriptOutPutString) { // only when rendering really takes place
    HTML_href = new QString(cp);
  }
}

void dviWindow::html_anchor_end(void)
{
#ifdef DEBUG_SPECIAL
  kdDebug() << "HTML-special, anchor-end" << endl;
#endif

  if (HTML_href != NULL) {
    delete HTML_href;
    HTML_href = NULL;
  }
}


void dviWindow::header_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdDebug() << "PostScript-special, header " << cp.latin1() << endl;
#endif

  if (PostScriptOutPutString && QFile::exists(cp)) {
    PS_interface->PostScriptHeaderString->append( QString(" (%1) run\n").arg(cp) );
  }
}


static void parse_special_argument(QString strg, const char *argument_name, int *variable)
{
  bool    OK;
  
  int index = strg.find(argument_name);
  if (index >= 0) {
    QString tmp     = strg.mid(index + strlen(argument_name));
    tmp.truncate(tmp.find(' '));
    int tmp_int = tmp.toUInt(&OK);
    if (OK)
      *variable = tmp_int;
    else
      kdError() << "Malformed urx in special" << endl;
  }
}



void dviWindow::epsf_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdError() << "epsf-special: psfile=" << cp <<endl;
#endif

  QString include_command = cp.simplifyWhiteSpace();

  // The line is supposed to start with "..ile=", and then comes the
  // filename. Figure out what the filename is and stow it away
  QString EPSfilename = include_command;
  EPSfilename.truncate(EPSfilename.find(' '));
  if (! QFile::exists(EPSfilename)) {
    QFileInfo fi1(dviFile->filename);
    QFileInfo fi2(fi1.dir(),EPSfilename);
    if (fi2.exists())
      EPSfilename = fi2.absFilePath();
  }


  //
  // Now parse the arguments. 
  //
  int  llx     = 0; 
  int  lly     = 0;
  int  urx     = 0;
  int  ury     = 0;
  int  rwi     = 0;
  int  rhi     = 0;

  // just to avoid ambiguities; the filename could contain keywords
  include_command = include_command.mid(include_command.find(' '));
  
  parse_special_argument(include_command, "llx=", &llx);
  parse_special_argument(include_command, "lly=", &lly);
  parse_special_argument(include_command, "urx=", &urx);
  parse_special_argument(include_command, "ury=", &ury);
  parse_special_argument(include_command, "rwi=", &rwi);
  parse_special_argument(include_command, "rhi=", &rhi);

  if (PostScriptOutPutString) {
    if (QFile::exists(EPSfilename)) {
      PostScriptOutPutString->append( QString(" %1 %2 moveto\n").arg(DVI_H/65536-300).arg(DVI_V/65536-300) );
      PostScriptOutPutString->append( "@beginspecial " );
      PostScriptOutPutString->append( QString(" %1 @llx").arg(llx) );
      PostScriptOutPutString->append( QString(" %1 @lly").arg(lly) );
      PostScriptOutPutString->append( QString(" %1 @urx").arg(urx) );
      PostScriptOutPutString->append( QString(" %1 @ury").arg(ury) );
      if (rwi != 0)
	PostScriptOutPutString->append( QString(" %1 @rwi").arg(rwi) );
      if (rhi != 0)
	PostScriptOutPutString->append( QString(" %1 @rhi").arg(rwi) );
      PostScriptOutPutString->append( " @setspecial \n" );
      PostScriptOutPutString->append( QString(" (%1) run\n").arg(EPSfilename) );
      PostScriptOutPutString->append( "@endspecial \n" );
    }
  } else {
    if (!_postscript || !QFile::exists(EPSfilename)) {
      // Don't show PostScript, just draw the bounding box
      // For this, calculate the size of the bounding box in Pixels
      double bbox_width  = urx - llx;
      double bbox_height = lly - ury;

      if ((rwi != 0)&&(bbox_width != 0)) {
	bbox_height = bbox_height*rwi/bbox_width;
	bbox_width  = rwi;
      }
      if ((rhi != 0)&&(bbox_height != 0)) {
	bbox_height = rhi;
	bbox_width  = bbox_width*rhi/bbox_height;
      }

      bbox_width  *= 0.1 * dimconv / shrink_factor;
      bbox_height *= 0.1 * dimconv / shrink_factor;

      QRect bbox(PXL_H - currwin.base_x, PXL_V - currwin.base_y, (int)bbox_width, (int)bbox_height);
      if (QFile::exists(EPSfilename))
	foreGroundPaint.setBrush(Qt::lightGray);
      else
	foreGroundPaint.setBrush(Qt::red);
      foreGroundPaint.setPen(Qt::black);
      foreGroundPaint.drawRoundRect(bbox, 2, 2);
      if (QFile::exists(EPSfilename))
	foreGroundPaint.drawText (bbox, (int)(Qt::AlignCenter), EPSfilename, -1, &bbox);
      else
	foreGroundPaint.drawText (bbox, (int)(Qt::AlignCenter), 
				  QString("File not found:\n %1").arg(EPSfilename), -1, &bbox);
    }
  }
  return;
}

void dviWindow::bang_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdDebug() << "PostScript-special, literal header " << cp.latin1() << endl;
#endif

  if (currwin.win == mane.win && PostScriptOutPutString) {
    PS_interface->PostScriptHeaderString->append( " @defspecial \n" );
    PS_interface->PostScriptHeaderString->append( cp );
    PS_interface->PostScriptHeaderString->append( " @fedspecial \n" );
  }
}



void dviWindow::quote_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  //  kdError() << "PostScript-special, literal PostScript " << cp.latin1() << endl;
#endif
  
  if (currwin.win == mane.win && PostScriptOutPutString) {
    PostScriptOutPutString->append( QString(" %1 %2 moveto\n").arg(DVI_H/65536 - 300).arg(DVI_V/65536 - 300) );
    PostScriptOutPutString->append( " @beginspecial @setspecial \n" );
    PostScriptOutPutString->append( cp );
    PostScriptOutPutString->append( " @endspecial \n" );
  }
}


void dviWindow::applicationDoSpecial(char *cp)
{
  QString special_command(cp);
  
  // Literal Postscript inclusion
  if (special_command[0] == '"') {
    quote_special(special_command.mid(1));
    return;
  }

  // Literal Postscript Header
  if (special_command[0] == '!') {
    bang_special(special_command.mid(1));
    return;
  }

  // Encapsulated Postscript File
  if (special_command.find("PSfile=") == 0 || special_command.find("psfile=") == 0) {
    epsf_special(special_command.mid(7));
    return;
  }

  // Postscript Header File
  if (special_command.find("header=") == 0) {
    header_special(special_command.mid(7));
    return;
  }

  // HTML reference
  if (special_command.find("html:<A href=") == 0) {
    html_href_special(special_command.mid(14));
    return;
  }

  // HTML anchor end
  if (special_command.find("html:</A>") == 0) {
    html_anchor_end();
    return;
  }

  // HTML anchor special
  if (special_command.find("html:<A name=") == 0) {
    html_anchor_special(special_command.mid(14));
    return;
  }

  kdError() << "special \"" << cp << "\" not implemented" << endl;
  return;
}
