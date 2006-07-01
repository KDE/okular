// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
//
// ghostscript_interface
//
// Part of KDVI - A framework for multipage text/gfx viewers
//
// (C) 2004 Stefan Kebekus
// Distributed under the GPL

#include <config.h>

#include "psgs.h"
#include "dviFile.h"
#include "kvs_debug.h"
#include "pageNumber.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kprocio.h>
#include <ktempfile.h>
#include <kurl.h>

#include <qapplication.h>
#include <qdir.h>

extern const char psheader[];

//#define DEBUG_PSGS


pageInfo::pageInfo(const QString& _PostScriptString) {
  PostScriptString = new QString(_PostScriptString);
  background  = Qt::white;
  permanentBackground = Qt::white;
}


pageInfo::~pageInfo() {
  if (PostScriptString != 0L)
    delete PostScriptString;
}


// ======================================================

ghostscript_interface::ghostscript_interface() {
  pageList.setAutoDelete(true);

  PostScriptHeaderString = new QString();

  knownDevices.append("png16m");
  knownDevices.append("jpeg");
  knownDevices.append("pnn");
  knownDevices.append("pnnraw");
  gsDevice = knownDevices.begin();
}

ghostscript_interface::~ghostscript_interface() {
  if (PostScriptHeaderString != 0L)
    delete PostScriptHeaderString;
}


void ghostscript_interface::setPostScript(const PageNumber& page, const QString& PostScript) {
#ifdef DEBUG_PSGS
  kdDebug(kvs::dvi) << "ghostscript_interface::setPostScript( " << page << ", ... )" << endl;
#endif

  if (pageList.find(page) == 0) {
    pageInfo *info = new pageInfo(PostScript);
    // Check if dict is big enough
    if (pageList.count() > pageList.size() -2)
      pageList.resize(pageList.size()*2);
    pageList.insert(page, info);
  } else
    *(pageList.find(page)->PostScriptString) = PostScript;
}


void ghostscript_interface::setIncludePath(const QString &_includePath) {
  if (_includePath.isEmpty())
     includePath = "*"; // Allow all files
  else
     includePath = _includePath+"/*";
}


void ghostscript_interface::setBackgroundColor(const PageNumber& page, const QColor& background_color, bool permanent) {
#ifdef DEBUG_PSGS
  kdDebug(kvs::dvi) << "ghostscript_interface::setBackgroundColor( " << page << ", " << background_color << " )" << endl;
#endif

  if (pageList.find(page) == 0) {
    pageInfo *info = new pageInfo(QString::null);
    info->background = background_color;
    if (permanent)
      info->permanentBackground = background_color;
    // Check if dict is big enough
    if (pageList.count() > pageList.size() -2)
      pageList.resize(pageList.size()*2);
    pageList.insert(page, info);
  } else {
    pageList.find(page)->background = background_color;
    if (permanent)
      pageList.find(page)->permanentBackground = background_color;
  }
}

void ghostscript_interface::restoreBackgroundColor(const PageNumber& page)
{
#ifdef DEBUG_PSGS
  kdDebug(kvs::dvi) << "ghostscript_interface::restoreBackgroundColor( " << page << " )" << endl;
#endif
  if (pageList.find(page) == 0)
    return;

  pageInfo *info = pageList.find(page);
  info->background = info->permanentBackground;
}

// Returns the background color for a certain page. This color is
// always guaranteed to be valid

QColor ghostscript_interface::getBackgroundColor(const PageNumber& page) const {
#ifdef DEBUG_PSGS
  kdDebug(kvs::dvi) << "ghostscript_interface::getBackgroundColor( " << page << " )" << endl;
#endif

  if (pageList.find(page) == 0)
    return Qt::white;
  else
    return pageList.find(page)->background;
}


void ghostscript_interface::clear() {
  PostScriptHeaderString->truncate(0);

  // Deletes all items, removes temporary files, etc.
  pageList.clear();
}


