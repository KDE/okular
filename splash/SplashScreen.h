//========================================================================
//
// SplashScreen.h
//
//========================================================================

#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "SplashTypes.h"

//------------------------------------------------------------------------
// SplashScreen
//------------------------------------------------------------------------

class SplashScreen {
public:

  SplashScreen(int sizeA);
  ~SplashScreen();

  SplashScreen *copy() { return new SplashScreen(size << 1); }

  // Return the computed pixel value (0=black, 1=white) for the gray
  // level <value> at (<x>, <y>).
  int test(int x, int y, SplashCoord value);

private:

  SplashCoord *mat;		// threshold matrix
  int size;			// size of the threshold matrix
};

#endif
