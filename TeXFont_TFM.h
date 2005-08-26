// -*- C++ -*-
// TeXFont_TFM.h
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#ifndef _TEXFONT_TFM_H
#define _TEXFONT_TFM_H

// Add header files alphabetically

#include <qstring.h>

#include "TeXFont.h"

class glyph;


class fix_word {
 public:
  void fromINT32(Q_INT32 val) {value = val;} 
  void fromDouble(double val) {value = (Q_INT32)(val * (1<<20) + 0.5);}
  double toDouble() {return (double(value)) / (double(1<<20));}

  Q_INT32 value;
};

class TeXFont_TFM : public TeXFont {
 public:
  TeXFont_TFM(TeXFontDefinition *parent);
  ~TeXFont_TFM();
  
  glyph *getGlyph(Q_UINT16 character, bool generateCharacterPixmap=false, QColor color=Qt::black);

 private:
  fix_word characterWidth_in_units_of_design_size[256];
  fix_word characterHeight_in_units_of_design_size[256];

  fix_word design_size_in_TeX_points;
};

#endif
