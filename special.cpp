
// special.cpp

// Methods for dviwin which deal with "\special" commands found in the
// DVI file

// Copyright 2000--2003, Stefan Kebekus (kebekus@kde.org).


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
#include "kdvi_multipage.h"
#include "xdvi.h"

//#define DEBUG_SPECIAL

extern QPainter foreGroundPaint;

void dviWindow::printErrorMsgForSpecials(QString msg)
{
  if (dviFile->errorCounter < 25) {
    kdError(4300) << msg << endl;
    dviFile->errorCounter++;
    if (dviFile->errorCounter == 25)
      kdError(4300) << i18n("That makes 25 errors. Further error messages will not be printed.") << endl;
  }
}

// Parses a color specification, as explained in the manual to
// dvips. If the spec could not be parsed, an invalid color will be
// returned.

QColor parseColorSpecification(QString colorSpec)
{
  QString specType = KStringHandler::word(colorSpec, (unsigned int)0);

  if (specType.find("rgb", false) == 0) {
    bool ok;

    double r = KStringHandler::word(colorSpec, (unsigned int)1).toDouble(&ok);
    if ((ok == false) || (r < 0.0) || (r > 1.0))
      return QColor();
    
    double g = KStringHandler::word(colorSpec, (unsigned int)2).toDouble(&ok);
    if ((ok == false) || (g < 0.0) || (g > 1.0))
      return QColor();
    
    double b = KStringHandler::word(colorSpec, (unsigned int)3).toDouble(&ok);
    if ((ok == false) || (b < 0.0) || (b > 1.0))
      return QColor();

    return QColor((int)(r*255.0+0.5), (int)(g*255.0+0.5), (int)(b*255.0+0.5));
  }

  if (specType.find("hsb", false) == 0) {
    bool ok;

    double h = KStringHandler::word(colorSpec, (unsigned int)1).toDouble(&ok);
    if ((ok == false) || (h < 0.0) || (h > 1.0))
      return QColor();
    
    double s = KStringHandler::word(colorSpec, (unsigned int)2).toDouble(&ok);
    if ((ok == false) || (s < 0.0) || (s > 1.0))
      return QColor();
    
    double b = KStringHandler::word(colorSpec, (unsigned int)3).toDouble(&ok);
    if ((ok == false) || (b < 0.0) || (b > 1.0))
      return QColor();

    return QColor((int)(h*359.0+0.5), (int)(s*255.0+0.5), (int)(b*255.0+0.5), QColor::Hsv);
  }

  if (specType.find("cmyk", false) == 0) {
    bool ok;

    double c = KStringHandler::word(colorSpec, (unsigned int)1).toDouble(&ok);
    if ((ok == false) || (c < 0.0) || (c > 1.0))
      return QColor();
    
    double m = KStringHandler::word(colorSpec, (unsigned int)2).toDouble(&ok);
    if ((ok == false) || (m < 0.0) || (m > 1.0))
      return QColor();
    
    double y = KStringHandler::word(colorSpec, (unsigned int)3).toDouble(&ok);
    if ((ok == false) || (y < 0.0) || (y > 1.0))
      return QColor();
    
    double k = KStringHandler::word(colorSpec, (unsigned int)3).toDouble(&ok);
    if ((ok == false) || (k < 0.0) || (k > 1.0))
      return QColor();

    // Convert cmyk coordinates to rgb.
    double r = 1.0 - c - k;
    if (r < 0.0)
      r = 0.0;
    double g = 1.0 - m - k;
    if (g < 0.0)
      g = 0.0;
    double b = 1.0 - y - k;
    if (b < 0.0)
      b = 0.0;

    return QColor((int)(r*255.0+0.5), (int)(g*255.0+0.5), (int)(b*255.0+0.5));
  }

  if (specType.find("gray", false) == 0) {
    bool ok;

    double g = KStringHandler::word(colorSpec, (unsigned int)1).toDouble(&ok);
    if ((ok == false) || (g < 0.0) || (g > 1.0))
      return QColor();
    
    return QColor((int)(g*255.0+0.5), (int)(g*255.0+0.5), (int)(g*255.0+0.5));
  }

  return QColor(specType);
}






