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




void dvifile::process_preamble(void)
{
  if (one(file) != PRE)
    dvi_oops(i18n("DVI file doesn't start with preamble."));
  if (one(file) != 2)
    dvi_oops(i18n("Wrong version of DVI output for this program."));

  numerator     = four(file);
  denominator   = four(file);
  magnification = four(file);
  dimconv       = (((double) numerator * magnification) / ((double) denominator * 1000.0));
  // @@@@ This does not fit the description of dimconv in the header file!!!
  dimconv       = dimconv * (((long) pixels_per_inch)<<16) / 254000;

  // Read the generatorString (such as "TeX output ..." from the DVI-File)
  char	job_id[300];
  unsigned char k = one(file);
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
  /* Read largest box height and width. Not used at the moment. */
  //int unshrunk_page_h = (spell_conv(sfour(file)) >> 16);//@@@ + basedpi;
  //int unshrunk_page_w = (spell_conv(sfour(file)) >> 16);//@@@ + basedpi;

  sfour(file);//@@@ + basedpi;
  sfour(file);//@@@ + basedpi;

  (void) two(file);	/* max stack size */
  total_pages = two(file);
  Boolean font_not_found = False;
  while ((cmnd = one(file)) >= FNTDEF1 && cmnd <= FNTDEF4) {
    int   TeXnumber = num(file, (int) cmnd - FNTDEF1 + 1);
    long  checksum  = four(file);
    int   scale     = four(file);
    int   design    = four(file);
    int   len       = one(file) + one(file); /* sequence point in the middle */
    char *fontname  = xmalloc((unsigned) len + 1, "font name");
    Fread(fontname, sizeof(char), len, file);
    fontname[len] = '\0';
    
#ifdef DEBUG_FONTS
    kdDebug() << "Postamble: define font \"" << fontname << "\" scale=" << scale << " design=" << design << endl;
#endif
    
    // Calculate the fsize as:  fsize = 0.001 * scale / design * magnification * MFResolutions[MetafontMode]
    struct font *fontp = font_pool->appendx(fontname, checksum, scale, design, 
					    0.001*scale/design*magnification*MFResolutions[font_pool->getMetafontMode()], dimconv);
    
    // Insert font in dictionary and make sure the dictionary is big
    // enough.
    if (tn_table.size()-2 <= tn_table.count())
      // Not quite optimal. The size of the dictionary should be a
      // prime. I don't care.
      tn_table.resize(tn_table.size()*2); 
    tn_table.insert(TeXnumber, fontp);
  }

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
#ifdef DEBUG_DVIFILE
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
#ifdef DEBUG_DVIFILE
  kdDebug() << "init_dvi_file: " << fname << endl;
#endif

  file        = NULL;
  page_offset = NULL;
  font_pool   = pool;
  sourceSpecialMarker = true;

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
#ifdef DEBUG_DVIFILE
  kdDebug() << "destroy dvi-file" << endl;
#endif

  if (font_pool != 0)
    font_pool->mark_fonts_as_unused();

  if (page_offset != NULL)
    free(page_offset);
  if (file != NULL)
    fclose(file);
}
