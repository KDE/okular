//========================================================================
//
// SplashBitmap.h
//
//========================================================================

#ifndef SPLASHBITMAP_H
#define SPLASHBITMAP_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "SplashTypes.h"

//------------------------------------------------------------------------
// SplashBitmap
//------------------------------------------------------------------------

class SplashBitmap {
public:

  // Create a new bitmap.
  SplashBitmap(int widthA, int heightA, SplashColorMode modeA);

  ~SplashBitmap();

  int getWidth() { return width; }
  int getHeight() { return height; }
  int getRowSize() { return rowSize; }
  SplashColorMode getMode() { return mode; }
  SplashColorPtr getDataPtr() { return data; }

  SplashError writePNMFile(char *fileName);

private:

  int width, height;		// size of bitmap
  int rowSize;			// size of one row of data, in bytes
  SplashColorMode mode;		// color mode
  SplashColorPtr data;

  friend class Splash;
};

#endif
