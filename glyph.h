// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-

#ifndef _GLYPH_H
#define _GLYPH_H

#include <qcolor.h>

#include <cairo.h>


struct bitmap {
  // width and height in pixels
  Q_UINT16 w, h;
  // scan-line width in bytes
  Q_UINT16 bytes_wide;
  // pointer to the bits
  char* bits;
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

  cairo_surface_t* shrunkenCharacter;

  // x and y offset in pixels (shrunken bitmap)
  short   x2, y2;

  int width;
  int height;
};

#endif //ifndef _GLYPH_H
