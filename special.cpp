
// special.cpp

// Methods for dviwin which deal with "\special" commands found in the
// DVI file

// Copyright 2000--2001, Stefan Kebekus (stefan.kebekus@uni-bayreuth.de).


#include <kdebug.h>
#include <klocale.h>
#include <kprocio.h>
#include <kstringhandler.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qstringlist.h>

#include "dviwin.h"
#include "kdvi.h"
#include "xdvi.h"


extern QPainter foreGroundPaint;

void dviWindow::color_special(QString cp)
{
  kdDebug(4300) << "Color special: " << cp << endl;
  
  // The color specials are ignore during the pre-scan phase, we use
  // them only during rendering
  if (PostScriptOutPutString == NULL) {
    
    cp = cp.stripWhiteSpace();
    
    QString command = KStringHandler::word(cp, (unsigned int)0);
    
    if (command == "pop") {
      // Take color off the stack
      return;
    }
    
    if (command == "push") {
      // Get color specification
      // Set color
      return;
    }
    
    // Get color specification
    // Set color for the rest of this page
    return;
  }
}

void dviWindow::html_anchor_special(QString cp)
{
  if (PostScriptOutPutString != NULL) { // only during scanning, not during rendering
    cp.truncate(cp.find('"'));
#ifdef DEBUG_SPECIAL
    kdDebug(4300) << "HTML-special, anchor " << cp.latin1() << endl;
    kdDebug(4300) << "page " << current_page << endl;
#endif
    
    anchorList[cp] = DVI_Anchor(current_page, currinf.data.dvi_v);
  }
}

void dviWindow::html_href_special(QString cp)
{
  cp.truncate(cp.find('"'));

#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "HTML-special, href " << cp.latin1() << endl;
#endif

  if (!PostScriptOutPutString) { // only when rendering really takes place
    HTML_href = new QString(cp);
  }
}

void dviWindow::html_anchor_end(void)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "HTML-special, anchor-end" << endl;
#endif

  if (HTML_href != NULL) {
    delete HTML_href;
    HTML_href = NULL;
  }
}

void dviWindow::header_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "PostScript-special, header " << cp.latin1() << endl;
#endif

  if (PostScriptOutPutString && QFile::exists(cp)) {
    PS_interface->PostScriptHeaderString->append( QString(" (%1) run\n").arg(cp) );
  }
}

void dviWindow::source_special(QString cp)
{
  if (PostScriptOutPutString == NULL) {  
    // only when rendering really takes place: set source_href to the
    // current special string. When characters are rendered, the
    // rendering routine will then generate a DVI_HyperLink and add it
    // to the proper list. This DVI_HyperLink is used to match mouse
    // positions with the hyperlinks for inverse search.
    if (source_href)
      *source_href = cp;
    else
      source_href = new QString(cp);
  } else { 
    // if no rendering takes place, i.e. when the DVI file is first
    // loaded, generate a DVI_SourceFileAnchor. These anchors are used
    // in forward search, i.e. to relate references line
    // "src:123file.tex" to positions in the DVI file

    // extract the file name and the numeral part from the string
    Q_UINT32 j;
    for(j=0;j<cp.length();j++)
      if (!cp.at(j).isNumber())
	break;
    Q_UINT32 sourceLineNumber = cp.left(j).toUInt();
    QString  sourceFileName   = QFileInfo(cp.mid(j).stripWhiteSpace()).absFilePath();
    DVI_SourceFileAnchor sfa(sourceFileName, sourceLineNumber, current_page, currinf.data.dvi_v);
    sourceHyperLinkAnchors.push_back(sfa);
  }
}

static void parse_special_argument(QString strg, const char *argument_name, int *variable)
{
  bool    OK;
  
  int index = strg.find(argument_name);
  if (index >= 0) {
    QString tmp     = strg.mid(index + strlen(argument_name));
    tmp.truncate(tmp.find(' '));
    float tmp_float = tmp.toFloat(&OK);
    if (OK)
      *variable = (int)(tmp_float+0.5);
    else
      // Maybe we should open a dialog here.
      kdError(4300) << i18n("Malformed parameter in the epsf special command.") << endl;
  }
}

