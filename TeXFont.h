// -*- C++ -*-
// TeXFont.h
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#ifndef _TEXFONT_H
#define _TEXFONT_H

#include "TeXFontDefinition.h"
#include "glyph.h"


class TeXFont {
 public:
  TeXFont(TeXFontDefinition *_parent)
    {
      parent       = _parent;
      errorMessage = QString::null;
    };
  
  virtual ~TeXFont();
  
  void setDisplayResolution()
    {
      for(unsigned int i=0; i<TeXFontDefinition::max_num_of_chars_in_font; i++)
	glyphtable[i].shrunkenCharacter.resize(0, 0);
    };
  
  virtual glyph* getGlyph(Q_UINT16 character, bool generateCharacterPixmap=false, const QColor& color=Qt::black) = 0;

  // Checksum of the font. Used e.g. by PK fonts. This field is filled
  // in by the constructor, or set to 0.0, if the font format does not
  // contain checksums.
  Q_UINT32           checksum;

  // If the font or if some glyphs could not be loaded, error messages
  // will be put here.
  QString            errorMessage;
   
 protected:
  glyph              glyphtable[TeXFontDefinition::max_num_of_chars_in_font];
  TeXFontDefinition *parent;
};

#endif
