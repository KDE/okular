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
 *	xdvi is based on prior work as noted in the modification history, below.
 */

/*
 * DVI previewer for X.
 *
 * Eric Cooper, CMU, September 1985.
 *
 * Code derived from dvi-imagen.c.
 *
 * Modification history:
 * 1/1986	Modified for X.10	--Bob Scheifler, MIT LCS.
 * 7/1988	Modified for X.11	--Mark Eichin, MIT
 * 12/1988	Added 'R' option, toolkit, magnifying glass
 *					--Paul Vojta, UC Berkeley.
 * 2/1989	Added tpic support	--Jeffrey Lee, U of Toronto
 * 4/1989	Modified for System V	--Donald Richardson, Clarkson Univ.
 * 3/1990	Added VMS support	--Scott Allendorf, U of Iowa
 * 7/1990	Added reflection mode	--Michael Pak, Hebrew U of Jerusalem
 * 1/1992	Added greyscale code	--Till Brychcy, Techn. Univ. Muenchen
 *					  and Lee Hetherington, MIT
 * 4/1994	Added DPS support, bounding box
 *					--Ricardo Telichevesky
 *					  and Luis Miguel Silveira, MIT RLE.
 */



#include "dvi_init.h"
#include "dviwin.h"



#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qbitmap.h> 
#include <qfileinfo.h>
#include <stdlib.h>


extern "C" {
#include "dvi.h"
}

#include "fontpool.h"
#include "glyph.h"
#include "oconfig.h"

extern char *xmalloc (unsigned, const char *);

#ifndef	DVI_BUFFER_LEN
#define	DVI_BUFFER_LEN	512
#endif

extern unsigned char	dvi_buffer[DVI_BUFFER_LEN];
extern struct frame	*current_frame;
extern struct frame	frame0;	/* dummy head of list */

#define	PK_PRE		247
#define	PK_ID		89
#define	PK_MAGIC	(PK_PRE << 8) + PK_ID
#define	GF_PRE		247
#define	GF_ID		131
#define	GF_MAGIC	(GF_PRE << 8) + GF_ID
#define	VF_PRE		247
#define	VF_ID_BYTE	202
#define	VF_MAGIC	(VF_PRE << 8) + VF_ID_BYTE

#define	dvi_oops(str)	(dvi_oops_msg = (str.utf8()), longjmp(dvi_env, 1))

static	Boolean	font_not_found;

/*
 * DVI preamble and postamble information.
 */

static	long	numerator, denominator;

/*
 * Offset in DVI file of last page, set in read_postamble().
 */
static	long	last_page_offset;




/** MAGSTEPVALUE - If the given magnification is close to a \magstep
    or a \magstephalf, then return twice the number of
    \magsteps. Otherwise return NOMAGSTP. */

#define	NOMAGSTP (-29999)
#define	NOBUILD	29999
#define MAGSTEP_MAX 40

static int magstep(int n, int bdpi)
{
  double t;
  int step;
  int neg = 0;

  if (n < 0) {
    neg = 1;
    n = -n;
  }
  
  if (n & 1) {
    n &= ~1;
    t = 1.095445115;
  }
  else
    t = 1.0;
  
  while (n > 8) {
    n -= 8;
    t = t * 2.0736;
  }

  while (n > 0) {
    n -= 2;
    t = t * 1.2;
  }

  /* Unnecessary casts to shut up stupid compilers. */
  step = (int)(0.5 + (neg ? bdpi / t : bdpi * t));
  return step;
}

static unsigned kpse_magstep_fix(unsigned dpi,  unsigned bdpi,  int *m_ret)
{
  int m;
  int mdpi = -1;
  unsigned real_dpi = 0;
  int sign = dpi < bdpi ? -1 : 1; /* negative or positive magsteps? */
  
  for (m = 0; !real_dpi && m < MAGSTEP_MAX; m++) { /* don't go forever */
    mdpi = magstep (m * sign, bdpi);
    if (abs(mdpi - (int) dpi) <= 1) /* if this magstep matches, quit */
      real_dpi = mdpi;
    else 
      if ((mdpi - (int) dpi) * sign > 0) /* if gone too far, quit */
	real_dpi = dpi;
  }
  
  // If requested, return the encoded magstep (the loop went one too
  // far). More unnecessary casts.
  if (m_ret)
    *m_ret = real_dpi == (unsigned)(mdpi ? (m - 1) * sign : 0);

  // Always return the true dpi found.
  return real_dpi ? real_dpi : dpi;
}

