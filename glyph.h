// -*- C++ -*-

#ifndef _GLYPH_H
#define _GLYPH_H

#include <qcolor.h>
#include <qpixmap.h>


struct bitmap {
  Q_UINT16	w, h;	/* width and height in pixels */
  Q_UINT16	bytes_wide;	/* scan-line width in bytes */
  char		*bits;		/* pointer to the bits */
};

class glyph {
 public:
  glyph();
  ~glyph();

  // address of bitmap in font file
  long    addr;

  QColor color;

  // DVI units to move reference point
  Q_INT32 dvi_advance_in_units_of_design_size_by_2e20;

  // x and y offset in pixels 
  short   x, y;

  QPixmap shrunkenCharacter;

  short   x2, y2;	/* x and y offset in pixels (shrunken bitmap) */
};

#endif //ifndef _GLYPH_H
