#include <stdio.h>

#include <kdebug.h>
#include <kprocess.h>


#include "psgs.h"

extern const char psheader[];

pageInfo::pageInfo(QString PS) {
  PostScriptString = new QString(PS);
  Gfx              = NULL;
}


// ======================================================

ghostscript_interface::ghostscript_interface(double dpi, int pxlw, int pxlh) {
  //@@@ Error checking !
  pageList     = new QIntDict<pageInfo>(256);
  pageList->setAutoDelete(TRUE);

  MemoryCache  = new QIntCache<QPixmap>(PAGES_IN_MEMORY_CACHE, PAGES_IN_MEMORY_CACHE);
  MemoryCache->setAutoDelete(TRUE);

  DiskCache    = new QIntCache<KTempFile>(PAGES_IN_DISK_CACHE, PAGES_IN_DISK_CACHE);
  DiskCache->setAutoDelete(TRUE);

  PostScriptHeaderString = new QString();
  resolution   = dpi;
  pixel_page_w = pxlw;
  pixel_page_h = pxlh;
}

ghostscript_interface::~ghostscript_interface() {
  if (pageList != NULL)
    delete pageList;

  if (MemoryCache != NULL)
    delete MemoryCache;

  if (DiskCache != NULL)
    delete DiskCache;
}

void ghostscript_interface::setSize(double dpi, int pxlw, int pxlh) {
  resolution   = dpi;
  pixel_page_w = pxlw;
  pixel_page_h = pxlh;

  MemoryCache->clear();
  DiskCache->clear();
}


void ghostscript_interface::setPostScript(int page, QString PostScript) {
  pageInfo *info = new pageInfo(PostScript);

  // Check if dict is big enough
  if (pageList->count() > pageList->size() -2)
    pageList->resize(pageList->size()*2);
  pageList->insert(page, info);
}


void ghostscript_interface::clear(void) {
  PostScriptHeaderString->truncate(0);

  MemoryCache->clear();
  DiskCache->clear();

  // Deletes all items, removes temporary files, etc.
  pageList->clear();
}

void ghostscript_interface::gs_generate_graphics_file(int page, QString filename){
  pageInfo *info = pageList->find(page);

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
  if (info->PostScriptString->latin1() != NULL)
    fputs(info->PostScriptString->latin1(),f);
  fputs("end\n",f);
  fputs("showpage \n",f);
  PSfile.close();

  // Step 2: Call GS with the File
  KProcess proc;
  proc << "gs";
  proc << "-dNOPAUSE" << "-dBATCH" << "-sDEVICE=png256";
  proc << QString("-sOutputFile=%1").arg(filename);
  proc << QString("-g%1x%2").arg(pixel_page_w).arg(pixel_page_h); // page size in pixels
  proc << QString("-r%1").arg(resolution);                       // resolution in dpi
  proc << PSfile.name();
  proc.start(KProcess::Block);
  PSfile.unlink();
}


QPixmap *ghostscript_interface::graphics(int page) {

  pageInfo *info = pageList->find(page);

  // No PostScript? Then return immediately.
  if (info == NULL)
    return NULL;

  // Gfx exists in the MemoryCache?
  QPixmap *CachedCopy = MemoryCache->find(page);
  if (CachedCopy != NULL) 
    return new QPixmap(*CachedCopy);

  // Gfx exists in the DiskCache?
  KTempFile *CachedCopyFile = DiskCache->find(page);
  if (CachedCopyFile != NULL) {
    QPixmap *MemoryCopy = new QPixmap(CachedCopyFile->name());
    QPixmap *ReturnCopy = new QPixmap(*MemoryCopy);
    MemoryCache->insert(page, MemoryCopy);
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
  MemoryCache->insert(page, MemoryCopy);
  DiskCache->insert(page, GfxFile);
  return ReturnCopy;
}