void dviWindow::epsf_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "epsf-special: psfile=" << cp <<endl;
#endif

  QString include_command = cp.simplifyWhiteSpace();

  // The line is supposed to start with "..ile=", and then comes the
  // filename. Figure out what the filename is and stow it away. Of
  // course, this does not work if the filename contains spaces
  // (already the simplifyWhiteSpace() above is wrong). If you have
  // files like this, go away.
  QString EPSfilename = include_command;
  EPSfilename.truncate(EPSfilename.find(' '));

  // Strip enclosing quotation marks which are included by some LaTeX
  // macro packages (but not by others). This probably means that
  // graphic files are no longer found if the filename really does
  // contain quotes, but we don't really care that much.
  if ((EPSfilename.at(0) == '\"') && (EPSfilename.at(EPSfilename.length()-1) == '\"')) {
    EPSfilename = EPSfilename.mid(1,EPSfilename.length()-2);
  }

  // Now see if the Gfx file exists... try to find it in the current
  // directory, in the DVI file's directory, and finally, if all else
  // fails, use kpsewhich to find the file. Later on, we should
  // probably use the DVI file's baseURL, once this is implemented.
  if (! QFile::exists(EPSfilename)) {
    QFileInfo fi1(dviFile->filename);
    QFileInfo fi2(fi1.dir(),EPSfilename);
    if (fi2.exists())
      EPSfilename = fi2.absFilePath();
    else {
      // Use kpsewhich to find the eps file.
      KProcIO proc;
      proc << "kpsewhich" << EPSfilename;
      proc.start(KProcess::Block);
      proc.readln(EPSfilename);
      EPSfilename = EPSfilename.stripWhiteSpace();
    }
  }
  
  // Now parse the arguments. 
  int  llx     = 0; 
  int  lly     = 0;
  int  urx     = 0;
  int  ury     = 0;
  int  rwi     = 0;
  int  rhi     = 0;
  int  angle   = 0;

  // just to avoid ambiguities; the filename could contain keywords
  include_command = include_command.mid(include_command.find(' '));
  
  parse_special_argument(include_command, "llx=", &llx);
  parse_special_argument(include_command, "lly=", &lly);
  parse_special_argument(include_command, "urx=", &urx);
  parse_special_argument(include_command, "ury=", &ury);
  parse_special_argument(include_command, "rwi=", &rwi);
  parse_special_argument(include_command, "rhi=", &rhi);
  parse_special_argument(include_command, "angle=", &angle);

  if (PostScriptOutPutString) {
    if (QFile::exists(EPSfilename)) {
      double PS_H = (currinf.data.dvi_h*300.0)/(65536*MFResolutions[font_pool->getMetafontMode()])-300;
      double PS_V = (currinf.data.dvi_v*300.0)/MFResolutions[font_pool->getMetafontMode()] - 300;
      PostScriptOutPutString->append( QString(" %1 %2 moveto\n").arg(PS_H).arg(PS_V) );
      PostScriptOutPutString->append( "@beginspecial " );
      PostScriptOutPutString->append( QString(" %1 @llx").arg(llx) );
      PostScriptOutPutString->append( QString(" %1 @lly").arg(lly) );
      PostScriptOutPutString->append( QString(" %1 @urx").arg(urx) );
      PostScriptOutPutString->append( QString(" %1 @ury").arg(ury) );
      if (rwi != 0)
	PostScriptOutPutString->append( QString(" %1 @rwi").arg(rwi) );
      if (rhi != 0)
	PostScriptOutPutString->append( QString(" %1 @rhi").arg(rhi) );
      if (angle != 0)
	PostScriptOutPutString->append( QString(" %1 @angle").arg(angle) );
      PostScriptOutPutString->append( " @setspecial \n" );
      PostScriptOutPutString->append( QString(" (%1) run\n").arg(EPSfilename) );
      PostScriptOutPutString->append( "@endspecial \n" );
    }
  } else {
    if (!_postscript || !QFile::exists(EPSfilename)) {
      // Don't show PostScript, just draw the bounding box. For this,
      // calculate the size of the bounding box in Pixels. 
      double bbox_width  = urx - llx;
      double bbox_height = ury - lly;

      if ((rwi != 0)&&(bbox_width != 0)) {
       	bbox_height *= rwi/bbox_width;
	bbox_width  = rwi;
      }
      if ((rhi != 0)&&(bbox_height != 0)) {
	bbox_width  *= rhi/bbox_height;
	bbox_height = rhi;
      }

      bbox_width  *= 0.1 * 65536.0*fontPixelPerDVIunit() / shrinkfactor;
      bbox_height *= 0.1 * 65536.0*fontPixelPerDVIunit() / shrinkfactor;

      QRect bbox(((int) ((currinf.data.dvi_h) / (shrinkfactor * 65536))), currinf.data.pxl_v - (int)bbox_height, (int)bbox_width, (int)bbox_height);
      foreGroundPaint.save();
      if (QFile::exists(EPSfilename))
	foreGroundPaint.setBrush(Qt::lightGray);
      else
	foreGroundPaint.setBrush(Qt::red);
      foreGroundPaint.setPen(Qt::black);
      foreGroundPaint.drawRoundRect(bbox, 2, 2);
      if (QFile::exists(EPSfilename))
	foreGroundPaint.drawText (bbox, (int)(Qt::AlignCenter), EPSfilename, -1);
      else
	foreGroundPaint.drawText (bbox, (int)(Qt::AlignCenter), 
				  i18n("File not found: \n %1").arg(EPSfilename), -1);
      foreGroundPaint.restore();
    }
  }
  return;
}

