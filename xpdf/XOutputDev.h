//========================================================================
//
// XOutputDev.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef XOUTPUTDEV_H
#define XOUTPUTDEV_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <stddef.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "config.h"
#include "CharTypes.h"
#include "GlobalParams.h"
#include "OutputDev.h"

class GString;
class GList;
struct GfxRGB;
class GfxFont;
class GfxSubpath;
class TextPage;
class XOutputFontCache;
struct T3FontCacheTag;
class T3FontCache;
struct T3GlyphStack;
class XOutputDev;
class Link;
class Catalog;
class DisplayFontParam;
class UnicodeMap;
class CharCodeToUnicode;

#if HAVE_T1LIB_H
class T1FontEngine;
class T1FontFile;
class T1Font;
#endif

#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
class FTFontEngine;
class FTFontFile;
class FTFont;
#endif

#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
class TTFontEngine;
class TTFontFile;
class TTFont;
#endif

//------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------

#define maxRGBCube 7		// max size of RGB color cube

#define numTmpPoints 256	// number of XPoints in temporary array
#define numTmpSubpaths 16	// number of elements in temporary arrays
				//   for fill/clip

//------------------------------------------------------------------------
// Misc types
//------------------------------------------------------------------------

struct BoundingRect {
  short xMin, xMax;		// min/max x values
  short yMin, yMax;		// min/max y values
};

//------------------------------------------------------------------------
// XOutputFont
//------------------------------------------------------------------------

class XOutputFont {
public:

  XOutputFont(Ref *idA, double m11OrigA, double m12OrigA,
	      double m21OrigA, double m22OrigA,
	      double m11A, double m12A, double m21A, double m22A,
	      Display *displayA, XOutputDev *xOutA);

  virtual ~XOutputFont();

  // Does this font match the ID and transform?
  GBool matches(Ref *idA, double m11OrigA, double m12OrigA,
		double m21OrigA, double m22OrigA)
    { return id.num == idA->num && id.gen == idA->gen &&
	     m11Orig == m11OrigA && m12Orig == m12OrigA &&
	     m21Orig == m21OrigA && m22Orig == m22OrigA; }

  // Was font created successfully?
  virtual GBool isOk() = 0;

  // Update <gc> with this font.
  virtual void updateGC(GC gc) = 0;

  // Draw character <c>/<u> at <x>,<y> (in device space).
  virtual void drawChar(GfxState *state, Pixmap pixmap, int w, int h,
			GC gc, GfxRGB *rgb,
			double x, double y, double dx, double dy,
			CharCode c, Unicode *u, int uLen) = 0;

  // Returns true if this XOutputFont subclass provides the
  // getCharPath function.
  virtual GBool hasGetCharPath() { return gFalse; }

  // Add the character outline for <c>/<u> to the current path.
  virtual void getCharPath(GfxState *state,
			   CharCode c, Unicode *u, int ulen);

protected:

  Ref id;			// font ID
  double m11Orig, m12Orig,	// original transform matrix
         m21Orig, m22Orig;
  double m11, m12, m21, m22;	// actual transform matrix (possibly
				//   modified for font substitution)
  Display *display;		// X display
  XOutputDev *xOut;
};

#if HAVE_T1LIB_H
//------------------------------------------------------------------------
// XOutputT1Font
//------------------------------------------------------------------------

class XOutputT1Font: public XOutputFont {
public:

  XOutputT1Font(Ref *idA, T1FontFile *fontFileA,
		double m11OrigA, double m12OrigA,
		double m21OrigA, double m22OrigA,
		double m11A, double m12A,
		double m21A, double m22A,
		Display *displayA, XOutputDev *xOutA);

  virtual ~XOutputT1Font();

  // Was font created successfully?
  virtual GBool isOk();

  // Update <gc> with this font.
  virtual void updateGC(GC gc);

  // Draw character <c>/<u> at <x>,<y>.
  virtual void drawChar(GfxState *state, Pixmap pixmap, int w, int h,
			GC gc, GfxRGB *rgb,
			double x, double y, double dx, double dy,
			CharCode c, Unicode *u, int uLen);

  // Returns true if this XOutputFont subclass provides the
  // getCharPath function.
  virtual GBool hasGetCharPath() { return gTrue; }

  // Add the character outline for <c>/<u> to the current path.
  virtual void getCharPath(GfxState *state,
			   CharCode c, Unicode *u, int ulen);

private:

