// TeXFont_PFB.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

// Add header files alphabetically


#ifndef _TEXFONT_PFB_H
#define _TEXFONT_PFB_H

#include <qstring.h>

#include "TeXFont.h"

class glyph;

class TeXFont_PFB : public TeXFont {
 public:
  TeXFont_PFB(TeXFontDefinition *parent);
  ~TeXFont_PFB();
  
  glyph *getGlyph(unsigned int character, bool generateCharacterPixmap=false);

 private:
};

#endif
