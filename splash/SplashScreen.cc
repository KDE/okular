//========================================================================
//
// SplashScreen.cc
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include "gmem.h"
#include "SplashMath.h"
#include "SplashScreen.h"

//------------------------------------------------------------------------
// SplashScreen
//------------------------------------------------------------------------

// This generates a 45 degree screen using a circular dot spot
// function.  DPI = resolution / ((size / 2) * sqrt(2)).
// Gamma correction (gamma = 1 / 1.33) is also computed here.
SplashScreen::SplashScreen(int sizeA) {
  SplashCoord *dist;
  SplashCoord u, v, d;
  int x, y, x1, y1, i;

  size = sizeA >> 1;
  if (size < 1) {
    size = 1;
  }

  // initialize the threshold matrix
  mat = (SplashCoord *)gmalloc(2 * size * size * sizeof(SplashCoord));
  for (y = 0; y < 2 * size; ++y) {
    for (x = 0; x < size; ++x) {
      mat[y * size + x] = -1;
    }
  }

  // build the distance matrix
  dist = (SplashCoord *)gmalloc(2 * size * size * sizeof(SplashCoord));
  for (y = 0; y < size; ++y) {
    for (x = 0; x < size; ++x) {
      if (x + y < size - 1) {
	u = (SplashCoord)x + 0.5 - 0;  //~ (-0.5);
	v = (SplashCoord)y + 0.5 - 0;
      } else {
	u = (SplashCoord)x + 0.5 - (SplashCoord)size; //~ ((SplashCoord)size - 0.5);
	v = (SplashCoord)y + 0.5 - (SplashCoord)size;
      }
      dist[y * size + x] = u*u + v*v;
    }
  }
  for (y = 0; y < size; ++y) {
    for (x = 0; x < size; ++x) {
      if (x < y) {
	u = (SplashCoord)x + 0.5 - 0;  //~ (-0.5);
	v = (SplashCoord)y + 0.5 - (SplashCoord)size;
      } else {
	u = (SplashCoord)x + 0.5 - (SplashCoord)size; //~ ((SplashCoord)size - 0.5);
	v = (SplashCoord)y + 0.5 - 0;
      }
      dist[(size + y) * size + x] = u*u + v*v;
    }
  }

  // build the threshold matrix
  x1 = y1 = 0; // make gcc happy
  for (i = 1; i <= 2 * size * size; ++i) {
    d = 2 * size * size;
    for (y = 0; y < 2 * size; ++y) {
      for (x = 0; x < size; ++x) {
	if (mat[y * size + x] < 0 &&
	    dist[y * size + x] < d) {
	  x1 = x;
	  y1 = y;
	  d = dist[y1 * size + x1];
	}
      }
    }
    u = 1.0 - (SplashCoord)i / (SplashCoord)(2 * size * size + 1);
    mat[y1 * size + x1] = splashPow(u, 1.33);
  }

  gfree(dist);
}

SplashScreen::~SplashScreen() {
  gfree(mat);
}

int SplashScreen::test(int x, int y, SplashCoord value) {
  SplashCoord *mat1;
  int xx, yy;

  xx = x % (2 * size);
  yy = y % (2 * size);
  mat1 = mat;
  if ((xx / size) ^ (yy / size)) {
    mat1 += size * size;
  }
  xx %= size;
  yy %= size;
  return value < mat1[yy * size + xx] ? 0 : 1;
}
