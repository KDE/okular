//========================================================================
//
// TextOutputDev.h
//
// Copyright 1997-2002 Glyph & Cog, LLC
//
//========================================================================

#ifndef TEXTOUTPUTDEV_H
#define TEXTOUTPUTDEV_H

#ifdef __GNUC__
#pragma interface
#endif

#include <stdio.h>
#include "gtypes.h"
#include "GfxFont.h"
#include "OutputDev.h"

class GfxState;
class GString;

//------------------------------------------------------------------------

typedef void (*TextOutputFunc)(void *stream, char *text, int len);

//------------------------------------------------------------------------
// TextString
//------------------------------------------------------------------------

class TextString {
public:

  // Constructor.
  TextString(GfxState *state, double fontSize);

  // Destructor.
  ~TextString();

  // Add a character to the string.
  void addChar(GfxState *state, double x, double y,
	       double dx, double dy, Unicode u);

private:

  double xMin, xMax;		// bounding box x coordinates
  double yMin, yMax;		// bounding box y coordinates
  int col;			// starting column
  Unicode *text;		// the text
  double *xRight;		// right-hand x coord of each char
  int len;			// length of text and xRight
  int size;			// size of text and xRight arrays
  TextString *yxNext;		// next string in y-major order
  TextString *xyNext;		// next string in x-major order

  friend class TextPage;
};

//------------------------------------------------------------------------
// TextPage
//------------------------------------------------------------------------

class TextPage {
public:

  // Constructor.
  TextPage(GBool rawOrderA);

  // Destructor.
  ~TextPage();

  // Update the current font.
  void updateFont(GfxState *state);

  // Begin a new string.
  void beginString(GfxState *state);

  // Add a character to the current string.
  void addChar(GfxState *state, double x, double y,
	       double dx, double dy, Unicode *u, int uLen);

  // End the current string, sorting it into the list of strings.
  void endString();

  // Coalesce strings that look like parts of the same line.
  void coalesce();

  // Find a string.  If <top> is true, starts looking at top of page;
  // otherwise starts looking at <xMin>,<yMin>.  If <bottom> is true,
  // stops looking at bottom of page; otherwise stops looking at
  // <xMax>,<yMax>.  If found, sets the text bounding rectange and
  // returns true; otherwise returns false.
  GBool findText(Unicode *s, int len,
		 GBool top, GBool bottom,
		 double *xMin, double *yMin,
		 double *xMax, double *yMax);

  // Get the text which is inside the specified rectangle.
  GString *getText(double xMin, double yMin,
		   double xMax, double yMax);

  // Dump contents of page to a file.
  void dump(void *outputStream, TextOutputFunc outputFunc);

  // Clear the page.
  void clear();

private:

  GBool rawOrder;		// keep strings in content stream order

  TextString *curStr;		// currently active string
  double fontSize;		// current font size

  TextString *yxStrings;	// strings in y-major order
  TextString *xyStrings;	// strings in x-major order
  TextString *yxCur1, *yxCur2;	// cursors for yxStrings list

  int nest;			// current nesting level (for Type 3 fonts)
};

//------------------------------------------------------------------------
// TextOutputDev
//------------------------------------------------------------------------

class TextOutputDev: public OutputDev {
public:

  // Open a text output file.  If <fileName> is NULL, no file is
  // written (this is useful, e.g., for searching text).  If
  // <rawOrder> is true, the text is kept in content stream order.
  TextOutputDev(char *fileName, GBool rawOrderA, GBool append);

  // Create a TextOutputDev which will write to a generic stream.  If
  // <rawOrder> is true, the text is kept in content stream order.
  TextOutputDev(TextOutputFunc func, void *stream, GBool rawOrderA);

  // Destructor.
  virtual ~TextOutputDev();

  // Check if file was successfully created.
  virtual GBool isOk() { return ok; }

  //---- get info about output device

  // Does this device use upside-down coordinates?
  // (Upside-down means (0,0) is the top left corner of the page.)
  virtual GBool upsideDown() { return gTrue; }

  // Does this device use drawChar() or drawString()?
  virtual GBool useDrawChar() { return gTrue; }

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  virtual GBool interpretType3Chars() { return gFalse; }

  // Does this device need non-text content?
  virtual GBool needNonText() { return gFalse; }

  //----- initialization and control

  // Start a page.
  virtual void startPage(int pageNum, GfxState *state);

  // End a page.
  virtual void endPage();

  //----- update text state
  virtual void updateFont(GfxState *state);

  //----- text drawing
  virtual void beginString(GfxState *state, GString *s);
  virtual void endString(GfxState *state);
  virtual void drawChar(GfxState *state, double x, double y,
			double dx, double dy,
			double originX, double originY,
			CharCode c, Unicode *u, int uLen);

  //----- special access

  // Find a string.  If <top> is true, starts looking at top of page;
  // otherwise starts looking at <xMin>,<yMin>.  If <bottom> is true,
  // stops looking at bottom of page; otherwise stops looking at
  // <xMax>,<yMax>.  If found, sets the text bounding rectange and
  // returns true; otherwise returns false.
  GBool findText(Unicode *s, int len,
		 GBool top, GBool bottom,
		 double *xMin, double *yMin,
		 double *xMax, double *yMax);

private:

  TextOutputFunc outputFunc;	// output function
  void *outputStream;		// output stream
  GBool needClose;		// need to close the output file?
				//   (only if outputStream is a FILE*)
  TextPage *text;		// text for the current page
  GBool rawOrder;		// keep text in content stream order
  GBool ok;			// set up ok?
};

#endif