  T1FontFile *fontFile;
  T1Font *font;
};
#endif // HAVE_T1LIB_H

#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
//------------------------------------------------------------------------
// XOutputFTFont
//------------------------------------------------------------------------

class XOutputFTFont: public XOutputFont {
public:

  XOutputFTFont(Ref *idA, FTFontFile *fontFileA,
		double m11OrigA, double m12OrigA,
		double m21OrigA, double m22OrigA,
		double m11A, double m12A,
		double m21A, double m22A,
		Display *displayA, XOutputDev *xOutA);

  virtual ~XOutputFTFont();

  // Was font created successfully?
  virtual GBool isOk();

  // Update <gc> with this font.
  virtual void updateGC(GC gc);

  // Draw character <c>/<u> at <x>,<y>.
  virtual void drawChar(GfxState *state, Pixmap pixmap, int w, int h,
			GC gc, GfxRGB *rgb,
			double x, double y, double dx, double dy,
			CharCode c, Unicode *u, int uLen);

  // Returns true if this XOutputFont subclass provides the
  // getCharPath function.
  virtual GBool hasGetCharPath() { return gTrue; }

  // Add the character outline for <c>/<u> to the current path.
  virtual void getCharPath(GfxState *state,
			   CharCode c, Unicode *u, int ulen);

private:

  FTFontFile *fontFile;
  FTFont *font;
};
#endif // FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)

#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
//------------------------------------------------------------------------
// XOutputTTFont
//------------------------------------------------------------------------

class XOutputTTFont: public XOutputFont {
public:

  XOutputTTFont(Ref *idA, TTFontFile *fontFileA,
		double m11OrigA, double m12OrigA,
		double m21OrigA, double m22OrigA,
		double m11A, double m12A,
		double m21A, double m22A,
		Display *displayA, XOutputDev *xOutA);

  virtual ~XOutputTTFont();

  // Was font created successfully?
  virtual GBool isOk();

  // Update <gc> with this font.
  virtual void updateGC(GC gc);

  // Draw character <c>/<u> at <x>,<y>.
  virtual void drawChar(GfxState *state, Pixmap pixmap, int w, int h,
			GC gc, GfxRGB *rgb,
			double x, double y, double dx, double dy,
			CharCode c, Unicode *u, int uLen);

private:

  TTFontFile *fontFile;
  TTFont *font;
};
#endif // !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)

//------------------------------------------------------------------------
// XOutputServer8BitFont
//------------------------------------------------------------------------

class XOutputServer8BitFont: public XOutputFont {
public:

  XOutputServer8BitFont(Ref *idA, GString *xlfdFmt,
			UnicodeMap *xUMapA, CharCodeToUnicode *fontUMap,
			double m11OrigA, double m12OrigA,
			double m21OrigA, double m22OrigA,
			double m11A, double m12A, double m21A, double m22A,
			Display *displayA, XOutputDev *xOutA);

  virtual ~XOutputServer8BitFont();

  // Was font created successfully?
  virtual GBool isOk();

  // Update <gc> with this font.
  virtual void updateGC(GC gc);

  // Draw character <c>/<u> at <x>,<y>.
  virtual void drawChar(GfxState *state, Pixmap pixmap, int w, int h,
			GC gc, GfxRGB *rgb,
			double x, double y, double dx, double dy,
			CharCode c, Unicode *u, int uLen);

private:

  XFontStruct *xFont;		// the X font
  Gushort map[256];		// forward map (char code -> X font code)
  UnicodeMap *xUMap;
};

//------------------------------------------------------------------------
// XOutputServer16BitFont
//------------------------------------------------------------------------

class XOutputServer16BitFont: public XOutputFont {
public:

  XOutputServer16BitFont(Ref *idA, GString *xlfdFmt,
			 UnicodeMap *xUMapA, CharCodeToUnicode *fontUMap,
			 double m11OrigA, double m12OrigA,
			 double m21OrigA, double m22OrigA,
			 double m11A, double m12A, double m21A, double m22A,
			 Display *displayA, XOutputDev *xOutA);

  virtual ~XOutputServer16BitFont();

  // Was font created successfully?
  virtual GBool isOk();

  // Update <gc> with this font.
  virtual void updateGC(GC gc);

