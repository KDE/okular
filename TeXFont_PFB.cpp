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

  int error = FT_New_Face( parent->font_pool->FreeType_library, parent->filename.local8Bit(), 0, &face );
     
  if ( error == FT_Err_Unknown_File_Format ) {
    kdError(4300) << "The font file " << parent->filename << " could be opened and read, but it appears that its font format is unsupported." << endl;
    exit(0); // @@@
  } else 
    if ( error ) {
      kdError(4300) << "The font file " << parent->filename << " is broken, or it could not be opened or read." << endl;
      exit(0); // @@@
    }

  /*
  int has_encoding = 0;
  for (int i = 0; i < face->num_charmaps; ++i)
    for (int j = 0; accepted_platform_ids[j] != -1; ++j)
      if (face->charmaps[i]->platform_id == accepted_platform_ids[j] && face->charmaps[i]->encoding_id == accepted_encoding_ids[j]){
	FT_Set_Charmap (face, face->charmaps[i]);
	has_encoding = 1;
	goto done_encoding;
      }
 done_encoding:
  if (!has_encoding)
    kdError(4300) << "The font file " << parent->filename << " has no good encoding." << endl;
  */  
}


TeXFont_PFB::~TeXFont_PFB()
{
}


glyph *TeXFont_PFB::getGlyph(unsigned int ch, bool generateCharacterPixmap)
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
  
  if ((generateCharacterPixmap == true) && (g->shrunkenCharacter.isNull())) {
    unsigned int res =  (unsigned int)(parent->displayResolution_in_dpi+0.5);
    double characterSize_in_printers_points_by_64 = (long int)(parent->scaled_size_in_DVI_units * 64.0 / (1<<16) + 0.5); // Only approximate, may vary from file to file!!!! @@@@@

    int error = FT_Set_Char_Size(face,    /* handle to face object           */
				 0,       /* char_width in 1/64th of points  */
				 characterSize_in_printers_points_by_64,   /* char_height in 1/64th of points (reminder: 1 pt = 1/72 inch) */
				 res,     /* horizontal device resolution    */
				 res );   /* vertical device resolution      */
    if (error)
      kdError(4300) << "B" << endl;
    
    // load glyph image into the slot and erase the previous one
    //    error = FT_Load_Glyph(face, ch, FT_LOAD_NO_HINTING );
    error = FT_Load_Glyph(face, ch, 0 ); // @@@
    //error = FT_Load_Glyph(face, FT_Get_Char_Index(face, ch),  FT_LOAD_NO_HINTING );
    if (error)
      kdError(4300) << "Cannot load glyph #" << ch << " for font " << parent->filename << endl;
    
    // convert to an anti-aliased bitmap
    error = FT_Render_Glyph( face->glyph, ft_render_mode_normal );
    if (error)
      kdError(4300) << "D" << endl;
    
    FT_GlyphSlot slot = face->glyph;
    
    QImage imgi(slot->bitmap.width, slot->bitmap.rows, 32);
    imgi.setAlphaBuffer(true);
    uchar *srcScanLine = slot->bitmap.buffer;
    for(int row=0; row<slot->bitmap.rows; row++) {
      uchar *destScanLine = imgi.scanLine(row);
      for(int col=0; col<slot->bitmap.width; col++) {
	destScanLine[4*col+0] = 0;
	destScanLine[4*col+1] = 0;
	destScanLine[4*col+2] = 0;
	destScanLine[4*col+3] = srcScanLine[col];
      }
      srcScanLine += slot->bitmap.pitch;
    }
    g->shrunkenCharacter.convertFromImage ( imgi, 0);

    if (g->shrunkenCharacter.isNull()) {
      kdError() << "Width  = " << g->shrunkenCharacter.width() << " & " << slot->bitmap.width <<  endl;
      kdError() << "Height = " << g->shrunkenCharacter.height() << " & " << slot->bitmap.rows <<  endl;
      kdError() << "face->glyph->metrics.horiAdvance = " << face->glyph->metrics.horiAdvance <<  endl;

      g->shrunkenCharacter.resize( 5, 5 );
      g->shrunkenCharacter.fill(QColor(255, 100, 100));
    }

    g->x2 = -slot->bitmap_left;
    g->y2 = slot->bitmap_top;

    // load glyph image into the slot and erase the previous one
    error = FT_Load_Glyph(face, ch, FT_LOAD_NO_SCALE );
    if (error)
      kdError(4300) << "E" << endl;
    
    g->dvi_advance_in_units_of_design_size_by_2e20 =  ((Q_INT32)(1<<20) * (Q_INT32)face->glyph->metrics.horiAdvance) / (Q_INT32)face->units_per_EM;
  }
  
  return g;
}

#endif // HAVE_FREETYPE
