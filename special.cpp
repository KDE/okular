
#define DEBUG_SPECIAL

#include <kdebug.h>

#include "dviwin.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "oconfig.h"


extern "C" {
#define HAVE_PROTOTYPES
#include <kpathsea/c-ctype.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/line.h>
}


extern QPainter foreGroundPaint;

void draw_bbox()
{
#ifdef auskommentiert
  if (bbox_valid) {
    put_border(PXL_H - currwin.base_x,
	       PXL_V - currwin.base_y - bbox_voffset,
	       bbox_width, bbox_height);
    bbox_valid = False;
  }
#endif
}

#ifdef auskommentiert
/* If FILENAME starts with a left quote, set *DECOMPRESS to 1 and return
   the rest of FILENAME. Otherwise, look up FILENAME along the usual
   path for figure files, set *DECOMPRESS to 0, and return the result
   (NULL if can't find the file).  */

static string find_fig_file (char *filename)
{
  char *name;
  
  name = kpse_find_pict (filename);
  if (!name)
    kdError() << "Cannot open PS file " << filename << endl;
  
  return name;
}


/* If DECOMPRESS is zero, pass NAME to the drawfile proc.  But if
   DECOMPRESS is nonzero, open a pipe to it and pass the resulting
   output to the drawraw proc (in chunks).  */

static void draw_file (psprocs psp, char *name)
{
  kDebugInfo(DEBUG, 4300, "draw file %s",name);
  psp.drawfile (name);
}

static	void psfig_special(char *cp)
{
  kDebugInfo("PSFig special: %s",cp);

  char	*filename;
  int	raww, rawh;

  if (strncmp(cp, ":[begin]", 8) == 0) {
    cp += 8;
    bbox_valid = False;
    if (sscanf(cp,"%d %d\n", &raww, &rawh) >= 2) {
      bbox_valid = True;
      bbox_width = pixel_conv(spell_conv(raww));
      bbox_height = pixel_conv(spell_conv(rawh));
      bbox_voffset = 0;
    }
    if (currwin.win == mane.win)
      psp.drawbegin(PXL_H - currwin.base_x, PXL_V - currwin.base_y,cp);
  } else
    if (strncmp(cp, " plotfile ", 10) == 0) {
      cp += 10;
      while (isspace(*cp)) cp++;
      for (filename = cp; !isspace(*cp); ++cp);
      *cp = '\0';
      {
	char *name = find_fig_file (filename);
	if (name && currwin.win == mane.win) {
	  draw_file(psp, name);
	  if (name != filename)
	    free (name);
	}
      }
    } else
      if (strncmp(cp, ":[end]", 6) == 0) {
	cp += 6;
	if (currwin.win == mane.win) 
	  psp.drawend(cp);
	bbox_valid = False;
      } else { /* I am going to send some raw postscript stuff */
	if (*cp == ':')
	  ++cp;	/* skip second colon in ps:: */
	if (currwin.win == mane.win) {
	  /* It's drawbegin that initializes the ps process, so make
	     sure it's started up.  */
	  psp.drawbegin(PXL_H - currwin.base_x, PXL_V - currwin.base_y, "");
	  psp.drawend("");
	  psp.drawraw(cp);
	}
      }
}
#endif

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

  if (PostScriptOutPutString) {
    PostScriptHeaderString.append( QString(" (%1) run\n").arg(cp) );
  }
}


static void parse_special_argument(QString strg, char *argument_name, int *variable)
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
  kdError() << "epsf-special: psfile=" << cp <<endl;

  QString include_command = cp.simplifyWhiteSpace();

  // The line is supposed to start with "..ile=", and then comes the
  // filename. Figure out what the filename is and stow it away
  QString filename = include_command;
  filename.truncate(filename.find(' '));
  char *name = kpse_find_pict(filename.local8Bit());
  if (name != NULL) {
    filename = name;
    free(name);
  } else
    filename.truncate(0);

  //
  // Now parse the arguments. 
  //
  int  llx     = 0; 
  int  lly     = 0;
  int  urx     = 0;
  int  ury     = 0;
  int  rwi     = 0;
  int  rhi     = 0;
  int  hoffset = 0;

  // just to avoid ambiguities; the filename could contain keywords
  include_command = include_command.mid(include_command.find(' '));
  
  parse_special_argument(include_command, "llx=", &llx);
  parse_special_argument(include_command, "lly=", &lly);
  parse_special_argument(include_command, "urx=", &urx);
  parse_special_argument(include_command, "ury=", &ury);
  parse_special_argument(include_command, "rwi=", &rwi);
  parse_special_argument(include_command, "rhi=", &rhi);

  // calculate the size of the bounding box
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


  if (PostScriptOutPutString) {
    PostScriptOutPutString->append( QString(" %1 %2 moveto\n").arg(DVI_H/65536 - 300).arg(DVI_V/65536 - 520) );
    PostScriptOutPutString->append( "@beginspecial @setspecial \n" );
    PostScriptOutPutString->append( QString(" (%1) run\n").arg(filename) );
    PostScriptOutPutString->append( "@endspecial \n" );
  } else {
    kdDebug() << "_postscript " << _postscript << endl;
    if (!_postscript) {
      // Don't show PostScript, just draw to bounding box
      QRect bbox(PXL_H - currwin.base_x, PXL_V - currwin.base_y, (int)bbox_width, (int)bbox_height);
      foreGroundPaint.setBrush(Qt::lightGray);
      foreGroundPaint.setPen  (Qt::black);
      foreGroundPaint.drawRoundRect(bbox, 1, 1);
      foreGroundPaint.drawText (bbox, (int)(Qt::AlignCenter), filename, -1, &bbox);
    }
  }
  return;
}

void dviWindow::bang_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  //  kdDebug() << "PostScript-special, literal header " << cp.latin1() << endl;
#endif

  if (currwin.win == mane.win && PostScriptOutPutString) {
    PostScriptHeaderString.append( " @defspecial \n" );
    PostScriptHeaderString.append( cp );
    PostScriptHeaderString.append( " @fedspecial \n" );
  }
}



void dviWindow::quote_special(QString cp)
{
#ifdef DEBUG_SPECIAL
  //  kdError() << "PostScript-special, literal PostScript " << cp.latin1() << endl;
#endif
  
  if (currwin.win == mane.win && PostScriptOutPutString) {
    PostScriptOutPutString->append( QString(" %1 %2 moveto\n").arg(DVI_H/65536 - 300).arg(DVI_V/65536 - 520) );
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

  kdError() << "special \"" << cp << "\" not implemented" << endl;
  return;
}
