//========================================================================
//
// PBMOutputDev.cc
//
// Copyright 1998-2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "gmem.h"
#include "GString.h"
#include "Object.h"
#include "Stream.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "Error.h"
#include "PBMOutputDev.h"

//------------------------------------------------------------------------

PBMOutputDev *PBMOutputDev::makePBMOutputDev(char *displayName,
					     char *fileRootA) {
  Display *displayA;
  Pixmap pixmapA;
  Window dummyWinA;
  int screenA;
  int invertA;
  unsigned long black, white;
  PBMOutputDev *out;

  if (!(displayA = XOpenDisplay(displayName))) {
    fprintf(stderr, "Couldn't open display '%s'\n", displayName);
    exit(1);
  }
  screenA = DefaultScreen(displayA);

  black = BlackPixel(displayA, screenA);
  white = WhitePixel(displayA, screenA);
  if ((black & 1) == (white & 1)) {
    fprintf(stderr, "Weird black/white pixel colors\n");
    XCloseDisplay(displayA);
    return NULL;
  } 
  invertA = (white & 1) == 1 ? 0xff : 0x00;

  dummyWinA = XCreateSimpleWindow(displayA, RootWindow(displayA, screenA),
				  0, 0, 1, 1, 0,
				  black, white);
  pixmapA = XCreatePixmap(displayA, dummyWinA, 1, 1, 1);
  out = new PBMOutputDev(displayA, screenA, pixmapA, dummyWinA,
			 invertA, fileRootA);
  return out;
}

void PBMOutputDev::killPBMOutputDev(PBMOutputDev *out) {
  Display *displayA;
  Pixmap pixmapA;
  Window dummyWinA;

  displayA = out->display;
  pixmapA = out->pixmap;
  dummyWinA = out->dummyWin;

  delete out;

  // these have to be done *after* the XOutputDev (parent of the
  // PBMOutputDev) is deleted, since XOutputDev::~XOutputDev() needs
  // them
  XFreePixmap(displayA, pixmapA);
  XDestroyWindow(displayA, dummyWinA);
  XCloseDisplay(displayA);
}

PBMOutputDev::PBMOutputDev(Display *displayA, int screenA,
			   Pixmap pixmapA, Window dummyWinA,
			   int invertA, char *fileRootA):
  XOutputDev(displayA, screenA,
	     DefaultVisual(displayA, screenA),
	     DefaultColormap(displayA, screenA),
	     gFalse,
	     WhitePixel(displayA, DefaultScreen(displayA)),
	     gFalse, 1, 1)
{
  display = displayA;
  screen = screenA;
  pixmap = pixmapA;
  dummyWin = dummyWinA;
  invert = invertA;
  fileRoot = fileRootA;
  fileName = (char *)gmalloc(strlen(fileRoot) + 20);
}

PBMOutputDev::~PBMOutputDev() {
  gfree(fileName);
}

void PBMOutputDev::startPage(int pageNum, GfxState *state) {
  curPage = pageNum;
  width = (int)(state->getPageWidth() + 0.5);
  height = (int)(state->getPageHeight() + 0.5);
  XFreePixmap(display, pixmap);
  pixmap = XCreatePixmap(display, dummyWin, width, height, 1);
  setPixmap(pixmap, width, height);
  XOutputDev::startPage(pageNum, state);
}

void PBMOutputDev::endPage() {
  XImage *image;
  FILE *f;
  int p;
  int x, y, i;

  image = XCreateImage(display, DefaultVisual(display, screen),
		       1, ZPixmap, 0, NULL, width, height, 8, 0);
  image->data = (char *)gmalloc(height * image->bytes_per_line);
  XGetSubImage(display, pixmap, 0, 0, width, height, 1, ZPixmap,
	       image, 0, 0);

  sprintf(fileName, "%s-%06d.pbm", fileRoot, curPage);
  if (!(f = fopen(fileName, "wb"))) {
    fprintf(stderr, "Couldn't open output file '%s'\n", fileName);
    goto err;
  }
  fprintf(f, "P4\n");
  fprintf(f, "%d %d\n", width, height);

  for (y = 0; y < height; ++y) {
    for (x = 0; x+8 <= width; x += 8) {
      p = 0;
      for (i = 0; i < 8; ++i)
	p = (p << 1) + (XGetPixel(image, x+i, y) & 1);
      p ^= invert;
      fputc((char)p, f);
    }
    if (width & 7) {
      p = 0;
      for (i = 0; i < (width & 7); ++i)
	p = (p << 1) + (XGetPixel(image, x+i, y) & 1);
      p <<= 8 - (width & 7);
      p ^= invert;
      fputc((char)p, f);
    }
  }

  fclose(f);

 err:
  gfree(image->data);
  image->data = NULL;
  XDestroyImage(image);

  XOutputDev::endPage();
}
