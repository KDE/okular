//========================================================================
//
// FoFiType1.h
//
// Copyright 1999-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef FOFITYPE1_H
#define FOFITYPE1_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "gtypes.h"
#include "FoFiBase.h"

//------------------------------------------------------------------------
// FoFiType1
//------------------------------------------------------------------------

class FoFiType1: public FoFiBase {
public:

  // Create a FoFiType1 object from a memory buffer.
  static FoFiType1 *make(char *fileA, int lenA);

  // Create a FoFiType1 object from a file on disk.
  static FoFiType1 *load(char *fileName);

  virtual ~FoFiType1();

  // Return the font name.
  const char *getName();

  // Return the encoding, as an array of 256 names (any of which may
  // be NULL).
  const char **getEncoding();

  // Write a version of the Type 1 font file with a new encoding.
  void writeEncoded(char **newEncoding,
		    FoFiOutputFunc outputFunc, void *outputStream);

private:

  FoFiType1(char *fileA, int lenA, GBool freeFileDataA);

  char *getNextLine(char *line);
  void parse();

  const char *name;
  const char **encoding;
  GBool parsed;
};

#endif
