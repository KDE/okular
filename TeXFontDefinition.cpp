// $Id$

#include <kdebug.h>
#include <klocale.h>
#include <qfile.h>

#include "../config.h"
#include "fontpool.h"
#include "kdvi.h"
#ifdef HAVE_FREETYPE
#include "TeXFont_PFB.h"
#endif
#include "TeXFont_PK.h"
#include "TeXFont_TFM.h"
#include "TeXFontDefinition.h"
#include "xdvi.h"

extern const int MFResolutions[];

#define	PK_PRE		247
#define	PK_ID		89
#define	PK_MAGIC	(PK_PRE << 8) + PK_ID
#define	GF_PRE		247
#define	GF_ID		131
#define	GF_MAGIC	(GF_PRE << 8) + GF_ID
#define	VF_PRE		247
#define	VF_ID_BYTE	202
#define	VF_MAGIC	(VF_PRE << 8) + VF_ID_BYTE



TeXFontDefinition::TeXFontDefinition(QString nfontname, double _displayResolution_in_dpi, Q_UINT32 chk, Q_INT32 _scaled_size_in_DVI_units,
	   class fontPool *pool, double _enlargement)
{
#ifdef DEBUG_FONT
  kdDebug(4300) << "TeXFontDefinition::TeXFontDefinition(...); fontname=" << nfontname << ", enlargement=" << _enlargement << endl;
#endif

  enlargement              = _enlargement;
  font_pool                = pool;
  fontname                 = nfontname;
  font                     = 0;
  displayResolution_in_dpi = _displayResolution_in_dpi;
  checksum                 = chk;
  flags                    = TeXFontDefinition::FONT_IN_USE;
  file                     = 0; 
  filename                 = QString::null;
  scaled_size_in_DVI_units = _scaled_size_in_DVI_units;
  
  macrotable               = 0;
  
  // By default, this font contains only empty characters. After the
  // font has been loaded, this function pointer will be replaced by
  // another one.
  set_char_p  = &dviWindow::set_empty_char;
}


TeXFontDefinition::~TeXFontDefinition()
{
#ifdef DEBUG_FONT
  kdDebug(4300) << "discarding font " << fontname << " at " << (int)(enlargement * MFResolutions[font_pool->getMetafontMode()] + 0.5) << " dpi" << endl;
#endif
  
  if (font != 0) {
    delete font;
    font = 0;
  }
  if (macrotable != 0) {
    delete [] macrotable;
    macrotable = 0;
  }

  if (flags & FONT_LOADED) {
    if (file != 0) {
      fclose(file);
      file = 0;
    }
    if (flags & FONT_VIRTUAL)
      vf_table.clear();
  }
}


void TeXFontDefinition::fontNameReceiver(QString fname)
{
#ifdef DEBUG_FONT
  kdDebug(4300) << "void TeXFontDefinition::fontNameReceiver( " << fname << " )" << endl;
#endif

  flags |= TeXFontDefinition::FONT_LOADED;
  filename = fname;

  file = fopen(QFile::encodeName(filename), "r");
  if (file == NULL) {
    kdError(4300) << i18n("Can't find font ") << fontname << "." << endl;
    return;
  }
  set_char_p = &dviWindow::set_char;
  int magic      = two(file);

#ifdef HAVE_FREETYPE
  if (fname.endsWith(".pfb")) {
      fclose(file);
      file = 0;
      font = new TeXFont_PFB(this);
      set_char_p = &dviWindow::set_char;
      return;
    }
#endif

  if (fname.endsWith("pk"))
    if (magic == PK_MAGIC) {
      fclose(file);
      file = 0;
      font = new TeXFont_PK(this);
      set_char_p = &dviWindow::set_char;
      if ((font->checksum != 0) && (checksum != font->checksum))
	kdWarning(4300) << i18n("Checksum mismatch for font file %1").arg(filename) << endl;
      return;
    }
    
  if (fname.endsWith(".vf"))  
    if (magic == VF_MAGIC) {
      read_VF_index();
      set_char_p = &dviWindow::set_vf_char;
      return;
    }

  if (fname.endsWith(".tfm")) {
      fclose(file);
      file = 0;
      font = new TeXFont_TFM(this);
      set_char_p = &dviWindow::set_char;
      return;
  }
  
  kdError(4300) << i18n("Cannot recognize format for font file %1").arg(filename) << endl; // @@@ Do sth smarter here
}


void TeXFontDefinition::reset(void)
{
  if (font != 0) {
    delete font;
    font = 0;
  }

  if (macrotable != 0) {
    delete [] macrotable;
    macrotable = 0;
  }
  
  if (flags & FONT_LOADED) {
    if (file != 0) {
      fclose(file);
      file = 0;
    }
    if (flags & FONT_VIRTUAL)
      vf_table.clear();
  }
  
  filename   = QString::null;
  flags      = TeXFontDefinition::FONT_IN_USE;
  set_char_p = &dviWindow::set_empty_char;
}


void TeXFontDefinition::setDisplayResolution(double _displayResolution_in_dpi)
{
  displayResolution_in_dpi = _displayResolution_in_dpi;
  if (font != 0)
    font->setDisplayResolution();
}


/** mark_as_used marks the font, and all the fonts it referrs to, as
    used, i.e. their FONT_IN_USE-flag is set. */

void TeXFontDefinition::mark_as_used(void)
{
#ifdef DEBUG_FONT
  kdDebug(4300) << "TeXFontDefinition::mark_as_used(void)" << endl;
#endif

  if (flags & TeXFontDefinition::FONT_IN_USE)
    return;

  flags |= TeXFontDefinition::FONT_IN_USE;

  // For virtual fonts, also go through the list of referred fonts
  if (flags & TeXFontDefinition::FONT_VIRTUAL) {
    QIntDictIterator<TeXFontDefinition> it(vf_table);
    while( it.current() ) {
      it.current()->flags |= TeXFontDefinition::FONT_IN_USE;
      ++it;
    }
  }
}


macro::macro()
{
  pos     = 0;		/* address of first byte of macro */
  end     = 0;		/* address of last+1 byte */
  dvi_advance_in_units_of_design_size_by_2e20 = 0;	/* DVI units to move reference point */
  free_me = false;
}


macro::~macro()
{
  if ((pos != 0L) && (free_me == true))
    delete [] pos;
}