void dviWindow::bang_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "PostScript-special, literal header " << cp.latin1() << endl;
#endif

  if (PostScriptOutPutString) {
    PS_interface->PostScriptHeaderString->append( " @defspecial \n" );
    PS_interface->PostScriptHeaderString->append( cp );
    PS_interface->PostScriptHeaderString->append( " @fedspecial \n" );
  }
}

void dviWindow::quote_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdError(4300) << "PostScript-special, literal PostScript " << cp.latin1() << endl;
#endif
  
  if (PostScriptOutPutString) {
    double PS_H = (currinf.data.dvi_h*300.0)/(65536*MFResolutions[font_pool->getMetafontMode()])-300;
    double PS_V = (currinf.data.dvi_v*300.0)/MFResolutions[font_pool->getMetafontMode()] - 300;
    PostScriptOutPutString->append( QString(" %1 %2 moveto\n").arg(PS_H).arg(PS_V) );
    PostScriptOutPutString->append( " @beginspecial @setspecial \n" );
    PostScriptOutPutString->append( cp );
    PostScriptOutPutString->append( " @endspecial \n" );
  }
}

void dviWindow::ps_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdError(4300) << "PostScript-special, direct PostScript " << cp.latin1() << endl;
#endif
  
  if (PostScriptOutPutString) {
    double PS_H = (currinf.data.dvi_h*300.0)/(65536*MFResolutions[font_pool->getMetafontMode()])-300;
    double PS_V = (currinf.data.dvi_v*300.0)/MFResolutions[font_pool->getMetafontMode()] - 300;
    
    if (cp.find("ps::[begin]", 0, false) == 0) {
      PostScriptOutPutString->append( QString(" %1 %2 moveto\n").arg(PS_H).arg(PS_V) );
      PostScriptOutPutString->append( QString(" %1\n").arg(cp.mid(11)) );
    } else {
      if (cp.find("ps::[end]", 0, false) == 0) {
	PostScriptOutPutString->append( QString(" %1\n").arg(cp.mid(9)) );
      } else {
	if (cp.find("ps::", 0, false) == 0) {
	  PostScriptOutPutString->append( QString(" %1\n").arg(cp.mid(4)) );
	} else {
	  PostScriptOutPutString->append( QString(" %1 %2 moveto\n").arg(PS_H).arg(PS_V) );
	  PostScriptOutPutString->append( QString(" %1\n").arg(cp.mid(3)) );
	}
      }
    }
  }
}

void dviWindow::applicationDoSpecial(char *cp)
{
  QString special_command(cp);

  // Encapsulated Postscript File
  if (special_command.find("src:", 0, false) == 0) {
    source_special(special_command.mid(4));
    return;
  }
  
  // Literal Postscript inclusion
  if (special_command[0] == '"') {
    quote_special(special_command.mid(1));
    return;
  }

  // PS-Postscript inclusion
  if (special_command.find("ps:", 0, false) == 0) {
    ps_special(special_command);
    return;
  }

  // Literal Postscript Header
  if (special_command[0] == '!') {
    bang_special(special_command.mid(1));
    return;
  }

  // Encapsulated Postscript File
  if (special_command.find("PSfile=", 0, false) == 0) {
    epsf_special(special_command.mid(7));
    return;
  }

  // Postscript Header File
  if (special_command.find("header=", 0, false) == 0) {
    header_special(special_command.mid(7));
    return;
  }

  // HTML reference
  if (special_command.find("html:<A href=", 0, false) == 0) {
    html_href_special(special_command.mid(14));
    return;
  }

  // HTML anchor end
  if (special_command.find("html:</A>", 0, false) == 0) {
    html_anchor_end();
    return;
  }

  // HTML anchor special
  if (special_command.find("html:<A name=", 0, false) == 0) {
    html_anchor_special(special_command.mid(14));
    return;
  }

  // color specials
  if (special_command.find("color", 0, false) == 0) {
    color_special(special_command.mid(5));
    return;
  }

  if (dviFile->errorCounter < 25) {
    kdError(4300) << i18n("The special command \"") << special_command << i18n("\" is not implemented.") << endl;
    dviFile->errorCounter++;
    if (dviFile->errorCounter == 25)
      kdError(4300) << i18n("That makes 25 errors. Further error messages will not be printed.") << endl;
  }

  return;
}