void dviWindow::color_special(QString cp)
{
  cp = cp.stripWhiteSpace();
  
  QString command = KStringHandler::word(cp, (unsigned int)0);
  
  if (command == "pop") {
    // Take color off the stack
    if (colorStack.isEmpty())
      printErrorMsgForSpecials( i18n("Error in DVIfile '%1', page %2. Color pop command issued when the color stack is empty." ).
				arg(dviFile->filename).arg(current_page));
    else
      colorStack.pop();
    return;
  }
  
  if (command == "push") {
    // Get color specification
    QColor col = parseColorSpecification(KStringHandler::word(cp, "1:"));
    // Set color
    if (col.isValid()) 
      colorStack.push(col); 
    else
      colorStack.push(Qt::black); 
    return;
  }
  
  // Get color specification and set the color for the rest of this
  // page
  QColor col = parseColorSpecification(cp);
  // Set color
  if (col.isValid()) 
    globalColor = col;
  else
    globalColor = Qt::black;
  return;
}


void dviWindow::html_href_special(QString cp)
{
  cp.truncate(cp.find('"'));
  
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "HTML-special, href " << cp.latin1() << endl;
#endif
  HTML_href = new QString(cp);
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


void dviWindow::source_special(QString cp)
{
  // only when rendering really takes place: set source_href to the
  // current special string. When characters are rendered, the
  // rendering routine will then generate a DVI_HyperLink and add it
  // to the proper list. This DVI_HyperLink is used to match mouse
  // positions with the hyperlinks for inverse search.
  if (source_href)
    *source_href = cp;
  else
    source_href = new QString(cp);
}


void parse_special_argument(QString strg, const char *argument_name, int *variable)
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
  EPSfilename = ghostscript_interface::locateEPSfile(EPSfilename, dviFile);
  
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

    double fontPixelPerDVIunit = dviFile->getCmPerDVIunit() * 
      MFResolutions[font_pool.getMetafontMode()]/2.54;
    
    bbox_width  *= 0.1 * 65536.0*fontPixelPerDVIunit / shrinkfactor;
    bbox_height *= 0.1 * 65536.0*fontPixelPerDVIunit / shrinkfactor;
    
    QRect bbox(((int) ((currinf.data.dvi_h) / (shrinkfactor * 65536))), currinf.data.pxl_v - (int)bbox_height,
	       (int)bbox_width, (int)bbox_height);
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
  
  return;
}


void dviWindow::TPIC_flushPath_special(void)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "TPIC special flushPath" << endl;
#endif

  if (number_of_elements_in_path == 0) {
    printErrorMsgForSpecials("TPIC special flushPath called when path was empty.");
    return;
  }

  QPen pen(Qt::black, (int)(penWidth_in_mInch*xres*_zoom/1000.0 + 0.5));  // Sets the pen size in milli-inches
  foreGroundPaint.setPen(pen);
  foreGroundPaint.drawPolyline(TPIC_path, 0, number_of_elements_in_path);
  number_of_elements_in_path = 0;
}


void dviWindow::TPIC_addPath_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "TPIC special addPath: " << cp << endl;
#endif

  // Adds a point to the path list
  QString cp_noWhiteSpace = cp.stripWhiteSpace();
  bool ok;
  float xKoord = KStringHandler::word(cp_noWhiteSpace, (uint)0).toFloat(&ok);
  if (ok == false) {
    printErrorMsgForSpecials( QString("TPIC special; cannot parse first argument in 'pn %1'.").arg(cp) );
    return;
  }
  float yKoord = KStringHandler::word(cp_noWhiteSpace, (uint)1).toFloat(&ok);
  if (ok == false) {
    printErrorMsgForSpecials( QString("TPIC special; cannot parse second argument in 'pn %1'.").arg(cp) );
    return;
  }
  
  int x = (int)( currinf.data.dvi_h/(shrinkfactor*65536.0) + xKoord*xres*_zoom/1000.0 + 0.5 );
  int y = (int)( currinf.data.pxl_v + yKoord*xres*_zoom/1000.0 + 0.5 );
  
  // Initialize the point array used to store the path
  if (TPIC_path.size() == 0) 
    number_of_elements_in_path = 0;
  if (TPIC_path.size() == number_of_elements_in_path) 
    TPIC_path.resize(number_of_elements_in_path+100);
  TPIC_path.setPoint(number_of_elements_in_path++, x, y);
}


