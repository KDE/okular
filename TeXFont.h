
#ifndef _TEXFONT_H
#define _TEXFONT_H

#include <qglobal.h>
#include <qstring.h>

class glyph;
class QPixmap;

#define max_num_of_chars_in_font 256

class TeXFont {
 public:
  TeXFont(QString _filename, double _displayResolution_in_dpi, double _fontResolution_in_dpi=0.0) :
    checksum(0), filename(_filename), displayResolution_in_dpi(_displayResolution_in_dpi), fontResolution_in_dpi(_fontResolution_in_dpi) 
    {;};
  virtual ~TeXFont();

  void setDisplayResolution(double _displayResolution_in_dpi)
    {
      displayResolution_in_dpi = _displayResolution_in_dpi;
      for(unsigned int i=0; i<max_num_of_chars_in_font; i++)
	glyphtable[i].shrunkenCharacter.resize(0, 0);
    };

  virtual glyph *getGlyph(unsigned int character, bool generateCharacterPixmap=false) =0;

 protected:
  // Checksum of the font. Used e.g. by PK fonts. This field is filled
  // in by the constructor, or set to 0.0, if the font format does not
  // contain checksums.
  Q_UINT32 checksum;

  glyph   glyphtable[max_num_of_chars_in_font];
  QString filename;
  double  displayResolution_in_dpi;
  double  fontResolution_in_dpi;
};

#endif
