
/* glyph.cpp
 *
 * part of kdvi, a dvi-previewer for the KDE desktop environement
 *
 * written by Stefan Kebekus, originally based on code by Paul Vojta
 * and a large number of co-authors */

#include "glyph.h"

glyph::glyph() 
{
  bitmap.bits = 0;
}

glyph::~glyph()
{
  if (bitmap.bits != 0L)
    delete [] bitmap.bits;
}