void dviWindow::TPIC_setPen_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "TPIC special setPen: " << cp << endl;
#endif
  
  // Sets the pen size in milli-inches
  bool ok;
  penWidth_in_mInch = cp.stripWhiteSpace().toFloat(&ok);
  if (ok == false) {
    printErrorMsgForSpecials( QString("TPIC special; cannot parse argument in 'pn %1'.").arg(cp) );
    penWidth_in_mInch = 0.0;
    return;
  }
}


void dviWindow::applicationDoSpecial(char *cp)
{
  QString special_command(cp);

  // First come specials which is only interpreted during rendering,
  // and NOT during the prescan phase

  // font color specials
  if (strncasecmp(cp, "color", 5) == 0) {
    color_special(special_command.mid(5));
    return;
  }
  
  // HTML reference
  if (strncasecmp(cp, "html:<A href=", 13) == 0) {
    html_href_special(special_command.mid(14));
    return;
  }
  
  // HTML anchor end
  if (strncasecmp(cp, "html:</A>", 9) == 0) {
    html_anchor_end();
    return;
  }

  // TPIC specials
  if (strncasecmp(cp, "pn", 2) == 0) {
    TPIC_setPen_special(special_command.mid(2));
    return;
  }
  if (strncasecmp(cp, "pa ", 3) == 0) {
    TPIC_addPath_special(special_command.mid(3));
    return;
  }
  if (strncasecmp(cp, "fp", 2) == 0) {
    TPIC_flushPath_special();
    return;
  }

  // Encapsulated Postscript File
  if (strncasecmp(cp, "PSfile=", 7) == 0) {
    epsf_special(special_command.mid(7));
    return;
  }
  
  // source special
  if (strncasecmp(cp, "src:", 4) == 0) {
    source_special(special_command.mid(4));
    return;
  }

  // Unfortunately, in some TeX distribution the hyperref package uses
  // the dvips driver by default, rather than the hypertex driver. As
  // a result, the DVI files produced are full of PostScript that
  // specifies links and anchors, and KDVI would call the ghostscript
  // interpreter for every page which makes it really slow. This is a
  // major nuisance, so that we try to filter and interpret the
  // hypertex generated PostScript here.
  if (special_command.startsWith("ps:SDict begin")) {

    // Hyperref: start of hyperref rectangle. At this stage it is not
    // yet clear if the rectangle will conain a hyperlink, an anchor,
    // or another type of object. We suspect that this rectangle will
    // define a hyperlink, allocate a QString and set HTML_href to
    // point to this string. The string contains the name of the
    // destination which ---due to the nature of the PostScript
    // language--- will be defined only after characters are drawn and
    // the hyperref rectangle has been closed. We use "glopglyph" as a
    // temporary name. Since the pointer HTML_href is not NULL, the
    // chracter drawing routines will now underline all characters in
    // blue to point out that they correspond to a hyperlink. Also, as
    // soon as characters are drawn, the drawing routines will
    // allocate a DVI_Hyperlink and add it to the top of the vector
    // currentlyDrawnPage->hyperLinkList.
    if (special_command == "ps:SDict begin H.S end") {
      // At this stage, the vector 'hyperLinkList' should not contain
      // links with unspecified destinations (i.e. destination set to
      // 'glopglyph'). As a protection against bad DVI files, we make
      // sure to remove all link rectangles which point to
      // 'glopglyph'.
      while (!currentlyDrawnPage->hyperLinkList.isEmpty())
	if (currentlyDrawnPage->hyperLinkList.last().linkText == "glopglyph")
	  currentlyDrawnPage->hyperLinkList.pop_back();
	else
	  break;

      HTML_href = new QString("glopglyph");
      return;
    }
    
    // Hyperref: end of hyperref rectangle of unknown type or hyperref
    // link rectangle. In these cases we set HTML_href to NULL, which
    // causes the character drawing routines to stop drawing
    // characters underlined in blue. Note that the name of the
    // destination is still set to "glopglyph". In a well-formed DVI
    // file, this special command is immediately followed by another
    // special, where the destination is specified. This special is
    // treated below.
    if ((special_command == "ps:SDict begin H.R end") || special_command.endsWith("H.L end")) {
      if (HTML_href != NULL) {
	delete HTML_href;
	HTML_href = NULL;
      }
      return; // end of hyperref rectangle
    }
    
    // Hyperref: end of anchor rectangle. If this special is
    // encountered, the rectangle, which was started with "ps:SDict
    // begin H.S end" does not contain a link, but an anchor for a
    // link. Anchors, however, have already been dealt with in the
    // prescan phase and will not be considered here. Thus, we set
    // HTML_href to NULL so that character drawing routines will no
    // longer underline hyperlinks in blue, and remove the link from
    // the hyperLinkList. NOTE: in a well-formed DVI file, the "H.A"
    // special comes directly after the "H.S" special. A
    // hyperlink-anchor rectangle therefore never contains characters,
    // so no character will by accidentally underlined in blue.
    if (special_command.endsWith("H.A end")) {
      if (HTML_href != NULL) {
	delete HTML_href;
	HTML_href = NULL;
      }
      while (!currentlyDrawnPage->hyperLinkList.isEmpty())
	if (currentlyDrawnPage->hyperLinkList.last().linkText == "glopglyph")
	  currentlyDrawnPage->hyperLinkList.pop_back();
	else
	  break;
      return; // end of hyperref anchor
    }
    
    // Hyperref: specification of a hyperref link rectangle's
    // destination. As mentioned above, the destination of a hyperlink
    // is specified only AFTER the rectangle has been specified. We
    // will therefore go through the list of rectangles stored in
    // currentlyDrawnPage->hyperLinkList, find those whose destination
    // is open and fill in the value found here. NOTE: the character
    // drawing routines sometimes split a single hyperlink rectangle
    // into several rectangles (e.g. if the font changes, or when a
    // line break is encountered) 
    if (special_command.startsWith("ps:SDict begin [") && special_command.endsWith(" pdfmark end")) {
      if (!currentlyDrawnPage->hyperLinkList.isEmpty()) {
	QString targetName = special_command.section('(', 1, 1).section(')', 0, 0);
	QValueVector<DVI_Hyperlink>::iterator it;
        for( it = currentlyDrawnPage->hyperLinkList.begin(); it != currentlyDrawnPage->hyperLinkList.end(); ++it ) 
	  if (it->linkText == "glopglyph")
	    it->linkText = targetName;
      }
      return; // hyperref definition of link/anchor/bookmark/etc
    }
  }
  
  
  // The following special commands are not used here; they are of
  // interest only during the prescan phase. We recognize them here
  // anyway, to make sure that KDVI doesn't complain about
  // unrecognized special commands.
  if ((cp[0] == '!') || 
      (cp[0] == '"') || 
      (strncasecmp(cp, "html:<A name=", 13) == 0) ||
      (strncasecmp(cp, "ps:", 3) == 0) ||
      (strncasecmp(cp, "papersize", 9) == 0) ||
      (strncasecmp(cp, "background", 10) == 0) )
    return;
  
  printErrorMsgForSpecials(i18n("The special command '%1' is not implemented.").arg(special_command));
  return;
}
