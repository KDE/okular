
#define DEBUG 1

#include <malloc.h>

#include <kdebug.h>

#include "font.h"


extern "C" {
#include <kpathsea/config.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-stat.h>
#include <kpathsea/magstep.h>
#include <kpathsea/tex-glyph.h>
#include "dvi.h"
}

#include <stdio.h>
#include "oconfig.h"

extern FILE *xfopen(char *filename, char *type);

/** We try for a VF first because that's what dvips does.  Also, it's
 *  easier to avoid running MakeTeXPK if we have a VF this way.  */

FILE *font_open (char *font, char **font_ret, double dpi, int *dpi_ret, int dummy, char **filename_ret)
{
  FILE *ret;
  char *name = kpse_find_vf (font);
  
  if (name) {
    // VF fonts don't have a resolution, but loadfont will complain if
    // we don't return what it asked for.
    *dpi_ret  = (int)dpi;
    *font_ret = NULL;
  } else {
    kpse_glyph_file_type file_ret;
    name = kpse_find_glyph (font, (unsigned) (dpi + .5), kpse_any_glyph_format, &file_ret);
    if (name) {
      // If we got it normally, from an alias, or from MakeTeXPK,
      // don't fill in FONT_RET.  That tells load_font to complain.

      // tell load_font we found something good
      *font_ret = file_ret.source == kpse_glyph_source_fallback ? file_ret.name : NULL; 
      *dpi_ret  = file_ret.dpi;
    }
    // If no VF and no PK, FONT_RET is irrelevant?
  }
  
  // If we found a name, return the stream.
  ret           = name ? xfopen (name, FOPEN_R_MODE) : NULL;
  *filename_ret = name;

  return ret;
}



extern int n_files_left;

font::font(char *nfontname, float nfsize, long chk, int mag, double dconv)
{
  kDebugInfo(DEBUG, 4300, "constructing font %s at %d dpi", nfontname, (int) (nfsize + 0.5));

  fontname   = nfontname;
  fsize      = nfsize;
  checksum   = chk;
  magstepval = mag;
  flags      = FONT_IN_USE;
  file       = NULL; 
  filename   = NULL;
  dimconv    = dconv;
}

font::~font()
{
  kDebugInfo(DEBUG, 4300, "discarding font \"%s\" at %d dpi",fontname,(int)(fsize + 0.5));

  free(fontname);

  if (flags & FONT_LOADED) {
    if (file != NULL) {
      fclose(file);
      ++n_files_left;
    }
    free(filename);

    if (flags & FONT_VIRTUAL) {
      register struct macro *m;

      for (m = macro; m <= macro + maxchar; ++m)
	if (m->free_me)
	  free((char *) m->pos);
      free((char *) macro);
      vf_table.clear();
    } else {
      struct glyph *g;
      
      for (g = glyph; g <= glyph + maxchar; ++g)
	delete g;
      
      free((char *) glyph);
    }
  }
}


#define	PK_PRE		247
#define	PK_ID		89
#define	PK_MAGIC	(PK_PRE << 8) + PK_ID
#define	GF_PRE		247
#define	GF_ID		131
#define	GF_MAGIC	(GF_PRE << 8) + GF_ID
#define	VF_PRE		247
#define	VF_ID_BYTE	202
#define	VF_MAGIC	(VF_PRE << 8) + VF_ID_BYTE

/** load_font locates the raster file and reads the index of
 * characters, plus whatever other preprocessing is done (depending on
 * the format).  */

unsigned char font::load_font(void)
{
  kDebugInfo(DEBUG, 4300, "loading font %s at %d dpi", fontname, (int) (fsize + 0.5));

  double n_fsize  = fsize;
  int	 dpi      = (int)(fsize + 0.5);
  char	*font_found;
  int	 size_found;
  int	 magic;

  Boolean hushcs	= hush_chk;

  flags |= FONT_LOADED;
  file = font_open(fontname, &font_found, fsize, &size_found, magstepval, &filename);
  if (file == NULL) {
    kDebugError("Can't find font %s.", fontname);
    return True;
  }
  --n_files_left;
  if (font_found != NULL) {
    kDebugError("Can't find font %s; using %s instead at %d dpi.", fontname, font_found, dpi);
    free(fontname);
    fontname = font_found;
    hushcs = True;
  }
  else
    if (!kpse_bitmap_tolerance ((double) size_found, fsize))
      kDebugError("Can't find font %s at %d dpi; using %d dpi instead.", fontname, dpi, size_found);
  fsize      = size_found;
  timestamp  = ++current_timestamp;
  fmaxchar   = maxchar = 255;
  set_char_p = set_char;
  magic      = two(file);

  if (magic == PK_MAGIC)
    read_PK_index(this, WIDENINT hushcs);
  else
    if (magic == GF_MAGIC)
      read_GF_index(this, WIDENINT hushcs);
    else
      if (magic == VF_MAGIC)
	read_VF_index(this, WIDENINT hushcs);
      else
	oops("Cannot recognize format for font file %s", filename);

  if (flags & FONT_VIRTUAL) {
    while (maxchar > 0 && macro[maxchar].pos == NULL)
      --maxchar;
    if (maxchar < 255)
      realloc_font(WIDENINT maxchar);
  } else {
    while (maxchar > 0 && glyph[maxchar].addr == 0)
      --maxchar;
    if (maxchar < 255)
      realloc_font(WIDENINT maxchar);
  }
  return False;
}


/** realloc_font allocates the font structure to contain (newsize + 1)
 * characters (or macros, if the font is a virtual font).  */

void font::realloc_font(unsigned int newsize)
{
  kDebugInfo(DEBUG, 4300, "Realloc font");

  if (flags & FONT_VIRTUAL) {
    struct macro *macrop;

    macrop = macro = (struct macro *) realloc((char *) macro,
					      ((unsigned int) newsize + 1) * sizeof(struct macro));
    if (macrop == NULL)
      oops("! Cannot reallocate space for macro array.");
    if (newsize > fmaxchar)
      bzero((char *) (macro + fmaxchar + 1), (int) (newsize - fmaxchar) * sizeof(struct macro));
  } else {
    struct glyph *glyphp;
    
    glyphp = glyph = (struct glyph *)realloc((char *) glyph,
					     ((unsigned int) newsize + 1) * sizeof(struct glyph));
    if (glyph == NULL) 
      oops("! Cannot reallocate space for glyph array.");
    if (newsize > fmaxchar)
      bzero((char *) (glyph + fmaxchar + 1), (int) (newsize - fmaxchar) * sizeof(struct glyph));
  }
  fmaxchar = newsize;
}


/** mark_as_used marks the font, and all the fonts it referrs to, as
 * used, i.e. their FONT_IN_USE-flag is set. */

void font::mark_as_used(void)
{
  if (flags & FONT_IN_USE) 
    return;

  kDebugInfo(DEBUG, 4300, "marking font %s at %d dpi as used", fontname, (int) (fsize + 0.5));

  flags |= FONT_IN_USE;

  // For virtual fonts, also go through the list of referred fonts
  if (flags & FONT_VIRTUAL) {
    QIntDictIterator<font> it(vf_table);
    while( it.current() ) {
      it.current()->flags |= FONT_IN_USE;
      ++it;
    }
  }
}
