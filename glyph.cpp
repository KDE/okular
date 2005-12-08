// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-

/* glyph.cpp
 *
 * part of kdvi, a dvi-previewer for the KDE desktop environement
 *
 * written by Stefan Kebekus, originally based on code by Paul Vojta
 * and a large number of co-authors */

#include <config.h>

#include "glyph.h"
#include "kvs_debug.h"


glyph::glyph()
{
#ifdef DEBUG_GLYPH
  kdDebug(kvs::dvi) << "glyph::glyph()" << endl;
#endif

  addr                     = 0;
  x                        = 0;
  y                        = 0;
  dvi_advance_in_units_of_design_size_by_2e20 = 0;
}

glyph::~glyph()
{}
