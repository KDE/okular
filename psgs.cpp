#include <stdio.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kprocio.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "dviFile.h"
#include "psgs.h"

extern const char psheader[];



pageInfo::pageInfo(QString _PostScriptString) {
  PostScriptString = new QString(_PostScriptString);
  Gfx              = NULL;
  background       = Qt::white;
}


pageInfo::~pageInfo() {
  if (PostScriptString != 0L)
    delete PostScriptString;
}


// ======================================================

ghostscript_interface::ghostscript_interface(double dpi, int pxlw, int pxlh) {
  pageList.setAutoDelete(TRUE);
  MemoryCache.setAutoDelete(TRUE);
  DiskCache.setAutoDelete(TRUE);

  PostScriptHeaderString = new QString();
  resolution   = dpi;
  pixel_page_w = pxlw;
  pixel_page_h = pxlh;

  knownDevices.append("png256");
  knownDevices.append("jpeg");
  knownDevices.append("pnn");
  knownDevices.append("pnnraw");
  gsDevice = knownDevices.begin();
}

ghostscript_interface::~ghostscript_interface() {
  if (PostScriptHeaderString != 0L)
    delete PostScriptHeaderString;
}


void ghostscript_interface::setSize(double dpi, int pxlw, int pxlh) {
  resolution   = dpi;
  pixel_page_w = pxlw;
  pixel_page_h = pxlh;

  MemoryCache.clear();
  DiskCache.clear();
}


