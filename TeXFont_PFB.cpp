// TeXFont_PFB.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

// This file is compiled only if the FreeType library is present on
// the system

#include "../config.h"
#ifdef HAVE_FREETYPE


// Add header files alphabetically

#include <kdebug.h>
#include <klocale.h>
#include <qimage.h>

#include "fontpool.h"
#include "glyph.h"
#include "TeXFont_PFB.h"

//#define DEBUG_PFB


TeXFont_PFB::TeXFont_PFB(TeXFontDefinition *parent) 
  : TeXFont(parent)
{
#ifdef DEBUG_PFB
  kdDebug(4300) << "TeXFont_PFB::TeXFont_PFB( parent=" << parent << ")" << endl;
#endif

  fatalErrorInFontLoading = false;

  int error = FT_New_Face( parent->font_pool->FreeType_library, parent->filename.local8Bit(), 0, &face );
  
  if ( error == FT_Err_Unknown_File_Format ) {
    errorMessage = i18n("The font file %1 could be opened and read, but its font format is unsupported.").arg(parent->filename);
    kdError(4300) << errorMessage << endl;
    fatalErrorInFontLoading = true;
    return;
  } else 
    if ( error ) {
      errorMessage = i18n("The font file %1 is broken, or it could not be opened or read.").arg(parent->filename);
      kdError(4300) << errorMessage << endl;
      fatalErrorInFontLoading = true;
      return;
    }

  //  kdDebug() << "Encodings of " <<  parent->filename << endl;
  FT_CharMap  charmap;
  int         n;
  for ( n = 0; n < face->num_charmaps; n++ ) {
    charmap = face->charmaps[n];
    //    kdDebug() << " Platform " << charmap->platform_id << endl;
    //    kdDebug() << " Encoding " << charmap->encoding_id << endl;
    
    if ((charmap->platform_id == 7)|| (charmap->encoding_id == 2))
      FT_Set_Charmap( face, charmap );
  }
}


TeXFont_PFB::~TeXFont_PFB()
{
  FT_Done_Face( face );
}


glyph *TeXFont_PFB::getGlyph(Q_UINT16 ch, bool generateCharacterPixmap, QColor color)
{
#ifdef DEBUG_PFB
  kdDebug(4300) << "TeXFont_PFB::getGlyph( ch=" << ch << ", '" << (char)(ch) << "', generateCharacterPixmap=" << generateCharacterPixmap << " )" << endl;
#endif
  
  // Paranoia checks
  if (ch >= TeXFontDefinition::max_num_of_chars_in_font) {
    kdError(4300) << "TeXFont_PFB::getGlyph(): Argument is too big." << endl;
    return glyphtable;
  }

  // This is the address of the glyph that will be returned.
  struct glyph *g = glyphtable+ch;


  if (fatalErrorInFontLoading == true)
    return g;
  
  if ((generateCharacterPixmap == true) && ((g->shrunkenCharacter.isNull()) || (color != g->color)) ) {
    int error;
    unsigned int res =  (unsigned int)(parent->displayResolution_in_dpi/parent->enlargement +0.5);
    g->color = color;

    // Character height in 1/64th of points (reminder: 1 pt = 1/72 inch)
    // Only approximate, may vary from file to file!!!! @@@@@
    long int characterSize_in_printers_points_by_64 = (long int)(parent->scaled_size_in_DVI_units * 64.0 / (1<<16) + 0.5); 
    error = FT_Set_Char_Size(face, 0, characterSize_in_printers_points_by_64, res, res );
    if (error) {
      QString msg = i18n("FreeType reported an error when setting the character size for font file %1.").arg(parent->filename);
      if (errorMessage.isEmpty())
	errorMessage = msg;
      kdError(4300) << msg << endl;
      g->shrunkenCharacter.resize(1,1);
      g->shrunkenCharacter.fill(QColor(255, 255, 255));
      return g;
    }
    
    // load glyph image into the slot and erase the previous one
    if (parent->font_pool->getUseFontHints() == true)
      error = FT_Load_Glyph(face, FT_Get_Char_Index( face, ch ), FT_LOAD_DEFAULT ); 
    else
      error = FT_Load_Glyph(face, FT_Get_Char_Index( face, ch ), FT_LOAD_NO_HINTING );
    if (error) {
      QString msg = i18n("FreeType is unable to load glyph #%1 from font file %2.").arg(ch).arg(parent->filename);
      if (errorMessage.isEmpty())
	errorMessage = msg;
      kdError(4300) << msg << endl;
      g->shrunkenCharacter.resize(1,1);
      g->shrunkenCharacter.fill(QColor(255, 255, 255));
      return g;
    }
    
    // convert to an anti-aliased bitmap
    error = FT_Render_Glyph( face->glyph, ft_render_mode_normal );
    if (error) {
      QString msg = i18n("FreeType is unable to render glyph #%1 from font file %2.").arg(ch).arg(parent->filename);
      if (errorMessage.isEmpty())
	errorMessage = msg;
      kdError(4300) << msg << endl;
      g->shrunkenCharacter.resize(1,1);
      g->shrunkenCharacter.fill(QColor(255, 255, 255));
      return g;
    }
    
    FT_GlyphSlot slot = face->glyph;

    if ((slot->bitmap.width == 0) || (slot->bitmap.rows == 0)) {
      QString msg = i18n("Glyph #%1 from font file %2 is empty.").arg(ch).arg(parent->filename);
      if (errorMessage.isEmpty())
	errorMessage = msg;
      kdError(4300) << msg << endl;
      g->shrunkenCharacter.resize( 15, 15 );
      g->shrunkenCharacter.fill(QColor(255, 0, 0));
      g->x2 = 0;
      g->y2 = 15;
    } else {
      QImage imgi(slot->bitmap.width, slot->bitmap.rows, 32);
      imgi.setAlphaBuffer(true);
      uchar *srcScanLine = slot->bitmap.buffer;
      for(int row=0; row<slot->bitmap.rows; row++) {
	uchar *destScanLine = imgi.scanLine(row);
	for(int col=0; col<slot->bitmap.width; col++) {
	  destScanLine[4*col+0] = color.blue();
	  destScanLine[4*col+1] = color.green();
	  destScanLine[4*col+2] = color.red();
	  destScanLine[4*col+3] = srcScanLine[col];
	}
	srcScanLine += slot->bitmap.pitch;
      }
      g->shrunkenCharacter.convertFromImage ( imgi, 0);
      g->x2 = -slot->bitmap_left;
      g->y2 = slot->bitmap_top;
    }
  }
  
  // Load glyph width, if that hasn't been done yet.
  if (g->dvi_advance_in_units_of_design_size_by_2e20 == 0) {
    int error = FT_Load_Glyph(face, FT_Get_Char_Index( face, ch ), FT_LOAD_NO_SCALE );
    if (error) {
      QString msg = i18n("FreeType is unable to load metric for glyph #%1 from font file %2.").arg(ch).arg(parent->filename);
      if (errorMessage.isEmpty())
	errorMessage = msg;
      kdError(4300) << msg << endl;
      g->dvi_advance_in_units_of_design_size_by_2e20 =  1;
    }
    g->dvi_advance_in_units_of_design_size_by_2e20 =  ((Q_INT32)(1<<20) * (Q_INT32)face->glyph->metrics.horiAdvance) / (Q_INT32)face->units_per_EM;
  }
  
  return g;
}

#endif // HAVE_FREETYPE