  // Draw character <c>/<u> at <x>,<y>.
  virtual void drawChar(GfxState *state, Pixmap pixmap, int w, int h,
			GC gc, GfxRGB *rgb,
			double x, double y, double dx, double dy,
			CharCode c, Unicode *u, int uLen);

private:

  XFontStruct *xFont;		// the X font
  UnicodeMap *xUMap;
};

//------------------------------------------------------------------------
// XOutputFontCache
//------------------------------------------------------------------------

#if HAVE_T1LIB_H
class XOutputT1FontFile {
public:
  XOutputT1FontFile(int numA, int genA, GBool substA,
		    T1FontFile *fontFileA, GString *tmpFileNameA)
    { num = numA; gen = genA; subst = substA;
      fontFile = fontFileA; tmpFileName = tmpFileNameA; }
  ~XOutputT1FontFile();
  int num, gen;
  GBool subst;
  T1FontFile *fontFile;
  GString *tmpFileName;
};
#endif

#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
class XOutputFTFontFile {
public:
  XOutputFTFontFile(int numA, int genA, GBool substA,
		    FTFontFile *fontFileA, GString *tmpFileNameA)
    { num = numA; gen = genA; subst = substA;
      fontFile = fontFileA; tmpFileName = tmpFileNameA; }
  ~XOutputFTFontFile();
  int num, gen;
  GBool subst;
  FTFontFile *fontFile;
  GString *tmpFileName;
};
#endif

#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
class XOutputTTFontFile {
public:
  XOutputTTFontFile(int numA, int genA, GBool substA,
		    TTFontFile *fontFileA, GString *tmpFileNameA)
    { num = numA; gen = genA; subst = substA;
      fontFile = fontFileA; tmpFileName = tmpFileNameA; }
  ~XOutputTTFontFile();
  int num, gen;
  GBool subst;
  TTFontFile *fontFile;
  GString *tmpFileName;
};
#endif

class XOutputFontCache {
public:

  // Constructor.
  XOutputFontCache(Display *displayA, Guint depthA,
		   XOutputDev *xOutA,
		   FontRastControl t1libControlA,
		   FontRastControl freetypeControlA);

  // Destructor.
  ~XOutputFontCache();

  // Initialize (or re-initialize) the font cache for a new document.
  void startDoc(int screenNum, Visual *visual,
		Colormap colormap, GBool trueColor,
		int rMul, int gMul, int bMul,
		int rShift, int gShift, int bShift,
		Gulong *colors, int numColors);

  // Get a font.  This creates a new font if necessary.
  XOutputFont *getFont(XRef *xref, GfxFont *gfxFont, double m11, double m12,
		       double m21, double m22);

private:

  void delFonts();
  void clear();
  XOutputFont *tryGetFont(XRef *xref, DisplayFontParam *dfp, GfxFont *gfxFont,
			  double m11Orig, double m12Orig,
			  double m21Orig, double m22Orig,
			  double m11, double m12, double m21, double m22,
			  GBool subst);
#if HAVE_T1LIB_H
  XOutputFont *tryGetT1Font(XRef *xref, GfxFont *gfxFont,
			    double m11, double m12, double m21, double m22);
  XOutputFont *tryGetT1FontFromFile(XRef *xref, GString *fileName,
				    GBool deleteFile, GfxFont *gfxFont,
				    double m11Orig, double m12Orig,
				    double m21Orig, double m22Orig,
				    double m11, double m12,
				    double m21, double m22, GBool subst);
#endif
#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  XOutputFont *tryGetFTFont(XRef *xref, GfxFont *gfxFont,
			    double m11, double m12, double m21, double m22);
  XOutputFont *tryGetFTFontFromFile(XRef *xref, GString *fileName,
				    GBool deleteFile, GfxFont *gfxFont,
				    double m11Orig, double m12Orig,
				    double m21Orig, double m22Orig,
				    double m11, double m12,
				    double m21, double m22,
				    GBool embedded, GBool subst);
#endif
#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  XOutputFont *tryGetTTFont(XRef *xref, GfxFont *gfxFont,
			    double m11, double m12, double m21, double m22);
  XOutputFont *tryGetTTFontFromFile(XRef *xref, GString *fileName,
				    GBool deleteFile, GfxFont *gfxFont,
				    double m11Orig, double m12Orig,
				    double m21Orig, double m22Orig,
				    double m11, double m12,
				    double m21, double m22, GBool subst);
