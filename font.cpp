
#define DEBUG 0

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

extern FILE *xfopen(const char *filename, const char *type);

/** We try for a VF first because that's what dvips does.  Also, it's
 *  easier to avoid running MakeTeXPK if we have a VF this way.  */

FILE *font::font_open (char *font, char **font_ret, double dpi, int *dpi_ret, char **filename_ret)
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
  ret           = name ? xfopen(name, FOPEN_R_MODE) : NULL;
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
  flags      = font::FONT_IN_USE;
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
      for (macro *m = macrotable; m < macrotable + max_num_of_chars_in_font; ++m)
	if (m->free_me)
	  free((char *) m->pos);
      free((char *) macrotable);
      vf_table.clear();
    } else {
      for (glyph *g = glyphtable; g < glyphtable + max_num_of_chars_in_font; ++g)
	delete g;
      free((char *) glyphtable);
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
    characters, plus whatever other preprocessing is done (depending
    on the format).  */

unsigned char font::load_font(void)
{
  kDebugInfo(DEBUG, 4300, "loading font %s at %d dpi", fontname, (int) (fsize + 0.5));

  int	 dpi      = (int)(fsize + 0.5);
  char	*font_found;
  int	 size_found;
  int	 magic;

  Boolean hushcs	= hush_chk;

  flags |= font::FONT_LOADED;
  file = font_open(fontname, &font_found, (double)fsize, &size_found, &filename);
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
  set_char_p = set_char;
  magic      = two(file);

  if (magic == PK_MAGIC)
    read_PK_index(WIDENINT hushcs);
  else
    if (magic == GF_MAGIC)
      oops("The GF format for font file %s is no longer supported", filename);
    else
      if (magic == VF_MAGIC)
	read_VF_index(WIDENINT hushcs);
      else
	oops("Cannot recognize format for font file %s", filename);

  return False;
}



/** mark_as_used marks the font, and all the fonts it referrs to, as
    used, i.e. their FONT_IN_USE-flag is set. */

void font::mark_as_used(void)
{
  if (flags & font::FONT_IN_USE) 
    return;

  kDebugInfo(DEBUG, 4300, "marking font %s at %d dpi as used", fontname, (int) (fsize + 0.5));

  flags |= font::FONT_IN_USE;

  // For virtual fonts, also go through the list of referred fonts
  if (flags & font::FONT_VIRTUAL) {
    QIntDictIterator<font> it(vf_table);
    while( it.current() ) {
      it.current()->flags |= font::FONT_IN_USE;
      ++it;
    }
  }
}

/** Returns a pointer to glyph number ch in the font, or NULL, if this
    number does not exist. This function first reads the bitmap of the
    character from the PK-file, if necessary */

struct glyph *font::glyphptr(unsigned int ch) {
  
  struct glyph *g = glyphtable+ch;
  if (g->bitmap.bits == NULL) {
    if (g->addr == 0) {
      kDebugError(1,4300,"Character %d not defined in font %s", ch, fontname);
      g->addr = -1;
      return NULL;
    }
    if (g->addr == -1)
      return NULL;	/* previously flagged missing char */

    if (file == NULL) {
      file = xfopen(filename, OPEN_MODE);
      if (file == NULL) {
	oops("Font file disappeared: %s", filename);
	return NULL;
      }
    }
    Fseek(file, g->addr, 0);
    read_PK_char(ch);
    timestamp = ++current_timestamp;

    if (g->bitmap.bits == NULL) {
      g->addr = -1;
      return NULL;
    }
  }

  return g;
}
