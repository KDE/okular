// TeXFont_PFB.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

// This file is compiled only if the FreeType library is present on
// the system

#include <../config.h>
#ifdef HAVE_FREETYPE
#ifndef FT_FREETYPE_H
#undef HAVE_FREETYPE
#else

#ifndef _TEXFONT_PFB_H
#define _TEXFONT_PFB_H

// Add header files alphabetically

#include <ft2build.h>
#include FT_FREETYPE_H
#include <qstring.h>

#include "TeXFont.h"

class glyph;

class TeXFont_PFB : public TeXFont {
 public:
  TeXFont_PFB(TeXFontDefinition *parent, fontEncoding *enc=0);
  ~TeXFont_PFB();
  
  glyph *getGlyph(Q_UINT16 character, bool generateCharacterPixmap=false, QColor color=Qt::black);

 private:
  FT_Face       face;
  bool          fatalErrorInFontLoading;
  Q_UINT16      charMap[256];
};

#endif
#endif
#endif // HAVE_FREETYPE
