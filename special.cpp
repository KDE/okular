/*
 * Copyright (c) 1994 Paul Vojta.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * NOTE:
 *	This module is based on prior work as noted below.
 */

/*
 * Support drawing routines for TeXsun and TeX
 *
 *      Copyright, (C) 1987, 1988 Tim Morgan, UC Irvine
 *	Adapted for xdvi by Jeffrey Lee, U. of Toronto
 *
 * At the time these routines are called, the values of hh and vv should
 * have been updated to the upper left corner of the graph (the position
 * the \special appears at in the dvi file).  Then the coordinates in the
 * graphics commands are in terms of a virtual page with axes oriented the
 * same as the Imagen and the SUN normally have:
 *
 *                      0,0
 *                       +-----------> +x
 *                       |
 *                       |
 *                       |
 *                      \ /
 *                       +y
 *
 * Angles are measured in the conventional way, from +x towards +y.
 * Unfortunately, that reverses the meaning of "counterclockwise"
 * from what it's normally thought of.
 *
 * A lot of floating point arithmetic has been converted to integer
 * arithmetic for speed.  In some places, this is kind-of kludgy, but
 * it's worth it.
 */

#include <kdebug.h>

#include "dviwin.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "oconfig.h"
extern	char	*xmalloc (unsigned, _Xconst char *);

extern "C" {
#define HAVE_PROTOTYPES
#include <kpathsea/c-ctype.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/line.h>
}

#define	MAXPOINTS	300	/* Max points in a path */
#define	TWOPI		(3.14159265359*2.0)
#define	MAX_PEN_SIZE	7	/* Max pixels of pen width */

extern QString PostScriptString;


GC	ruleGC; // @@@ needs to be removed.

/* Unfortunately, these values also appear in dvisun.c */
#define	xRESOLUTION	(pixels_per_inch/shrink_factor)
#define	yRESOLUTION	(pixels_per_inch/shrink_factor)



/*
 *	Code for PostScript<tm> specials begins here.
 */

static	void	ps_startup ARGS((int, int, char *));
void	NullProc ARGS((void)) {}
/* ARGSUSED */
static	void	NullProc2 ARGS((char *));

struct psprocs	psp = {		/* used for lazy startup of the ps machinery */
	/* toggle */		NullProc,
	/* destroy */		NullProc,
	/* interrupt */		NullProc,
	/* endpage */		NullProc,
	/* drawbegin */		ps_startup,
	/* drawraw */		NullProc2,
	/* drawfile */		NullProc2,
	/* drawend */		NullProc2};

struct psprocs	no_ps_procs = {		/* used if postscript is unavailable */
	/* toggle */		NullProc,
	/* destroy */		NullProc,
	/* interrupt */		NullProc,
	/* endpage */		NullProc,
	/* drawbegin */		drawbegin_none,
	/* drawraw */		NullProc2,
	/* drawfile */		NullProc2,
	/* drawend */		NullProc2};

static	Boolean		bbox_valid;
static	unsigned int	bbox_width;
static	unsigned int	bbox_height;
static	int		bbox_voffset;

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

static	void actual_startup()
{
  if (!((useGS && initGS())))
    psp = no_ps_procs;
}

static	void ps_startup(int xul, int yul, char *cp)
{
  if (!_postscript) {
    psp.toggle = actual_startup;
    draw_bbox();
    return;
  }
  actual_startup();
  psp.drawbegin(xul, yul, cp);
}

/* ARGSUSED */
static	void NullProc2(char *cp)
{
}

