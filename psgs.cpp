#include <stdio.h>

#include <qpainter.h>
#include <kdebug.h>
#include <kprocess.h>

#include "dviwin.h"

extern unsigned int	page_w, page_h;
extern struct WindowRec currwin;
extern QPainter foreGroundPaint; // QPainter used for text

QString PostScriptHeaderString;

void dviWindow::renderPostScript(QString *PostScript) {

  // Step 1: Write the PostScriptString to a File
  FILE *f = fopen("/tmp/t","w");

  fputs("%!PS-Adobe-2.0\n",f);
  fputs("%%Creator: kdvi\n",f);
  fputs("%%Title: test.dvi\n",f);
  fputs("%%Pages: 1\n",f);
  fputs("%%PageOrder: Ascend\n",f);
  fputs("%%BoundingBox: 0 0 596 842\n",f); //@@@
  fputs("%%EndComments\n",f);

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
  //  fputs("@beginspecial 0 @llx 0 @lly 100 @urx 100 @ury @setspecial\n",f);
  if (PostScriptHeaderString.latin1() != NULL)
    fputs(PostScriptHeaderString.latin1(),f);
  if (PostScript->latin1() != NULL)
    fputs(PostScript->latin1(),f);
  //  fputs("@endspecial\n",f);
  fputs("end\n",f);
  fputs("showpage \n",f);
  fclose(f);

  // Step 2: Call GS with the File
  KProcess proc;
  proc << "gs";
  proc << "-dNOPAUSE" << "-dBATCH" << "-sDEVICE=png16m" << "-sOutputFile=/tmp/s";
  proc << QString("-g%1x%2").arg(page_w).arg(page_h);        // page size in pixels
  proc << QString("-r%1").arg(basedpi/currwin.shrinkfactor); // resolution in dpi
  proc << "/tmp/t";
  proc.start(KProcess::Block);

  // Step 3: write the generated output to the pixmap
  QPixmap tmp("/tmp/s");
  foreGroundPaint.drawPixmap(0, 0, tmp);
}
