/* font-open.c: find font filenames.  This bears no relation (but the
   interface) to the original font_open.c, so I renamed it.  */


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

/* We try for a VF first because that's what dvips does.  Also, it's
   easier to avoid running MakeTeXPK if we have a VF this way.  */

FILE *font_open (char *font, char **font_ret, double dpi, int *dpi_ret, int dummy, char **filename_ret)
{
  FILE *ret;
  char *name = kpse_find_vf (font);
  
  if (name)
    {
      /* VF fonts don't have a resolution, but loadfont will complain if
         we don't return what it asked for.  */
      *dpi_ret = dpi;
      *font_ret = NULL;
    }
  else
    {
      kpse_glyph_file_type file_ret;
      name = kpse_find_glyph (font, (unsigned) (dpi + .5), kpse_any_glyph_format, &file_ret);
      if (name)
        {
          /* If we got it normally, from an alias, or from MakeTeXPK,
             don't fill in FONT_RET.  That tells load_font to complain.  */
          *font_ret
             = file_ret.source == kpse_glyph_source_fallback ? file_ret.name
               : NULL; /* tell load_font we found something good */
          
          *dpi_ret = file_ret.dpi;
        }
      /* If no VF and no PK, FONT_RET is irrelevant? */
    }
  
  /* If we found a name, return the stream.  */
  ret = name ? xfopen (name, FOPEN_R_MODE) : NULL;
  *filename_ret = name;

  return ret;
}
