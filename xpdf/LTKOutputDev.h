//========================================================================
//
// LTKOutputDev.h
//
// Copyright 1998-2002 Glyph & Cog, LLC
//
//========================================================================

#ifndef LTKOUTPUTDEV_H
#define LTKOUTPUTDEV_H

#ifdef __GNUC__
#pragma interface
#endif

#include <stddef.h>
#include "config.h"
#include "XOutputDev.h"

class LTKApp;
class LTKWindow;

//------------------------------------------------------------------------

class LTKOutputDev: public XOutputDev {
public:

  LTKOutputDev(LTKWindow *winA, GBool reverseVideoA,
	       unsigned long paperColor, GBool installCmap,
	       GBool rgbCubeSize, GBool incrementalUpdateA);

  ~LTKOutputDev();

  //----- initialization and control

  // Start a page.
  virtual void startPage(int pageNum, GfxState *state);

  // End a page.
  virtual void endPage();

  // Dump page contents to display.
  virtual void dump();

private:

  LTKWindow *win;		// window
  LTKScrollingCanvas *canvas;	// drawing canvas
  GBool incrementalUpdate;	// incrementally update the display?
};

#endif