bool ghostscript_interface::gs_generate_graphics_file(const PageNumber& page, const QString& filename, long magnification) {
#ifdef DEBUG_PSGS
  kdDebug(kvs::dvi) << "ghostscript_interface::gs_generate_graphics_file( " << page << ", " << filename << " )" << endl;
#endif

  if (knownDevices.isEmpty()) {
    kdError(kvs::dvi) << "No known devices found" << endl;
    return false;
  }

  emit(setStatusBarText(i18n("Generating PostScript graphics...")));

  pageInfo *info = pageList.find(page);

  // Generate a PNG-file
  // Step 1: Write the PostScriptString to a File
  KTempFile PSfile(QString::null,".ps");

  QTextStream& os = *PSfile.textStream();
  os << "%!PS-Adobe-2.0\n"
     << "%%Creator: kdvi\n"
     << "%%Title: KDVI temporary PostScript\n"
     << "%%Pages: 1\n"
     << "%%PageOrder: Ascend\n"
        // HSize and VSize in 1/72 inch
     << "%%BoundingBox: 0 0 "
     << (Q_INT32)(72*(pixel_page_w/resolution)) << ' '
     << (Q_INT32)(72*(pixel_page_h/resolution)) << '\n'
     << "%%EndComments\n"
     << "%!\n"
     << psheader
     << "TeXDict begin "
        // HSize in (1/(65781.76*72))inch
     << (Q_INT32)(72*65781*(pixel_page_w/resolution)) << ' '
        // VSize in (1/(65781.76*72))inch
     << (Q_INT32)(72*65781*(pixel_page_h/resolution)) << ' '
        // Magnification
     << (Q_INT32)(magnification)
        // dpi and vdpi
     << " 300 300"
        // Name
     << " (test.dvi)"
     << " @start end\n"
     << "TeXDict begin\n"
        // Start page
     << "1 0 bop 0 0 a \n";

  if (PostScriptHeaderString->latin1() != NULL)
    os << PostScriptHeaderString->latin1();

  if (info->background != Qt::white) {
    QString colorCommand = QString("gsave %1 %2 %3 setrgbcolor clippath fill grestore\n").
      arg(info->background.red()/255.0).
      arg(info->background.green()/255.0).
      arg(info->background.blue()/255.0);
    os << colorCommand.latin1();
  }

  if (info->PostScriptString->latin1() != NULL)
    os << info->PostScriptString->latin1();

  os << "end\n"
     << "showpage \n";

  PSfile.close();

  // Step 2: Call GS with the File
  QFile::remove(filename.ascii());
  KProcIO proc;
  QStringList argus;
  argus << "gs";
  argus << "-dSAFER" << "-dPARANOIDSAFER" << "-dDELAYSAFER" << "-dNOPAUSE" << "-dBATCH";
  argus << QString("-sDEVICE=%1").arg(*gsDevice);
  argus << QString("-sOutputFile=%1").arg(filename);
  argus << QString("-sExtraIncludePath=%1").arg(includePath);
  argus << QString("-g%1x%2").arg(pixel_page_w).arg(pixel_page_h); // page size in pixels
  argus << QString("-r%1").arg(resolution);                       // resolution in dpi
  argus << "-dTextAlphaBits=4 -dGraphicsAlphaBits=2"; // Antialiasing
  argus << "-c" << "<< /PermitFileReading [ ExtraIncludePath ] /PermitFileWriting [] /PermitFileControl [] >> setuserparams .locksafe";
  argus << "-f" << PSfile.name();

#ifdef DEBUG_PSGS
  kdDebug(kvs::dvi) << argus.join(" ") << endl;
#endif

  proc << argus;
  if (proc.start(KProcess::Block) == false) {
    // Starting ghostscript did not work.
    // TODO: Issue error message, switch PS support off.
    kdError(kvs::dvi) << "ghostscript could not be started" << endl;
  }
  PSfile.unlink();

  // Check if gs has indeed produced a file.
  if (QFile::exists(filename) == false)
  {
    kdError(kvs::dvi) << "GS did not produce output." << endl;

    // No. Check is the reason is that the device is not compiled into
    // ghostscript. If so, try again with another device.
    QString GSoutput;
    while(proc.readln(GSoutput) != -1)
    {
      if (GSoutput.contains("Unknown device"))
      {
        kdDebug(kvs::dvi) << QString("The version of ghostview installed on this computer does not support "
                                   "the '%1' ghostview device driver.").arg(*gsDevice) << endl;
        knownDevices.remove(gsDevice);
        gsDevice = knownDevices.begin();
        if (knownDevices.isEmpty())
        {
          // TODO: show a requestor of some sort.
          KMessageBox::detailedError(0,
                                     i18n("<qt>The version of Ghostview that is installed on this computer does not contain "
                                          "any of the Ghostview device drivers that are known to KDVI. PostScript "
                                          "support has therefore been turned off in KDVI.</qt>"),
                                     i18n("<qt><p>The Ghostview program, which KDVI uses internally to display the "
                                          "PostScript graphics that is included in this DVI file, is generally able to "
                                          "write its output in a variety of formats. The sub-programs that Ghostview uses "
                                          "for these tasks are called 'device drivers'; there is one device driver for "
                                          "each format that Ghostview is able to write. Different versions of Ghostview "
                                          "often have different sets of device drivers available. It seems that the "
                                          "version of Ghostview that is installed on this computer does not contain "
                                          "<strong>any</strong> of the device drivers that are known to KDVI.</p>"
                                          "<p>It seems unlikely that a regular installation of Ghostview would not contain "
                                          "these drivers. This error may therefore point to a serious misconfiguration of "
                                          "the Ghostview installation on your computer.</p>"
                                          "<p>If you want to fix the problems with Ghostview, you can use the command "
                                          "<strong>gs --help</strong> to display the list of device drivers contained in "
                                          "Ghostview. Among others, KDVI can use the 'png256', 'jpeg' and 'pnm' "
                                          "drivers. Note that KDVI needs to be restarted to re-enable PostScript support."
                                          "</p></qt>"));
          return false;
        } else {
          kdDebug(kvs::dvi) << QString("KDVI will now try to use the '%1' device driver.").arg(*gsDevice) << endl;
          gs_generate_graphics_file(page, filename, magnification);
        }
      }
    }
  }
  else
  {
    emit(setStatusBarText(QString::null));
    return true;
  }
  return false;
}