void ghostscript_interface::setPostScript(int page, QString PostScript) {
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


void ghostscript_interface::setColor(int page, QColor background_color) {
#ifdef DEBUG_PSGS
  kdDebug(4300) << "ghostscript_interface::setColor( " << page << ", " << background_color << " )" << endl;
#endif

  if (pageList.find(page) == 0) {
    pageInfo *info = new pageInfo(QString::null);
    info->background = background_color;
    // Check if dict is big enough
    if (pageList.count() > pageList.size() -2)
      pageList.resize(pageList.size()*2);
    pageList.insert(page, info);
  } else
    pageList.find(page)->background = background_color;
}


// Returns the background color for a certain page. This color is
// always guaranteed to be valid

QColor ghostscript_interface::getBackgroundColor(int page) {
#ifdef DEBUG_PSGS
  kdDebug(4300) << "ghostscript_interface::getBackgroundColor( " << page << " )" << endl;
#endif

  if (pageList.find(page) == 0) 
    return Qt::white;
  else 
    return pageList.find(page)->background;
}


void ghostscript_interface::clear(void) {
  PostScriptHeaderString->truncate(0);
  MemoryCache.clear();
  DiskCache.clear();
  
  // Deletes all items, removes temporary files, etc.
  pageList.clear();
}


void ghostscript_interface::gs_generate_graphics_file(int page, const QString &filename) {
  if (knownDevices.isEmpty())
    return;

  emit(setStatusBarText(i18n("Generating PostScript graphics...")));
  
  pageInfo *info = pageList.find(page);

  // Generate a PNG-file
  // Step 1: Write the PostScriptString to a File
  KTempFile PSfile(QString::null,".ps");
  FILE *f = PSfile.fstream();

  fputs("%!PS-Adobe-2.0\n",f);
  fputs("%%Creator: kdvi\n",f);
  fputs("%%Title: KDVI temporary PostScript\n",f);
  fputs("%%Pages: 1\n",f);
  fputs("%%PageOrder: Ascend\n",f);
  fprintf(f,"%%BoundingBox: 0 0 %ld %ld\n", 
	  (long)(72*(pixel_page_w/resolution)), 
	  (long)(72*(pixel_page_h/resolution)));  // HSize and VSize in 1/72 inch
  fputs("%%EndComments\n",f);
  fputs("%!\n",f);

  fputs(psheader,f);
  fputs("TeXDict begin",f);

  fprintf(f," %ld", (long)(72*65781*(pixel_page_w/resolution)) );  // HSize in (1/(65781.76*72))inch @@@
  fprintf(f," %ld", (long)(72*65781*(pixel_page_h/resolution)) );  // VSize in (1/(65781.76*72))inch 

  fputs(" 1000",f);                           // Magnification
  fputs(" 300 300",f);                        // dpi and vdpi
  fputs(" (test.dvi)",f);                     // Name
  fputs(" @start end\n",f);
  fputs("TeXDict begin\n",f);

  fputs("1 0 bop 0 0 a \n",f);                // Start page
  if (PostScriptHeaderString->latin1() != NULL)
    fputs(PostScriptHeaderString->latin1(),f);

  if (info->background != Qt::white) {
    QString colorCommand = QString("gsave %1 %2 %3 setrgbcolor clippath fill grestore\n").
      arg(info->background.red()).
      arg(info->background.green()).
      arg(info->background.blue());
    fputs(colorCommand.latin1(),f);
  }

  if (info->PostScriptString->latin1() != NULL)
    fputs(info->PostScriptString->latin1(),f);
  fputs("end\n",f);
  fputs("showpage \n",f);
  PSfile.close();

  // Step 2: Call GS with the File
  QFile::remove(filename.ascii());
  KProcIO proc;
  proc << "gs";
  proc << "-dSAFER" << "-dPARANOIDSAFER" << "-dDELAYSAFER" << "-dNOPAUSE" << "-dBATCH";
  proc << QString("-sDEVICE=%1").arg(*gsDevice);
  proc << QString("-sOutputFile=%1").arg(filename);
  proc << QString("-sExtraIncludePath=%1").arg(includePath);
  proc << QString("-g%1x%2").arg(pixel_page_w).arg(pixel_page_h); // page size in pixels
  proc << QString("-r%1").arg(resolution);                       // resolution in dpi
  proc << "-c" << "<< /PermitFileReading [ ExtraIncludePath ] /PermitFileWriting [] /PermitFileControl [] >> setuserparams .locksafe";
  proc << "-f" << PSfile.name();
  if (proc.start(KProcess::Block) == false) {
    // Starting ghostscript did not work. 
    // TODO: Issue error message, switch PS support off.
    kdError(4300) << "ghostview could not be started" << endl;
  }
  PSfile.unlink();

  // Check if gs has indeed produced a file.
  if (QFile::exists(filename) == false) {
    // No. Check is the reason is that the device is not compiled into
    // ghostscript. If so, try again with another device.
    QString GSoutput;
    while(proc.readln(GSoutput) != -1) {

      if (GSoutput.contains("Unknown device")) {
	kdDebug(4300) << QString("The version of ghostview installed on this computer does not support "
				   "the '%1' ghostview device driver.").arg(*gsDevice) << endl;
	knownDevices.remove(gsDevice);
	gsDevice = knownDevices.begin();
	if (knownDevices.isEmpty())
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
	else {
	  kdDebug(4300) << QString("KDVI will now try to use the '%1' device driver.").arg(*gsDevice) << endl;
	  gs_generate_graphics_file(page, filename);
	}
	return;
      }
    }
  }

  emit(setStatusBarText(QString::null));
}


QPixmap *ghostscript_interface::graphics(int page) {
  pageInfo *info = pageList.find(page);

  // No PostScript? Then return immediately.
  if ((info == 0) || (info->PostScriptString->isEmpty()))
    return 0;

  // Gfx exists in the MemoryCache?
  QPixmap *CachedCopy = MemoryCache.find(page);
  if (CachedCopy != NULL) 
    return new QPixmap(*CachedCopy);

  // Gfx exists in the DiskCache?
  KTempFile *CachedCopyFile = DiskCache.find(page);
  if (CachedCopyFile != NULL) {
    QPixmap *MemoryCopy = new QPixmap(CachedCopyFile->name());
    QPixmap *ReturnCopy = new QPixmap(*MemoryCopy);
    MemoryCache.insert(page, MemoryCopy);
    return ReturnCopy;
  }

  // Gfx exists neither in Disk- nor in Memorycache. We have to build
  // it ourselfes.
  KTempFile *GfxFile = new KTempFile(QString::null,".png");
  GfxFile->setAutoDelete(1);
  GfxFile->close(); // we are want the filename, not the file

  gs_generate_graphics_file(page, GfxFile->name());

  QPixmap *MemoryCopy = new QPixmap(GfxFile->name());
  QPixmap *ReturnCopy = new QPixmap(*MemoryCopy);
  MemoryCache.insert(page, MemoryCopy);
  DiskCache.insert(page, GfxFile);
  return ReturnCopy;
}


QString ghostscript_interface::locateEPSfile(const QString &filename, class dvifile *dvi)
{
  QString EPSfilename(filename);

  if (dvi == 0) {
    kdError(4300) << "ghostscript_interface::locateEPSfile called with second argument == 0" << endl;
    return EPSfilename;
  }

  QFileInfo fi1(dvi->filename);
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

  return EPSfilename;
}

#include "psgs.moc"
