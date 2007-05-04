// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
// TeXFont.h
//
// Part of KDVI - A DVI previewer for the KDE desktop environment
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#ifndef _TEXFONT_H
#define _TEXFONT_H

#include "glyph.h"
#include "TeXFontDefinition.h"


class TeXFont {
 public:
  TeXFont(TeXFontDefinition *_parent)
    {
      parent       = _parent;
      errorMessage.clear();
    }

  virtual ~TeXFont();

  void setDisplayResolution()
    {
      for(unsigned int i=0; i<TeXFontDefinition::max_num_of_chars_in_font; i++)
        glyphtable[i].shrunkenCharacter = QImage();
    }

  virtual glyph* getGlyph(quint16 character, bool generateCharacterPixmap=false, const QColor& color=Qt::black) = 0;

  // Checksum of the font. Used e.g. by PK fonts. This field is filled
  // in by the constructor, or set to 0.0, if the font format does not
  // contain checksums.
  quint32           checksum;

  // If the font or if some glyphs could not be loaded, error messages
  // will be put here.
  QString            errorMessage;

 protected:
  glyph              glyphtable[TeXFontDefinition::max_num_of_chars_in_font];
  TeXFontDefinition *parent;
};

#endif