cairo_surface_t* ghostscript_interface::graphics(const PageNumber& page, double dpi, long magnification, QSize pageSize) {
#ifdef DEBUG_PSGS
  kdDebug(kvs::dvi) << "ghostscript_interface::graphics( " << page << ", " << dpi << ", ... ) called." << endl;
#endif

  resolution   = dpi;

  pixel_page_w = pageSize.width();
  pixel_page_h = pageSize.height();

  pageInfo *info = pageList.find(page);

  // No PostScript? Then return immediately.
  if ((info == 0) || (info->PostScriptString->isEmpty())) {
#ifdef DEBUG_PSGS
    kdDebug(kvs::dvi) << "No PostScript found. Not drawing anything." << endl;
#endif
    return 0;
  }

  KTempFile gfxFile(QString::null, ".png");
  gfxFile.setAutoDelete(1);
  gfxFile.close(); // we are want the filename, not the file

  bool ok = gs_generate_graphics_file(page, gfxFile.name(), magnification);

  // If there were errors return 0.
  if (!ok)
    return 0;

  cairo_surface_t* pngImage = cairo_image_surface_create_from_png(gfxFile.name().latin1());

  if (!pngImage)
  {
    kdDebug(kvs::dvi) << "loading of the temporary png file failed." << endl;
    return 0;
  }
  return pngImage;
}


QString ghostscript_interface::locateEPSfile(const QString &filename, const KURL &base)
{
  // If the base URL indicates that the DVI file is local, try to find
  // the graphics file in the directory where the DVI file resides
  if (base.isLocalFile()) {
    QString path = base.path();       // -> "/bar/foo.dvi"
    QFileInfo fi1(path);
    QFileInfo fi2(fi1.dir(),filename);
    if (fi2.exists())
      return fi2.absFilePath();
  }

  // Otherwise, use kpsewhich to find the eps file.
  QString EPSfilename;
  KProcIO proc;
  proc << "kpsewhich" << filename;
  proc.start(KProcess::Block);
  proc.readln(EPSfilename);

  return EPSfilename.stripWhiteSpace();
}

#include "psgs.moc"