#endif
  XOutputFont *tryGetServerFont(GString *xlfd, GString *encodingName,
				GfxFont *gfxFont,
				double m11Orig, double m12Orig,
				double m21Orig, double m22Orig,
				double m11, double m12,
				double m21, double m22);

  Display *display;		// X display pointer
  XOutputDev *xOut;
  Guint depth;			// pixmap depth

  XOutputFont *
    fonts[xOutFontCacheSize];
  int nFonts;

#if HAVE_T1LIB_H
  FontRastControl t1libControl;	// t1lib settings
  T1FontEngine *t1Engine;	// Type 1 font engine
  GList *t1FontFiles;		// list of Type 1 font files
				//   [XOutputT1FontFile]
#endif

#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H
  FontRastControl		// FreeType settings
    freetypeControl;
#endif
#if FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  FTFontEngine *ftEngine;	// FreeType font engine
  GList *ftFontFiles;		// list of FreeType font files
				//   [XOutputFTFontFile]
#endif
#if !FREETYPE2 && (HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H)
  TTFontEngine *ttEngine;	// TrueType font engine
  GList *ttFontFiles;		// list of TrueType font files
				//   [XOutputTTFontFile]
#endif
};

//------------------------------------------------------------------------
// XOutputState
//------------------------------------------------------------------------

struct XOutputState {
  GC strokeGC;
  GC fillGC;
  Region clipRegion;
  XOutputState *next;
};

//------------------------------------------------------------------------
// XOutputDev
//------------------------------------------------------------------------

class XOutputDev: public OutputDev {
public:

  // Constructor.
  XOutputDev(Display *displayA, int screenNumA,
	     Visual *visualA, Colormap colormapA,
	     GBool reverseVideoA, unsigned long paperColorA,
	     GBool installCmap, int rgbCubeSize,
	     int forceDepth = 0);

  // Destructor.
  virtual ~XOutputDev();

  //---- get info about output device

  // Does this device use upside-down coordinates?
  // (Upside-down means (0,0) is the top left corner of the page.)
  virtual GBool upsideDown() { return gTrue; }

  // Does this device use drawChar() or drawString()?
  virtual GBool useDrawChar() { return gTrue; }

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  virtual GBool interpretType3Chars() { return gTrue; }

  //----- initialization and control

  // Start a page.
  virtual void startPage(int pageNum, GfxState *state);

  // End a page.
  virtual void endPage();

  //----- link borders
  virtual void drawLink(Link *link, Catalog *catalog);

  //----- save/restore graphics state
  virtual void saveState(GfxState *state);
  virtual void restoreState(GfxState *state);

  //----- update graphics state
  virtual void updateAll(GfxState *state);
  virtual void updateCTM(GfxState *state, double m11, double m12,
			 double m21, double m22, double m31, double m32);
  virtual void updateLineDash(GfxState *state);
  virtual void updateFlatness(GfxState *state);
  virtual void updateLineJoin(GfxState *state);
  virtual void updateLineCap(GfxState *state);
  virtual void updateMiterLimit(GfxState *state);
  virtual void updateLineWidth(GfxState *state);
  virtual void updateFillColor(GfxState *state);
  virtual void updateStrokeColor(GfxState *state);

  //----- update text state
  virtual void updateFont(GfxState *state);

  //----- path painting
  virtual void stroke(GfxState *state);
  virtual void fill(GfxState *state);
  virtual void eoFill(GfxState *state);

  //----- path clipping
  virtual void clip(GfxState *state);
  virtual void eoClip(GfxState *state);

  //----- text drawing
  virtual void beginString(GfxState *state, GString *s);
  virtual void endString(GfxState *state);
  virtual void drawChar(GfxState *state, double x, double y,
			double dx, double dy,
			double originX, double originY,
			CharCode code, Unicode *u, int uLen);
  virtual GBool beginType3Char(GfxState *state,
			       CharCode code, Unicode *u, int uLen);
  virtual void endType3Char(GfxState *state);

  //----- image drawing
  virtual void drawImageMask(GfxState *state, Object *ref, Stream *str,
			     int width, int height, GBool invert,
			     GBool inlineImg);
  virtual void drawImage(GfxState *state, Object *ref, Stream *str,
			 int width, int height, GfxImageColorMap *colorMap,
			 int *maskColors, GBool inlineImg);

