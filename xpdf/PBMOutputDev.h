//========================================================================
//
// PBMOutputDev.h
//
// Copyright 1998-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef PBMOUTPUTDEV_H
#define PBMOUTPUTDEV_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <stddef.h>
#include "config.h"
#include "XOutputDev.h"

//------------------------------------------------------------------------

class PBMOutputDev: public XOutputDev {
public:

  // NB: Users must use makePBMOutputDev and killPBMOutputDev rather
  // than the constructor and destructor.  (This is due to some
  // constraints in the underlying XOutputDev object.)

  static PBMOutputDev *makePBMOutputDev(char *displayName,
					char *fileRootA);

  static void killPBMOutputDev(PBMOutputDev *out);

  virtual ~PBMOutputDev();

  //----- initialization and control

  // Start a page.
  virtual void startPage(int pageNum, GfxState *state);

  // End a page.
  virtual void endPage();

private:

  PBMOutputDev(Display *displayA, int screenA,
	       Pixmap pixmapA, Window dummyWinA,
	       int invertA, char *fileRootA);

  char *fileRoot;
  char *fileName;
  int curPage;

  Display *display;
  int screen;
  Pixmap pixmap;
  Window dummyWin;
  int width, height;
  int invert;
};

#endif
