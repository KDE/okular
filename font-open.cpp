/* font-open.c: find font filenames.  This bears no relation (but the
   interface) to the original font_open.c, so I renamed it.  */

#include <malloc.h>
#include <stdio.h>

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



extern int n_files_left;


font::~font()
{
  //@@@  if (_debug & DBG_PK)
  kDebugInfo("xdvi: Discarding font \"%s\" at %d dpi\n",fontname,(int)(fsize + 0.5));

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
      free((char *) vf_table);

      tn *tnp = vf_chain;
      while (tnp != NULL) {
	register struct tn *tnp1 = tnp->next;
	free((char *) tnp);
	tnp = tnp1;
      }
    } else {
      struct glyph *g;
      
      for (g = glyph; g <= glyph + maxchar; ++g)
	delete g;
      
      free((char *) glyph);
    }
  }
}



/** realloc_font allocates the font structure to contain (newsize + 1)
 * characters (or macros, if the font is a virtual font).  */

void font::realloc_font(unsigned int newsize)
{
  if (flags & FONT_VIRTUAL) {
    struct macro *macrop;

    macrop = macro = (struct macro *) realloc((char *) macro,
					      ((unsigned int) newsize + 1) * sizeof(struct macro));
    if (macrop == NULL)
      oops("! Cannot reallocate space for macro array.");
    if (newsize > maxchar)
      bzero((char *) (macro + maxchar + 1), (int) (newsize - maxchar) * sizeof(struct macro));
  } else {
    struct glyph *glyphp;
    
    glyphp = glyph = (struct glyph *)realloc((char *) glyph,
					     ((unsigned int) newsize + 1) * sizeof(struct glyph));
    if (glyph == NULL) 
      oops("! Cannot reallocate space for glyph array.");
    if (newsize > maxchar)
      bzero((char *) (glyph + maxchar + 1), (int) (newsize - maxchar) * sizeof(struct glyph));
  }
  maxchar = newsize;
}
