//========================================================================
//
// LTKOutputDev.cc
//
// Copyright 1998-2002 Glyph & Cog, LLC
//
//========================================================================

#ifdef __GNUC__
#pragma implementation
#endif

#include <aconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "gmem.h"
#include "GString.h"
#include "LTKWindow.h"
#include "LTKScrollingCanvas.h"
#include "Object.h"
#include "Stream.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "Error.h"
#include "LTKOutputDev.h"

//------------------------------------------------------------------------

LTKOutputDev::LTKOutputDev(LTKWindow *winA, GBool reverseVideoA,
			   unsigned long paperColor, GBool installCmap,
			   GBool rgbCubeSize, GBool incrementalUpdateA):
  XOutputDev(winA->getDisplay(),
	     ((LTKScrollingCanvas *)winA->findWidget("canvas"))->getPixmap(),
	     0, winA->getColormap(), reverseVideoA, paperColor,
	     installCmap, rgbCubeSize)
{
  win = winA;
  canvas = (LTKScrollingCanvas *)win->findWidget("canvas");
  setPixmap(canvas->getPixmap(),
	    canvas->getRealWidth(), canvas->getRealHeight());
  incrementalUpdate = incrementalUpdateA;
}

LTKOutputDev::~LTKOutputDev() {
}

void LTKOutputDev::startPage(int pageNum, GfxState *state) {
  canvas->resize((int)(state->getPageWidth() + 0.5),
		 (int)(state->getPageHeight() + 0.5));
  setPixmap(canvas->getPixmap(),
	    canvas->getRealWidth(), canvas->getRealHeight());
  XOutputDev::startPage(pageNum, state);
  if (incrementalUpdate) {
    canvas->redraw();
  }
}

void LTKOutputDev::endPage() {
  if (!incrementalUpdate) {
    canvas->redraw();
  }
  XOutputDev::endPage();
}

void LTKOutputDev::dump() {
  if (incrementalUpdate) {
    canvas->redraw();
  }
  XOutputDev::dump();
}
