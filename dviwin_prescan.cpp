// dviwin_prescan.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include "dviwin.h"
#include "dvi.h"
#include "fontpool.h"
#include "performanceMeasurement.h"
#include "TeXFont.h"
#include "xdvi.h"

#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>
#include <kprocio.h>
#include <qbitmap.h> 
#include <qdir.h> 
#include <qfileinfo.h>
#include <qimage.h> 
#include <qpainter.h>
#include <qpaintdevice.h>


extern QPainter foreGroundPaint;
extern QColor parseColorSpecification(QString colorSpec);
extern void parse_special_argument(QString strg, const char *argument_name, int *variable);


//#define DEBUG_PRESCAN




void dviWindow::prescan_embedPS(char *cp, Q_UINT8 *beginningOfSpecialCommand)
{
#ifdef  DEBUG_PRESCAN
  kdDebug(4300) << "dviWindow::prescan_embedPS( cp = " << cp << " ) " << endl;
#endif

  // Encapsulated Postscript File
  if (strncasecmp(cp, "PSfile=", 7) != 0) 
    return;

  QString command(cp+7);
  
  QString include_command = command.simplifyWhiteSpace();
  
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

  QString originalFName = EPSfilename;
  
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
  
  if (!QFile::exists(EPSfilename)) {
    kdWarning(4300) << "Could not locate file '" << originalFName << "'" << endl;
    return;
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


  // Generate the PostScript commands to be included
  QString PS = QString("ps: @beginspecial %1 @llx %2 @lly %3 @urx %4 @ury").arg(llx).arg(lly).arg(urx).arg(ury);
  if (rwi != 0)
    PS.append( QString(" %1 @rwi").arg(rwi) );
  if (rhi != 0)
    PS.append( QString(" %1 @rhi").arg(rhi) );
  if (angle != 0)
    PS.append( QString(" %1 @angle").arg(angle) );
  PS.append( " @setspecial\n" );
  
  QFile file( EPSfilename );
  if ( file.open( IO_ReadOnly ) ) {
    QTextStream stream( &file );
    while ( !stream.atEnd() ) {
      PS += stream.readLine().section( '%', 0, 0);
      PS += "\n";
    }
    file.close();
  }
  PS.append( "@endspecial" );
  PS = PS.simplifyWhiteSpace();
  

  // Warm-up: just remove the PS inclusion
  Q_UINT32 lengthOfOldSpecial = command_pointer - beginningOfSpecialCommand;
  Q_UINT32 lengthOfNewSpecial = PS.length()+5;

  Q_UINT8 *newDVI = new Q_UINT8[dviFile->size_of_file + lengthOfNewSpecial-lengthOfOldSpecial];
  if (newDVI == 0) {
    kdError(4300) << "Out of memory -- could not embed PS file" << endl;
    return;
  }

  Q_UINT8 *commandPtrSav = command_pointer;
  Q_UINT8 *endPtrSav = end_pointer;
  end_pointer = newDVI + dviFile->size_of_file + lengthOfNewSpecial-lengthOfOldSpecial;
  memcpy(newDVI, dviFile->dvi_Data, beginningOfSpecialCommand-dviFile->dvi_Data);
  command_pointer = newDVI+(beginningOfSpecialCommand-dviFile->dvi_Data);
  command_pointer[0] = XXX4;
  command_pointer++;
  writeUINT32(PS.length());
  memcpy(newDVI+(beginningOfSpecialCommand-dviFile->dvi_Data)+5, PS.latin1(), PS.length() );

  memcpy(newDVI+(beginningOfSpecialCommand-dviFile->dvi_Data)+lengthOfNewSpecial, beginningOfSpecialCommand+lengthOfOldSpecial,
	 dviFile->size_of_file-(beginningOfSpecialCommand-dviFile->dvi_Data)-lengthOfOldSpecial );

  // Adjust page pointers in the DVI file
  dviFile->size_of_file = dviFile->size_of_file + lengthOfNewSpecial-lengthOfOldSpecial;
  end_pointer = newDVI + dviFile->size_of_file;
  Q_UINT32 currentOffset = beginningOfSpecialCommand-dviFile->dvi_Data;
  for(Q_UINT16 i=0; i < dviFile->total_pages; i++) {
    if (dviFile->page_offset[i] > currentOffset) {
      dviFile->page_offset[i] = dviFile->page_offset[i] + lengthOfNewSpecial-lengthOfOldSpecial;
      command_pointer = dviFile->page_offset[i] + newDVI + 4*10 + 1;
      Q_UINT32 a = readUINT32();
      if (a > currentOffset) {
	a = a + lengthOfNewSpecial-lengthOfOldSpecial;
	command_pointer = dviFile->page_offset[i] + newDVI + 4*10 + 1;
	writeUINT32(a);
      }
    }
  }


  dviFile->beginning_of_postamble            = dviFile->beginning_of_postamble + lengthOfNewSpecial - lengthOfOldSpecial;
  dviFile->page_offset[dviFile->total_pages] = dviFile->beginning_of_postamble;

  command_pointer = newDVI + dviFile->beginning_of_postamble + 1;
  Q_UINT32 a = readUINT32();
  if (a > currentOffset) {
    a = a + lengthOfNewSpecial - lengthOfOldSpecial;
    command_pointer = newDVI + dviFile->beginning_of_postamble + 1;
    writeUINT32(a);
  }

  command_pointer = newDVI + dviFile->size_of_file - 1;
  while((*command_pointer == TRAILER) && (command_pointer > newDVI))
    command_pointer--;
  command_pointer -= 4;
  writeUINT32(dviFile->beginning_of_postamble);
  command_pointer -= 4;

  command_pointer = commandPtrSav;
  end_pointer     = endPtrSav;
  
  // Modify all pointers to point to the newly allocated memory
  command_pointer = newDVI + (command_pointer - dviFile->dvi_Data) + lengthOfNewSpecial-lengthOfOldSpecial;
  end_pointer = newDVI + (end_pointer - dviFile->dvi_Data)  + lengthOfNewSpecial-lengthOfOldSpecial;

  delete [] dviFile->dvi_Data;
  dviFile->dvi_Data = newDVI;

  return;
}


void dviWindow::prescan_ParsePapersizeSpecial(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "Papersize-Special : papersize" << cp << endl;
#endif

  cp = cp.simplifyWhiteSpace();

  if (cp[0] == '=') {
    cp = cp.mid(1);
    dviFile->suggestedPageSize = new pageSize;
    dviFile->suggestedPageSize->setPageSize(cp);
#ifdef DEBUG_SPECIAL
    kdDebug(4300) << "Suggested paper size is " << dviFile->suggestedPageSize.serialize() << "." << endl;
#endif
  } else 
    printErrorMsgForSpecials(i18n("The papersize data '%1' could not be parsed.").arg(cp));

  return;
}


void dviWindow::prescan_ParseBackgroundSpecial(QString cp)
{
  QColor col = parseColorSpecification(cp.stripWhiteSpace());
  if (col.isValid())
    PS_interface->setColor(current_page, col);
  return;
}


void dviWindow::prescan_ParseHTMLAnchorSpecial(QString cp)
{
  cp.truncate(cp.find('"'));
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "HTML-special, anchor " << cp.latin1() << endl;
  kdDebug(4300) << "page " << current_page << endl;
#endif
  anchorList[cp] = DVI_Anchor(current_page, currinf.data.dvi_v);
}


void dviWindow::prescan_ParsePSHeaderSpecial(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "PostScript-special, header " << cp.latin1() << endl;
#endif

  if (QFile::exists(cp)) 
    PS_interface->PostScriptHeaderString->append( QString(" (%1) run\n").arg(cp) );
}


void dviWindow::prescan_ParsePSBangSpecial(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "PostScript-special, literal header " << cp.latin1() << endl;
#endif
  
  PS_interface->PostScriptHeaderString->append( " @defspecial \n" );
  PS_interface->PostScriptHeaderString->append( cp );
  PS_interface->PostScriptHeaderString->append( " @fedspecial \n" );
}


void dviWindow::prescan_ParsePSQuoteSpecial(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdError(4300) << "PostScript-special, literal PostScript " << cp.latin1() << endl;
#endif
  
  double PS_H = (currinf.data.dvi_h*300.0)/(65536*MFResolutions[font_pool->getMetafontMode()])-300;
  double PS_V = (currinf.data.dvi_v*300.0)/MFResolutions[font_pool->getMetafontMode()] - 300;
  PostScriptOutPutString->append( QString(" %1 %2 moveto\n").arg(PS_H).arg(PS_V) );
  PostScriptOutPutString->append( " @beginspecial @setspecial \n" );
  PostScriptOutPutString->append( cp );
  PostScriptOutPutString->append( " @endspecial \n" );
}


void dviWindow::prescan_ParsePSSpecial(QString cp)
{
#ifdef DEBUG_SPECIAL
  kdError(4300) << "PostScript-special, direct PostScript " << cp.latin1() << endl;
#endif
  
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


void dviWindow::prescan_ParsePSFileSpecial(QString cp)
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
  
  return;
}


void dviWindow::prescan_ParseSourceSpecial(QString cp)
{
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


void dviWindow::prescan_parseSpecials(char *cp, Q_UINT8 *)
{
  QString special_command(cp);

  // Now to those specials which are only interpreted during the
  // prescan phase, and NOT during rendering.
  
  // PaperSize special
  if (strncasecmp(cp, "papersize", 9) == 0) {
    prescan_ParsePapersizeSpecial(special_command.mid(9));
    return;
  }
  
  // color special for background color
  if (strncasecmp(cp, "background", 10) == 0) {
    prescan_ParseBackgroundSpecial(special_command.mid(10));
    return;
  }
  
  // HTML anchor special
  if (strncasecmp(cp, "html:<A name=", 13) == 0) {
    prescan_ParseHTMLAnchorSpecial(special_command.mid(14));
    return;
  }
  
  // Postscript Header File
  if (strncasecmp(cp, "header=", 7) == 0) {
    prescan_ParsePSHeaderSpecial(special_command.mid(7));
    return;
  }
  
  // Literal Postscript Header
  if (cp[0] == '!') {
    prescan_ParsePSBangSpecial(special_command.mid(1));
    return;
  }
  
  // Literal Postscript inclusion
  if (cp[0] == '"') {
    prescan_ParsePSQuoteSpecial(special_command.mid(1));
    return;
  }
  
  // PS-Postscript inclusion
  if (strncasecmp(cp, "ps:", 3) == 0) {
    prescan_ParsePSSpecial(special_command);
    return;
  }
  
  // Encapsulated Postscript File
  if (strncasecmp(cp, "PSfile=", 7) == 0) {
    prescan_ParsePSFileSpecial(special_command.mid(7));
    return;
  }
  
  // source special
  if (strncasecmp(cp, "src:", 4) == 0) {
    prescan_ParseSourceSpecial(special_command.mid(4));
    return;
  }
  
  // Finally there are those special commands which must be considered
  // both during rendering and during the pre-scan phase
  
  // HTML anchor end
  if (strncasecmp(cp, "html:</A>", 9) == 0) {
    html_anchor_end();
    return;
  }
  
  return;
}


void dviWindow::prescan_setChar(unsigned int ch)
{
  TeXFontDefinition *fontp = currinf.fontp;
  if (fontp == NULL)
    return;

  if (currinf.set_char_p == &dviWindow::set_char) {
    glyph *g = ((TeXFont *)(currinf.fontp->font))->getGlyph(ch, true, globalColor);
    if (g == NULL)
      return;
    currinf.data.dvi_h += (int)(currinf.fontp->scaled_size_in_DVI_units * dviFile->getCmPerDVIunit() * 
				(MFResolutions[font_pool->getMetafontMode()] / 2.54)/16.0 * g->dvi_advance_in_units_of_design_size_by_2e20 + 0.5);
    return;
  }
 
  if (currinf.set_char_p == &dviWindow::set_vf_char) {
    macro *m = &currinf.fontp->macrotable[ch];
    if (m->pos == NULL) 
      return;
    currinf.data.dvi_h += (int)(currinf.fontp->scaled_size_in_DVI_units * dviFile->getCmPerDVIunit() * 
				(MFResolutions[font_pool->getMetafontMode()] / 2.54)/16.0 * m->dvi_advance_in_units_of_design_size_by_2e20 + 0.5);
    return;
  }
}


void dviWindow::prescan(parseSpecials specialParser)
{
#ifdef DEBUG_PRESCAN
  kdDebug(4300) << "dviWindow::prescan( ... )" << endl;
#endif
  
  Q_INT32 RRtmp=0, WWtmp=0, XXtmp=0, YYtmp=0, ZZtmp=0;
  Q_UINT8 ch;

  stack.clear();

  currinf.fontp        = NULL;
  currinf.set_char_p   = &dviWindow::set_no_char;

  for (;;) {
    ch = readUINT8();
    
    if (ch <= (unsigned char) (SETCHAR0 + 127)) {
      prescan_setChar(ch);
      continue;
    }

    if (FNTNUM0 <= ch && ch <= (unsigned char) (FNTNUM0 + 63)) {
      currinf.fontp = currinf.fonttable->find(ch - FNTNUM0);
      if (currinf.fontp == NULL) {
	errorMsg = i18n("The DVI code referred to font #%1, which was not previously defined.").arg(ch - FNTNUM0);
	return;
      }
      currinf.set_char_p = currinf.fontp->set_char_p;
      continue;
    }
    
    
    Q_INT32 a, b;
    
    switch (ch) {
    case SET1:
      prescan_setChar(readUINT8());
      break;
      
    case SETRULE:
      /* Be careful, dvicopy outputs rules with height =
	 0x80000000. We don't want any SIGFPE here. */
      a = readUINT32();
      b = readUINT32();
      b = ((long) (b *  65536.0*fontPixelPerDVIunit()));
      currinf.data.dvi_h += b;
      break;
      
    case PUTRULE:
      a = readUINT32();
      b = readUINT32();
      break;
      
    case PUT1:
    case NOP:
      break;
      
    case BOP:
      command_pointer += 11 * 4;
      currinf.data.dvi_h = MFResolutions[font_pool->getMetafontMode()] << 16; // Reminder: DVI-coordinates start at (1",1") from top of page
      currinf.data.dvi_v = MFResolutions[font_pool->getMetafontMode()];
      currinf.data.pxl_v = int(currinf.data.dvi_v/shrinkfactor);
      currinf.data.w = currinf.data.x = currinf.data.y = currinf.data.z = 0;
      break;
      
    case EOP:
      // Sanity check for the dvi-file: The DVI-standard asserts that
      // at the end of a page, the stack should always be empty.
      if (!stack.isEmpty()) {
	kdDebug(4300) << "Prescan: The stack was not empty when the EOP command was encountered." << endl;
	errorMsg = i18n("The stack was not empty when the EOP command was encountered.");
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
      break;
      
    case RIGHT1:
    case RIGHT2:
    case RIGHT3:
    case RIGHT4:
      RRtmp = readINT(ch - RIGHT1 + 1);
      currinf.data.dvi_h += ((long) (RRtmp *  65536.0*fontPixelPerDVIunit()));
      break;
      
    case W1:
    case W2:
    case W3:
    case W4:
      WWtmp = readINT(ch - W0);
      currinf.data.w = ((long) (WWtmp *  65536.0*fontPixelPerDVIunit()));
    case W0:
      currinf.data.dvi_h += currinf.data.w;
      break;
      
    case X1:
    case X2:
    case X3:
    case X4:
      XXtmp = readINT(ch - X0);
      currinf.data.x = ((long) (XXtmp *  65536.0*fontPixelPerDVIunit()));
    case X0:
      currinf.data.dvi_h += currinf.data.x;
      break;
      
    case DOWN1:
    case DOWN2:
    case DOWN3:
    case DOWN4:
      {
	Q_INT32 DDtmp = readINT(ch - DOWN1 + 1);
	currinf.data.dvi_v += ((long) (DDtmp *  65536.0*fontPixelPerDVIunit()))/65536;
	currinf.data.pxl_v  = int(currinf.data.dvi_v/shrinkfactor);
      }
      break;
      
    case Y1:
    case Y2:
    case Y3:
    case Y4:
      YYtmp = readINT(ch - Y0);
      currinf.data.y    = ((long) (YYtmp *  65536.0*fontPixelPerDVIunit()));
    case Y0:
      currinf.data.dvi_v += currinf.data.y/65536;
      currinf.data.pxl_v = int(currinf.data.dvi_v/shrinkfactor);
      break;
      
    case Z1:
    case Z2:
    case Z3:
    case Z4:
      ZZtmp = readINT(ch - Z0);
      currinf.data.z    = ((long) (ZZtmp *  65536.0*fontPixelPerDVIunit()));
    case Z0:
      currinf.data.dvi_v += currinf.data.z/65536;
      currinf.data.pxl_v  = int(currinf.data.dvi_v/shrinkfactor);
      break;
      
    case FNT1:
    case FNT2:
    case FNT3:
    case FNT4:
      currinf.fontp = currinf.fonttable->find(readUINT(ch - FNT1 + 1));
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
      {
	Q_UINT8 *beginningOfSpecialCommand = command_pointer-1;
	a = readUINT(ch - XXX1 + 1);
	if (a > 0) {
	  char	*cmd	= new char[a+1];
	  strncpy(cmd, (char *)command_pointer, a);
	  command_pointer += a;
	  cmd[a] = '\0';
	  (this->*specialParser)(cmd, beginningOfSpecialCommand);
	  delete [] cmd;
	}
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

      
    default:
      errorMsg = i18n("The unknown op-code %1 was encountered.").arg(ch);
      return;
    } /* end switch */
  } /* end for */
}