/* ARGSUSED */
void drawbegin_none(int xul, int yul, char *cp)
{
  draw_bbox();
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

/*	Keys for epsf specials */

static	const char	*keytab[]	= {
    "clip", 
    "llx",
    "lly",
    "urx",
    "ury",
    "rwi",
    "rhi",
    "hsize",
    "vsize",
    "hoffset",
    "voffset",
    "hscale",
    "vscale",
    "angle"
    };

#define	KEY_LLX	keyval[0]
#define	KEY_LLY	keyval[1]
#define	KEY_URX	keyval[2]
#define	KEY_URY	keyval[3]
#define	KEY_RWI	keyval[4]
#define	KEY_RHI	keyval[5]

#define	NKEYS	(sizeof(keytab)/sizeof(*keytab))
#define	N_ARGLESS_KEYS 1

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

extern QPainter foreGroundPaint;

static	void epsf_special(char *cp)
{
  kdError() << "epsf-special: psf" << cp <<endl;

  QString include_command_line(cp);
  QString include_command = include_command_line.simplifyWhiteSpace();

  // The line is supposed to start with "..ile=", and then comes the
  // filename. Figure out what the filename is and stow it away
  if (include_command.find("ile=") != 0) {
    kdError() << "epsf special PSf" << cp << " is unknown" << endl;
    return;
  }
  QString filename = include_command.mid(4);
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

  // Now draw the bounding box
  QRect bbox(PXL_H - currwin.base_x, PXL_V - currwin.base_y, (int)bbox_width, (int)bbox_height);
  //  foreGroundPaint.rotate(20);
  foreGroundPaint.setBrush(Qt::lightGray);
  foreGroundPaint.setPen  (Qt::black);
  foreGroundPaint.drawRoundRect(bbox, 1, 1);
  foreGroundPaint.drawText (bbox, (int)(Qt::AlignCenter), filename, -1, &bbox);


  PostScriptString.append( QString(" %1 %2 moveto\n").arg(PXL_H - currwin.base_x).arg(PXL_V - currwin.base_y) );
  PostScriptString.append( "@beginspecial @setspecial \n" );
  PostScriptString.append( QString(" (%1) run\n").arg(filename) );
  PostScriptString.append( "@endspecial \n" );

  return;

  /*
  char	*filename, *name;
  static	char		*buffer;
  static	unsigned int	buflen	= 0;
  unsigned int		len;
  char	*p;
  char	*q;
  int	flags	= 0;
  double	keyval[6];

  if (memcmp(cp, "ile=", 4) != 0) {
    kdError(1) << "epsf special PSf" << cp << " is unknown" << endl;
    return;
  }

  // find the filename
  p = cp + 4;
  filename = p;
  if (*p == '\'' || *p == '"') {
    do
      ++p;
    while (*p != '\0' && *p != *filename);
    ++filename;
  }
  else
    while (*p != '\0' && *p != ' ' && *p != '\t') // Go to end of string or to next white space
      ++p;
  if (*p != '\0') 
    *p++ = '\0';

  name = find_fig_file (filename);

  // Skip white space
  while (*p == ' ' || *p == '\t') 
    ++p;

  len = strlen(p) + NKEYS + 30;
  if (buflen < len) {
    if (buflen != 0)
      free(buffer);
    buflen = len;
    buffer = xmalloc(buflen, "epsf buffer");
  }
  Strcpy(buffer, "@beginspecial");
  q = buffer + strlen(buffer);
  while (*p != '\0') {
    char *p1 = p;
    int keyno;

    while (*p1 != '=' && !isspace(*p1) && *p1 != '\0') 
      ++p1;
    for (keyno = 0;; ++keyno) {
      if (keyno >= NKEYS) {
<<<<<<< special.cpp
	kDebugError(4300, 1, "The unknown keyword (%*s) in \\special will be ignored", (int)(p1 - p), p);
=======
	kdError(4300) << "unknown keyword in \\special will be ignored" << endl;
>>>>>>> 1.6
	break;
      }
      if (memcmp(p, keytab[keyno], p1 - p) == 0) {
	if (keyno >= N_ARGLESS_KEYS) {
	  if (*p1 == '=') 
	    ++p1;
	  if (keyno < N_ARGLESS_KEYS + 6) {
	    keyval[keyno - N_ARGLESS_KEYS] = atof(p1);
	    flags |= (1 << (keyno - N_ARGLESS_KEYS));
	  }
	  *q++ = ' ';
	  while (!isspace(*p1) && *p1 != '\0') 
	    *q++ = *p1++;
	}
	*q++ = ' ';
	*q++ = '@';
	Strcpy(q, keytab[keyno]);
	q += strlen(q);
 	break;
      }
    }
    p = p1;
    while (!isspace(*p) && *p != '\0') 
      ++p;
    while (isspace(*p)) 
      ++p;
  }
  Strcpy(q, " @setspecial\n");

  bbox_valid = False;
  if ((flags & 0x30) == 0x30 || ((flags & 0x30) && (flags & 0xf) == 0xf)){
    bbox_valid = True;
    bbox_width = 0.1 * ((flags & 0x10) ? KEY_RWI : KEY_RHI * (KEY_URX - KEY_LLX) / (KEY_URY - KEY_LLY))
      * dimconv / shrink_factor + 0.5;
    bbox_voffset = 
      bbox_height = 0.1 * ((flags & 0x20) ? KEY_RHI : KEY_RWI * (KEY_URY - KEY_LLY) / (KEY_URX - KEY_LLX))
      * dimconv / shrink_factor + 0.5;
  }

  if (name && currwin.win == mane.win) {
    psp.drawbegin(PXL_H - currwin.base_x, PXL_V - currwin.base_y, buffer);
    draw_file(psp, name);
    psp.drawend(" @endspecial");
    if (name != filename)
      free (name);
  }
  bbox_valid = False;
  */
}

/*
static	void bang_special(char *cp)
{
<<<<<<< special.cpp
  kDebugInfo(DEBUG, 4300, "bang %s", cp);
=======
>>>>>>> 1.6
  bbox_valid = False;
  
  if (currwin.win == mane.win) {
    psp.drawbegin(PXL_H - currwin.base_x, PXL_V - currwin.base_y, "@defspecial ");
    psp.drawraw(cp);
    psp.drawend(" @fedspecial");
  }
}
*/


static	void quote_special(QString cp)
{
  kdError() << "PostScript-quote " << cp.latin1() << endl;
  
  if (currwin.win == mane.win) {
    PostScriptString.append( QString(" %1 %2 moveto\n").arg(DVI_H/65536 - 300).arg(DVI_V/65536 - 300) );

    /*
    kDebugInfo("DVI_H/65536     %d",(int)(DVI_H/65536) );
    kDebugInfo("DVI_V/65536     %d",(int)(DVI_V/65536) );
    kDebugInfo("PXL_V           %d",(int)(PXL_V) );
    kDebugInfo("unshrunk page_h %d",unshrunk_page_h);
    */
    PostScriptString.append( " @beginspecial @setspecial \n" );
    PostScriptString.append( cp );
    PostScriptString.append( " @endspecial \n" );
  }
}


void applicationDoSpecial(char *cp)
{
  QString special_command(cp);
  
  if (special_command[0] == '"') {
    quote_special(special_command.mid(1));
    return;
  }
  kdError() << "special \"" << cp << "\" not implemented" << endl;
  return;
}

void psp_destroy()	{	psp.destroy();		}
void psp_toggle()	{	psp.toggle();		}
void psp_interrupt()	{	psp.interrupt();	}

