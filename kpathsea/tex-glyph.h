/* tex-glyph.h: look for a TeX glyph font (GF or PK).

Copyright (C) 1993 Karl Berry.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef KPATHSEA_TEX_GLYPH_H
#define KPATHSEA_TEX_GLYPH_H

#include <kpathsea/tex-file.h>


/* This type describes the origination of a glyph font.  */

typedef enum
{
  kpse_glyph_source_normal,  /* the searched-for font: already existed */
  kpse_glyph_source_alias,   /* : was an alias for an existing file */
  kpse_glyph_source_maketex, /* : was created on the fly */
  kpse_glyph_source_fallback /* : wasn't found, but the fallback font was */
} kpse_glyph_source_type;


typedef struct
{
  string name;			/* font name found */
  unsigned dpi;			/* size found, for glyphs */
  kpse_file_format_type format;	/* glyph format found */
  kpse_glyph_source_type source;	/* where we found it */
} kpse_glyph_file_type;		

#define KPSE_GLYPH_FILE_NAME(f) ((f).name)
#define KPSE_GLYPH_FILE_DPI(f) ((f).dpi)
#define KPSE_GLYPH_FILE_FORMAT(f) ((f).format)
#define KPSE_GLYPH_FILE_SOURCE(f) ((f).source)


/* Search first for the font named FONT_NAME at resolution DPI in the
   glyph format FORMAT (see `try_size' for details of format searching).
   Then try resolutions within KPSE_BITMAP_TOLERANCE of DPI.  Then try
   the resolutions in `kpse_fallback_sizes', then within the tolerance
   of each of those.  Then if FONT_NAME is an alias defined in a
   texfonts.map do all the above for its real name.  Then try the above
   for kpse_fallback_name.  Then fail.  Return either the filename
   found, or NULL.  Also return information about the file found in
   *GLYPH_FILE.  */
extern KPSEDLL string kpse_find_glyph P4H(const_string font_name, unsigned dpi, 
                                  kpse_file_format_type format,
                                  kpse_glyph_file_type *glyph_file);

/* Look for a specific format only.  */
#define kpse_find_pk(font_name, dpi, glyph_file) \
  kpse_find_glyph (font_name, dpi, kpse_pk_format, glyph_file)
#define kpse_find_gf(font_name, dpi, glyph_file) \
  kpse_find_glyph (font_name, dpi, kpse_gf_format, glyph_file)


/* Defines how far away a pixel file can be found from its stated size.
   The DVI standard says any resolution within 0.2% of the stated size
   is ok, but we are more forgiving.  */
#define KPSE_BITMAP_TOLERANCE(r) ((r) / 500.0 + 1)

/* Check whether DPI1 is within KPSE_BITMAP_TOLERANCE of DPI2. */
extern KPSEDLL boolean kpse_bitmap_tolerance P2H(double dpi1, double dpi2);

#endif /* not KPATHSEA_TEX_GLYPH_H */