static	int magstepvalue(float *mag)
{
  int m_ret;
  unsigned dpi_ret = kpse_magstep_fix ((unsigned) *mag, (unsigned) pixels_per_inch, &m_ret);
  *mag = (float) dpi_ret; /* MAG is actually a dpi.  */
  return m_ret ? m_ret : NOMAGSTP;
}


/** define_font reads the rest of the fntdef command and then reads in
 *  the specified pixel file, adding it to the global linked-list
 *  holding all of the fonts used in the job.  */

font *define_font(FILE *file, unsigned int cmnd, font *vfparent, QIntDict<struct font> *TeXNumberTable, class fontPool *font_pool)
{
  struct font *fontp;
  int	magstepval;
  int	size;

  int   TeXnumber = num(file, (int) cmnd - FNTDEF1 + 1);
  long  checksum  = four(file);
  int   scale     = four(file);
  int   design    = four(file);
  int   len       = one(file) + one(file); /* sequence point in the middle */
  char *fontname  = xmalloc((unsigned) len + 1, "font name");
  Fread(fontname, sizeof(char), len, file);
  fontname[len] = '\0';



#ifdef DEBUG_FONTS
  kdDebug() << "Define font \"" << fontname << "\" scale=" << scale << " design=" << design << endl;
#endif

  // The calculation here is some sort of black magic which I do not
  // understand. Anyone with time, could you figure out what's going
  // on here? -- Stefan Kebekus
  float  fsize;
  double scale_dimconv;
  if (vfparent == NULL) {
    fsize = 0.001 * scale / design * magnification * pixels_per_inch;
    scale_dimconv = dimconv;
  } else {
    /* The scaled size is given in units of vfparent->scale * 2 ** -20
       SPELL units, so we convert it into SPELL units by multiplying
       by vfparent->dimconv. The design size is given in units of 2 **
       -20 pt, so we convert into SPELL units by multiplying by
       (pixels_per_inch * 2**16) / (72.27 * 2**20).  */

    fsize = (72.27 * (1<<4)) * vfparent->dimconv * scale / design;
    scale_dimconv = vfparent->dimconv;
  }
  magstepval = magstepvalue(&fsize);
  size       = (int)(fsize + 0.5);

  fontp = font_pool->appendx(fontname, fsize, checksum, magstepval, scale * scale_dimconv / (1<<20));

  // Insert font in dictionary and make sure the dictionary is big
  // enough.
  if (TeXNumberTable->size()-2 <= TeXNumberTable->count())
    // Not quite optimal. The size of the dict. should be a prime. I
    // don't care
    TeXNumberTable->resize(TeXNumberTable->size()*2); 
  TeXNumberTable->insert(TeXnumber, fontp);
  return fontp;
}


void dvifile::process_preamble(void)
{
  unsigned char   k;

  if (one(file) != PRE)
    dvi_oops(i18n("DVI file doesn't start with preamble."));
  if (one(file) != 2)
    dvi_oops(i18n("Wrong version of DVI output for this program."));

  numerator     = four(file);
  denominator   = four(file);
  magnification = four(file);
  dimconv       = (((double) numerator * magnification) / ((double) denominator * 1000.));
  dimconv       = dimconv * (((long) pixels_per_inch)<<16) / 254000;

  // Read the generatorString (such as "TeX output ..." from the DVI-File)
  char	job_id[300];
  k             = one(file);
  k             = (k > 299) ? 299 : k;
  Fread(job_id, sizeof(char), (int) k, file);
  job_id[k] = '\0';
  generatorString = job_id;
}

/*
 *      find_postamble locates the beginning of the postamble
 *	and leaves the file ready to start reading at that location.
 */
