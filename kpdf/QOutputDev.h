
//========================================================================
//
// XOutputDev.h
//
// Copyright 1996 Derek B. Noonburg
//
//========================================================================

#ifndef QOUTPUTDEV_H
#define QOUTPUTDEV_H

#ifdef __GNUC__
#pragma interface
#endif

#include "aconf.h"
#include <stddef.h>

class Object;

#include <qobject.h>

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
class Link;
class Catalog;
class DisplayFontParam;
class UnicodeMap;
class CharCodeToUnicode;

class QPainter;
class QPixmap;
class QPointArray;

typedef double fp_t;

//------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------


//------------------------------------------------------------------------
// Misc types
//------------------------------------------------------------------------


//------------------------------------------------------------------------
// XOutputDev
//------------------------------------------------------------------------

class QOutputDev : public QObject, public OutputDev {
	Q_OBJECT

public:

	// Constructor.
	QOutputDev( QPainter * p = 0 );

	// Destructor.
	virtual ~QOutputDev();

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

	//----- link borders
	virtual void drawLink(Link *link, Catalog *catalog);

	//----- save/restore graphics state
	virtual void saveState(GfxState *state);
	virtual void restoreState(GfxState *state);

	//----- update graphics state
	virtual void updateAll(GfxState *state);
	virtual void updateCTM(GfxState *state, fp_t m11, fp_t m12,
			 fp_t m21, fp_t m22, fp_t m31, fp_t m32);
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
	virtual void drawChar(GfxState *state, fp_t x, fp_t y,
	                      fp_t dx, fp_t dy,
	                      fp_t originX, fp_t originY,
	                      CharCode code, Unicode *u, int uLen);

	//----- image drawing
	virtual void drawImageMask(GfxState *state, Object *ref, Stream *str,
	                          int width, int height, GBool invert,
	                          GBool inlineImg);
	virtual void drawImage(GfxState *state, Object *ref, Stream *str,
	                       int width, int height, GfxImageColorMap *colorMap,
	                       int *maskColors, GBool inlineImg);

	// Find a string.  If <top> is true, starts looking at <l>,<t>;
	// otherwise starts looking at top of page.  If <bottom> is true,
	// stops looking at <l+w-1>,<t+h-1>; otherwise stops looking at bottom
	// of page.  If found, sets the text bounding rectange and returns
	// true; otherwise returns false.
	GBool findText ( Unicode *s, int len, GBool top, GBool bottom, int *xMin, int *yMin, int *xMax, int *yMax );

	//----- special QT access

	bool findText ( const QString &str, int &l, int &t, int &w, int &h, bool top = 0, bool bottom = 0 );
	bool findText ( const QString &str, QRect &r, bool top = 0, bool bottom = 0 );

	// Get the text which is inside the specified rectangle.
	QString getText ( int left, int top, int width, int height );
	QString getText ( const QRect &r );

protected:
	QPainter *m_painter;
	TextPage *m_text;		// text from the current page

private:
	QFont matchFont ( GfxFont *, fp_t m11, fp_t m12, fp_t m21, fp_t m22 );

	void updateLineAttrs ( GfxState *state, GBool updateDash );
	void doFill ( GfxState *state, bool winding );
	void doClip ( GfxState *state, bool winding );
	int convertPath ( GfxState *state, QPointArray &points, QMemArray<int> &lengths );
	int convertSubpath ( GfxState *state, GfxSubpath *subpath, QPointArray &points );
};

#endif
