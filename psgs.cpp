#include <stdio.h>

#include <qpainter.h>
#include <kdebug.h>
#include <kprocess.h>
#include <ktempfile.h>

#include "dviwin.h"

extern unsigned int	page_w, page_h;
extern struct WindowRec currwin;
extern QPainter foreGroundPaint; // QPainter used for text

QString PostScriptHeaderString;

void dviWindow::renderPostScript(QString *PostScript) {

  // Step 1: Write the PostScriptString to a File
  KTempFile PSfile(QString::null,".ps");
  FILE *f = PSfile.fstream();

  fputs("%!PS-Adobe-2.0\n",f);
  fputs("%%Creator: kdvi\n",f);
  fprintf(f,"%%Title: %s\n", filename.latin1());
  fputs("%%Pages: 1\n",f);
  fputs("%%PageOrder: Ascend\n",f);
  fprintf(f,"%%BoundingBox: 0 0 %ld %ld\n", (long)(72*unshrunk_paper_w) / basedpi, 
	  (long)(72*unshrunk_paper_h) / basedpi);  // HSize and VSize in 1/72 inch
  fputs("",f); //@@@
  fputs("%%EndComments\n",f);
  fputs("%!\n",f);

  fputs("(/usr/share/texmf/dvips/base/texc.pro) run\n",f);
  fputs("(/usr/share/texmf/dvips/base/special.pro) run\n",f);
  fputs("TeXDict begin",f);

  fprintf(f," %ld", (long)72*65781*(unshrunk_paper_w / basedpi));  // HSize in (1/(65781.76*72))inch
  fprintf(f," %ld", (long)72*65781*(unshrunk_paper_h / basedpi));  // VSize in (1/(65781.76*72))inch
  fputs(" 1000",f);                                                // Magnification
  fputs(" 300 300",f);                                             // dpi and vdpi 
  fputs(" (test.dvi)",f);                                          // Name
  fputs(" @start end\n",f);
  fputs("TeXDict begin\n",f);

  fputs("1 0 bop 0 0 a \n",f);                                     // Start page
  if (PostScriptHeaderString.latin1() != NULL)
    fputs(PostScriptHeaderString.latin1(),f);
  if (PostScript->latin1() != NULL)
    fputs(PostScript->latin1(),f);
  fputs("end\n",f);
  fputs("showpage \n",f);
  PSfile.close();

  // Step 2: Call GS with the File
  KTempFile PNGfile(QString::null,".png");
  PNGfile.close(); // we are just interested in the filename, not the file

  KProcess proc;
  proc << "gs";
  proc << "-dNOPAUSE" << "-dBATCH" << "-sDEVICE=png16m";
  proc << QString("-sOutputFile=%1").arg(PNGfile.name());
  proc << QString("-g%1x%2").arg(page_w).arg(page_h);        // page size in pixels
  proc << QString("-r%1").arg(basedpi/currwin.shrinkfactor); // resolution in dpi
  proc << PSfile.name();
  proc.start(KProcess::Block);
  PSfile.unlink();

  // Step 3: write the generated output to the pixmap
  QPixmap tmp(PNGfile.name());
  foreGroundPaint.drawPixmap(0, 0, tmp);
  PNGfile.unlink();
}