  //----- Type 3 font operators
  virtual void type3D0(GfxState *state, double wx, double wy);
  virtual void type3D1(GfxState *state, double wx, double wy,
		       double llx, double lly, double urx, double ury);

  //----- special access

  // Called to indicate that a new PDF document has been loaded.
  void startDoc(XRef *xrefA);

  // Find a string.  If <top> is true, starts looking at <xMin>,<yMin>;
  // otherwise starts looking at top of page.  If <bottom> is true,
  // stops looking at <xMax>,<yMax>; otherwise stops looking at bottom
  // of page.  If found, sets the text bounding rectange and returns
  // true; otherwise returns false.
  GBool findText(Unicode *s, int len, GBool top, GBool bottom,
		 int *xMin, int *yMin, int *xMax, int *yMax);

  // Get the text which is inside the specified rectangle.
  GString *getText(int xMin, int yMin, int xMax, int yMax);

  GBool isReverseVideo() { return reverseVideo; }

  // Update pixmap ID after a page change.
  void setPixmap(Pixmap pixmapA, int pixmapWA, int pixmapHA)
    { pixmap = pixmapA; pixmapW = pixmapWA; pixmapH = pixmapHA; }

  // Get the off-screen pixmap, its size, various display info.
  Pixmap getPixmap() { return pixmap; }
  int getPixmapWidth() { return pixmapW; }
  int getPixmapHeight() { return pixmapH; }
  Display *getDisplay() { return display; }
  Guint getDepth() { return depth; }

  Gulong findColor(GfxRGB *rgb);

private:

  XRef *xref;			// the xref table for this PDF file
  Display *display;		// X display pointer
  int screenNum;		// X screen number
  Pixmap pixmap;		// pixmap to draw into
  int pixmapW, pixmapH;		// size of pixmap
  Guint depth;			// pixmap depth
  Visual *visual;		// X visual
  Colormap colormap;		// X colormap
  int flatness;			// line flatness
  GC paperGC;			// GC for background
  GC strokeGC;			// GC with stroke color
  GC fillGC;			// GC with fill color
  Region clipRegion;		// clipping region
  GBool trueColor;		// set if using a TrueColor visual
  int rMul, gMul, bMul;		// RGB multipliers (for TrueColor)
  int rShift, gShift, bShift;	// RGB shifts (for TrueColor)
  Gulong			// color cube
    colors[maxRGBCube * maxRGBCube * maxRGBCube];
  unsigned long paperColor;	// paper color (pixel value)
  int numColors;		// size of color cube
  double redMap[256];		// map pixel (from color cube) to red value
  GBool reverseVideo;		// reverse video mode
  XPoint			// temporary points array
    tmpPoints[numTmpPoints];
  int				// temporary arrays for fill/clip
    tmpLengths[numTmpSubpaths];
  BoundingRect
    tmpRects[numTmpSubpaths];
  GfxFont *gfxFont;		// current PDF font
  XOutputFont *font;		// current font
  GBool needFontUpdate;		// set when the font needs to be updated
  XOutputFontCache *fontCache;	// font cache
  T3FontCache *			// Type 3 font cache
    t3FontCache[xOutT3FontCacheSize];
  int nT3Fonts;			// number of valid entries in t3FontCache
  T3GlyphStack *t3GlyphStack;	// Type 3 glyph context stack
  XOutputState *save;		// stack of saved states

  TextPage *text;		// text from the current page

  void updateLineAttrs(GfxState *state, GBool updateDash);
  void doFill(GfxState *state, int rule);
  void doClip(GfxState *state, int rule);
  int convertPath(GfxState *state, XPoint **points, int *size,
		  int *numPoints, int **lengths, GBool fillHack);
  void convertSubpath(GfxState *state, GfxSubpath *subpath,
		      XPoint **points, int *size, int *n);
  void doCurve(XPoint **points, int *size, int *k,
	       double x0, double y0, double x1, double y1,
	       double x2, double y2, double x3, double y3);
  void addPoint(XPoint **points, int *size, int *k, int x, int y);
  void drawType3Glyph(T3FontCache *t3Font,
		      T3FontCacheTag *tag, Guchar *data,
		      double x, double y, GfxRGB *color);
  Gulong findColor(GfxRGB *x, GfxRGB *err);
};

#endif
