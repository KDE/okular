// TeXFont_PFB.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

// Add header files alphabetically


#include <kdebug.h>
#include <klocale.h>

#include "glyph.h"
#include "TeXFont_PFB.h"


TeXFont_PFB::TeXFont_PFB(TeXFontDefinition *parent) 
  : TeXFont(parent)
{
#ifdef DEBUG_PFB
  kdDebug(4300) << "TeXFont_PFB::TeXFont_PFB( parent=" << parent << ")" << endl;
#endif
}


TeXFont_PFB::~TeXFont_PFB()
{
}


glyph *TeXFont_PFB::getGlyph(unsigned int ch, bool generateCharacterPixmap)
{
#ifdef DEBUG_PFB
  kdDebug(4300) << "TeXFont_PFB::getGlyph( ch=" << ch << ", generateCharacterPixmap=" << generateCharacterPixmap << " )" << endl;
#endif

  // Paranoia checks
  if (ch >= TeXFontDefinition::max_num_of_chars_in_font) {
    kdError(4300) << "TeXFont_PFB::getGlyph(): Argument is too big." << endl;
    return glyphtable;
  }
  
  // This is the address of the glyph that will be returned.
  struct glyph *g = glyphtable+ch;

  if ((generateCharacterPixmap == true) && (g->shrunkenCharacter.isNull())) {
    g->shrunkenCharacter.resize(15,15);
    g->shrunkenCharacter.fill(Qt::red);
    g->x2 = 0;
    g->y2 = 15;
  }

  return g;
}
