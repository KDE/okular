


#include <kdebug.h>
#include <klocale.h>
#include <malloc.h>
#include <qapplication.h>
#include <stdio.h>

#include "dviwin.h"
#include "font.h"
#include "kdvi.h"

#include "oconfig.h"

extern FILE *xfopen(const char *filename, const char *type);
extern void oops(QString message);
extern int n_files_left;

#define	PK_PRE		247
#define	PK_ID		89
#define	PK_MAGIC	(PK_PRE << 8) + PK_ID
#define	GF_PRE		247
#define	GF_ID		131
#define	GF_MAGIC	(GF_PRE << 8) + GF_ID
#define	VF_PRE		247
#define	VF_ID_BYTE	202
#define	VF_MAGIC	(VF_PRE << 8) + VF_ID_BYTE

#define DEBUG_FONT 1

void font::font_name_receiver(KProcess *, char *buffer, int buflen)
{
  if (buflen < 3)
    return;

  flags |= font::FONT_LOADED;

  filename = QString::fromLocal8Bit(buffer, buflen-1);

  kdDebug() << "FONT NAME RECEIVED:" << filename << endl;

  file = xfopen(filename.latin1(), "r");
  if (file == NULL) {
    kdError() << i18n("Can't find font ") << fontname << "." << endl;
    return;
  }

  --n_files_left;
  timestamp  = ++current_timestamp;
  set_char_p = &dviWindow::set_char;
  int magic      = two(file);

  if (magic == PK_MAGIC) {
    // Achtung! Read_PK_index kann auch schiefgehen!
    read_PK_index();
    set_char_p = &dviWindow::set_char;
  } else
    if (magic == GF_MAGIC)
      oops(QString(i18n("The GF format for font file %1 is no longer supported")).arg(filename) );
    else
      if (magic == VF_MAGIC) {
	read_VF_index();
	set_char_p = &dviWindow::set_vf_char;
      } else
	oops(QString(i18n("Cannot recognize format for font file %1")).arg(filename) );
}


font::font(char *nfontname, float nfsize, long chk, int mag, double dconv)
{
#ifdef DEBUG_FONT
  kdDebug() << "constructing font " << nfontname << " at " << (int) (nfsize + 0.5) << " dpi" << endl;
#endif

  fontname   = nfontname;
  fsize      = nfsize;
  checksum   = chk;
  magstepval = mag;
  flags      = font::FONT_IN_USE || font::FONT_NEEDS_TO_BE_LOADED;
  file       = NULL; 
  filename   = "";
  dimconv    = dconv;
  
  // By default, this font contains only empty characters. After the
  // font has been loaded, this function pointer will be replaced by another one.
  set_char_p = &dviWindow::set_empty_char;
}

font::~font()
{
#ifdef DEBUG_FONT
  kdDebug() << "discarding font " << fontname << " at " << (int)(fsize + 0.5) << " dpi" << endl;
#endif

  free(fontname);

  if (flags & FONT_LOADED) {
    if (file != NULL) {
      fclose(file);
      ++n_files_left;
    }

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





/** mark_as_used marks the font, and all the fonts it referrs to, as
    used, i.e. their FONT_IN_USE-flag is set. */

void font::mark_as_used(void)
{
#ifdef DEBUG_FONT
  kdDebug() << "marking font " << fontname << " at " << (int) (fsize + 0.5) << " dpi" << endl;
#endif

  if (flags & font::FONT_IN_USE)
    return;

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
      kdError() << i18n("Character %1 not defined in font %2").arg(ch).arg(fontname) << endl;
      g->addr = -1;
      return NULL;
    }
    if (g->addr == -1)
      return NULL;	/* previously flagged missing char */

    if (file == NULL) {
      file = xfopen(filename.latin1(), "r");
      if (file == NULL) {
	oops(QString(i18n("Font file disappeared: %1")).arg(filename) );
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