#define	TMPSIZ	516	/* 4 trailer bytes + 512 junk bytes allowed */
void dvifile::find_postamble(void)
{
  long	pos;
  unsigned char	temp[TMPSIZ];
  unsigned char	*p;
  unsigned char	*p1;
  unsigned char	byte;

  Fseek(file, (long) 0, 2);
  pos = ftell(file) - TMPSIZ;
  if (pos < 0)
    pos = 0;
  Fseek(file, pos, 0);
  p = temp + fread((char *) temp, sizeof(char), TMPSIZ, file);
  for (;;) {
    p1 = p;
    while (p1 > temp && *(--p1) != TRAILER)
      ;
    p = p1;
    while (p > temp && *(--p) == TRAILER)
      ;
    if (p <= p1 - 4)
      break;	/* found 4 TRAILER bytes */
    if (p <= temp)
      dvi_oops(i18n("DVI file corrupted"));
  }
  pos += p - temp;
  byte = *p;
  while (byte == TRAILER) {
    Fseek(file, --pos, 0);
    byte = one(file);
  }
  if (byte != 2)
    dvi_oops(i18n("Wrong version of DVI output for this program"));
  Fseek(file, pos - 4, 0);
  Fseek(file, sfour(file), 0);
}


void dvifile::read_postamble(void)
{
  unsigned char   cmnd;

  if (one(file) != POST)
    dvi_oops(i18n("Postamble doesn't begin with POST"));
  last_page_offset = four(file);
  if (numerator != four(file) || denominator != four(file) || magnification != four(file))
    dvi_oops(i18n("Postamble doesn't match preamble"));
  /* read largest box height and width */
  int unshrunk_page_h = (spell_conv(sfour(file)) >> 16);//@@@ + basedpi;
  int unshrunk_page_w = (spell_conv(sfour(file)) >> 16);//@@@ + basedpi;
  (void) two(file);	/* max stack size */
  total_pages = two(file);
  font_not_found = False;
  while ((cmnd = one(file)) >= FNTDEF1 && cmnd <= FNTDEF4)
    (void) define_font(file, cmnd, (struct font *) NULL, &tn_table, font_pool);
  if (cmnd != POSTPOST)
    dvi_oops(i18n("Non-fntdef command found in postamble"));
  if (font_not_found)
    KMessageBox::sorry( 0, i18n("Not all pixel files were found"));

  // Now we remove all those fonts from the memory which are no longer
  // in use.
  font_pool->release_fonts();
}

void dvifile::prepare_pages()
{
#ifdef DEBUG
  kdDebug() << "prepare_pages" << endl;
#endif

  page_offset = (long *) xmalloc((unsigned) total_pages * sizeof(long), "page directory");
  int i = total_pages;
  page_offset[--i] = last_page_offset;
  Fseek(file, last_page_offset, 0);
  /* Follow back pointers through pages in the DVI file, storing the
     offsets in the page_offset table. */
  while (i > 0) {
    Fseek(file, (long) (1+4+(9*4)), 1);
    Fseek(file, page_offset[--i] = four(file), 0);
  }
}


dvifile::dvifile(QString fname, fontPool *pool)
{
#ifdef DEBUG
  kdDebug() << "init_dvi_file: " << fname << endl;
#endif

  file        = NULL;
  page_offset = NULL;
  font_pool   = pool;

  file = fopen(fname.ascii(), "r");
  if (file == NULL) {
    /*@@@    KMessageBox::error( this,
			i18n("File error!\n\n") +
			i18n("Could not open the file\n") + 
			fname);
    */		
    return;
  }

  filename = fname;
  tn_table.clear();
  
  process_preamble();
  find_postamble();
  read_postamble();
  prepare_pages();

  return;
}

dvifile::~dvifile()
{
#ifdef DEBUG
  kdDebug() << "destroy dvi-file" << endl;
#endif

  if (font_pool != 0)
    font_pool->mark_fonts_as_unused();

  if (page_offset != NULL)
    free(page_offset);
  if (file != NULL)
    fclose(file);
}
