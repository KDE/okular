///========================================================================
//
// QOutputDevPixmap.cc
//
// Copyright 1996 Derek B. Noonburg
// CopyRight 2002 Robert Griebl
//
//========================================================================

#ifdef __GNUC__
#pragma implementation
#endif

#include <aconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <iostream>

#include <GString.h>
#include <Object.h>
#include <Stream.h>
#include <Link.h>
#include <GfxState.h>
#include <GfxFont.h>
#include <UnicodeMap.h>
#include <CharCodeToUnicode.h>
#include <FontFile.h>
#include <Error.h>
#include <TextOutputDev.h>
#include <Catalog.h>


#include <qpixmap.h>
#include <qcolor.h>
#include <qimage.h>
#include <qpainter.h>
#include <qdict.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qclipboard.h>

#include "QOutputDevPixmap.h"

//#define QPDFDBG(x) x		// special debug mode
#define QPDFDBG(x)   		// normal compilation

static inline long int round( float x ) { static_cast<long int>( x + x > 0 ? 0.5 : -0.5); }

//------------------------------------------------------------------------
// Constants and macros
//------------------------------------------------------------------------

#ifndef KDE_USE_FINAL
static inline QColor q_col ( const GfxRGB &rgb )
{
	return QColor ( round ( rgb. r * 255 ), round ( rgb. g * 255 ), round ( rgb. b * 255 ));
convertSubpath ( state, path-> getSubpath ( i ), points );
	}

	return n;
}

//
// Transform points in a single subpath and convert curves to line
// segments.
//
int QOutputDevPixmap::convertSubpath ( GfxState *state, GfxSubpath *subpath, QPointArray &points )
{
	int oldcnt = points. count ( );

	fp_t x0, y0, x1, y1, x2, y2, x3, y3;

	int m = subpath-> getNumPoints ( );
	int i = 0;

	while ( i < m ) {
		if ( i >= 1 && subpath->getCurve ( i )) {
			state->transform( subpath->getX( i - 1 ), subpath->getY( i - 1 ), &x0, &y0 );
			state->transform( subpath->getX( i ),     subpath->getY( i ),     &x1, &y1 );
			state->transform( subpath->getX( i + 1 ), subpath->getY( i + 1 ), &x2, &y2 );
			state->transform( subpath->getX( i + 2 ), subpath->getY( i + 2 ), &x3, &y3 );

			QPointArray tmp;
			tmp.setPoints ( 4, round ( x0 ), round ( y0 ), round ( x1 ), round ( y1 ),
			                    round ( x2 ), round ( y2 ), round ( x3 ), round ( y3 ));

			tmp = tmp.cubicBezier();
			points.putPoints ( points.count(), tmp. count(), tmp );

			i += 3;
		}
		else {
			state-> transform ( subpath-> getX ( i ), subpath-> getY ( i ), &x1, &y1 );

			points. putPoints ( points. count ( ), 1, round ( x1 ), round ( y1 ));
			++i;
		}
	}
	return points. count ( ) - oldcnt;
}


void QOutputDevPixmap::beginString ( GfxState *state, GString * /*s*/ )
{
	m_text-> beginWord ( state, state->getCurX(), state->getCurY() );
}

void QOutputDevPixmap::endString ( GfxState */*state*/ )
{
	m_text-> endWord ( );
}

void QOutputDevPixmap::drawChar ( GfxState *state, fp_t x, fp_t y,
                           fp_t dx, fp_t dy, fp_t originX, fp_t originY,
                           CharCode code, Unicode *u, int uLen )
{
	fp_t x1, y1, dx1, dy1;

	if ( uLen > 0 )
		m_text-> addChar ( state, x, y, dx, dy, code, u, uLen );

	// check for invisible text -- this is used by Acrobat Capture
	if (( state-> getRender ( ) & 3 ) == 3 ) {
		return;
	}

	x -= originX;
	y -= originY;
	state-> transform      ( x,  y,  &x1,  &y1 );
	state-> transformDelta ( dx, dy, &dx1, &dy1 );


	if ( uLen > 0 ) {
		QString str;
		QFontMetrics fm = m_painter-> fontMetrics ( );

		for ( int i = 0; i < uLen; i++ ) {
			QChar c = QChar ( u [i] );

			if ( fm. inFont ( c )) {
				str [i] = QChar ( u [i] );
			}
			else {
				str [i] = ' ';
				QPDFDBG( printf ( "CHARACTER NOT IN FONT: %hx\n", c. unicode ( )));
			}
		}

		if (( uLen == 1 ) && ( str [0] == ' ' ))
			return;


		fp_t m11, m12, m21, m22;

		state-> getFontTransMat ( &m11, &m12, &m21, &m22 );
		m11 *= state-> getHorizScaling ( );
		m12 *= state-> getHorizScaling ( );

		fp_t fsize = m_painter-> font ( ). pixelSize ( );

#ifndef QT_NO_TRANSFORMATIONS
		QWMatrix oldmat;

		bool dorot = (( m12 < -0.1 ) || ( m12 > 0.1 )) && (( m21 < -0.1 ) || ( m21 > 0.1 ));

		if ( dorot ) {
			oldmat = m_painter-> worldMatrix ( );

			// std::cerr << std::endl << "ROTATED: " << m11 << ", " << m12 << ", " << m21 << ", " << m22 << " / SIZE: " << fsize << " / TEXT: " << str. local8Bit ( ) << endl << endl;

			QWMatrix mat ( round ( m11 / fsize ), round ( m12 / fsize ), -round ( m21 / fsize ), -round ( m22 / fsize ), round ( x1 ), round ( y1 ));

			m_painter-> setWorldMatrix ( mat );

			x1 = 0;
			y1 = 0;
		}
#endif

		QPen oldpen = m_painter-> pen ( );

		if (!( state-> getRender ( ) & 1 )) {
			QPen fillpen = oldpen;

			fillpen. setColor ( m_painter-> brush ( ). color ( ));
			m_painter-> setPen ( fillpen );
		}

		if ( fsize > 5 )
			m_painter-> drawText ( round ( x1 ), round ( y1 ), str );
		else
			m_painter-> fillRect ( round ( x1 ), round ( y1 ), round ( QMAX( fp_t(1), dx1 )), round ( QMAX( fsize, dy1 )), m_painter-> pen ( ). color ( ));

		m_painter-> setPen ( oldpen );

#ifndef QT_NO_TRANSFORMATIONS
		if ( dorot )
			m_painter-> setWorldMatrix ( oldmat );
#endif

		QPDFDBG( printf ( "DRAW TEXT: \"%s\" at (%ld/%ld)\n", str. local8Bit ( ). data ( ), round ( x1 ), round ( y1 )));
	}
	else if ( code != 0 ) {
		// some PDF files use CID 0, which is .notdef, so just ignore it
		qWarning ( "Unknown character (CID=%d Unicode=%hx)\n", code, (unsigned short) ( uLen > 0 ? u [0] : (Unicode) 0 ));
	}
}



void QOutputDevPixmap::drawImageMask ( GfxState *state, Object */*ref*/, Stream *str, int width, int height, GBool invert, GBool inlineImg )
{
	// get CTM, check for singular matrix
	fp_t *ctm = state-> getCTM ( );

	if ( fabs ( ctm [0] * ctm [3] - ctm [1] * ctm [2] ) < 0.000001 ) {
		qWarning ( "Singular CTM in drawImage\n" );

		if ( inlineImg ) {
			str-> reset ( );
			int j = height * (( width + 7 ) / 8 );
			for ( int i = 0; i < j; i++ )
				str->getChar();

			str->close();
		}
		return;
	}

	GfxRGB rgb;
	state-> getFillRGB ( &rgb );
	uint val = ( int( round ( rgb. r * 255 ) ) & 0xff ) << 16 | ( int( round ( rgb. g * 255 ) ) & 0xff ) << 8 | ( int( round ( rgb. b * 255 ) ) & 0xff );


	QImage img ( width, height, 32 );
	img. setAlphaBuffer ( true );

	QPDFDBG( printf ( "IMAGE MASK (%dx%d)\n", width, height ));

	// initialize the image stream
	ImageStream *imgStr = new ImageStream ( str, width, 1, 1 );
	imgStr-> reset ( );

	uchar **scanlines = img. jumpTable ( );

	if ( ctm [3] > 0 )
		scanlines += ( height - 1 );

	for ( int y = 0; y < height; y++ ) {
		QRgb *scanline = (QRgb *) *scanlines;

		if ( ctm [0] < 0 )
			scanline += ( width - 1 );

		for ( int x = 0; x < width; x++ ) {
			Guchar alpha;

			imgStr-> getPixel ( &alpha );

			if ( invert )
				alpha ^= 1;

			*scanline = ( alpha == 0 ) ? 0xff000000 | val : val;

			ctm [0] < 0 ? scanline-- : scanline++;
		}
		ctm [3] > 0 ? scanlines-- : scanlines++;
	}

#ifndef QT_NO_TRANSFORMATIONS
	QWMatrix mat ( ctm [0] / width, ctm [1], ctm [2], ctm [3] / height, ctm [4], ctm [5] );

	//std::cerr << "MATRIX T=" << mat. dx ( ) << "/" << mat. dy ( ) << std::endl
	//         << " - M=" << mat. m11 ( ) << "/" << mat. m12 ( ) << "/" << mat. m21 ( ) << "/" << mat. m22 ( ) << std::endl;

	QWMatrix oldmat = m_painter-> worldMatrix ( );
	m_painter-> setWorldMatrix ( mat, true );

#ifdef QWS
	QPixmap pm;
	pm. convertFromImage ( img );
	m_painter-> drawPixmap ( 0, 0, pm );
#else
	m_painter-> drawImage ( QPoint ( 0, 0 ), img );
#endif

	m_painter-> setWorldMatrix ( oldmat );

#else
	if (( ctm [1] < -0.1 ) || ( ctm [1] > 0.1 ) || ( ctm [2] < -0.1 ) || ( ctm [2] > 0.1 )) {
		QPDFDBG( printf (  "### ROTATED / SHEARED / ETC -- CANNOT DISPLAY THIS IMAGE\n" ));
	}
	else {
		int x = round ( ctm [4] );
		int y = round ( ctm [5] );

		int w = round ( ctm [0] );
		int h = round ( ctm [3] );

		if ( w < 0 ) {
			x += w;
			w = -w;
		}
		if ( h < 0 ) {
			y += h;
			h = -h;
		}

		QPDFDBG( printf ( "DRAWING IMAGE MASKED: %d/%d - %dx%d\n", x, y, w, h ));

		img = img. smoothScale ( w, h );
		m_painter-> drawImage ( x, y, img );
	}

#endif

	delete imgStr;
}


void QOutputDevPixmap::drawImage(GfxState *state, Object */*ref*/, Stream *str, int width, int height, GfxImageColorMap *colorMap, int *maskColors, GBool inlineImg )
{
	int nComps, nVals, nBits;

	// image parameters
	nComps = colorMap->getNumPixelComps ( );
	nVals = width * nComps;
	nBits = colorMap-> getBits ( );

	// get CTM, check for singular matrix
	fp_t *ctm = state-> getCTM ( );

	if ( fabs ( ctm [0] * ctm [3] - ctm [1] * ctm [2] ) < 0.000001 ) {
		qWarning ( "Singular CTM in drawImage\n" );

		if ( inlineImg ) {
			str-> reset ( );
			int j = height * (( nVals * nBits + 7 ) / 8 );
			for ( int i = 0; i < j; i++ )
				str->getChar();

			str->close();
		}
		return;
	}

	QImage img ( width, height, 32 );

	if ( maskColors )
		img. setAlphaBuffer ( true );

	QPDFDBG( printf ( "IMAGE (%dx%d)\n", width, height ));

	// initialize the image stream
	ImageStream *imgStr = new ImageStream ( str, width, nComps, nBits );
	imgStr-> reset ( );

	Guchar pixBuf [gfxColorMaxComps];
	GfxRGB rgb;


	uchar **scanlines = img. jumpTable ( );

 	if ( ctm [3] < 0 )
 		scanlines += ( height - 1 );

	for ( int y = 0; y < height; y++ ) {
		QRgb *scanline = (QRgb *) *scanlines;

		if ( ctm [0] < 0 )
			scanline += ( width - 1 );

		for ( int x = 0; x < width; x++ ) {
			imgStr-> getPixel ( pixBuf );
			colorMap-> getRGB ( pixBuf, &rgb );

			uint val = ( int( round ( rgb. r * 255 ) ) & 0xff ) << 16 | ( int( round ( rgb. g * 255 ) ) & 0xff ) << 8 | ( int( round ( rgb. b * 255 ) ) & 0xff );

			if ( maskColors ) {
				for ( int k = 0; k < nComps; ++k ) {
					if (( pixBuf [k] < maskColors [2 * k] ) || ( pixBuf [k] > maskColors [2 * k] )) {
						val |= 0xff000000;
						break;
					}
				}
			}
			*scanline = val;

			ctm [0] < 0 ? scanline-- : scanline++;
		}
		ctm [3] > 0 ? scanlines++ : scanlines--;

	}


#ifndef QT_NO_TRANSFORMATIONS
	QWMatrix mat ( ctm [0] / width, ctm [1], ctm [2], ctm [3] / height, ctm [4], ctm [5] );

	// std::cerr << "MATRIX T=" << mat. dx ( ) << "/" << mat. dy ( ) << std::endl
	//          << " - M=" << mat. m11 ( ) << "/" << mat. m12 ( ) << "/" << mat. m21 ( ) << "/" << mat. m22 ( ) << std::endl;

	QWMatrix oldmat = m_painter-> worldMatrix ( );
	m_painter-> setWorldMatrix ( mat, true );

#ifdef QWS
	QPixmap pm;
	pm. convertFromImage ( img );
	m_painter-> drawPixmap ( 0, 0, pm );
#else
	m_painter-> drawImage ( QPoint ( 0, 0 ), img );
#endif

	m_painter-> setWorldMatrix ( oldmat );

#else // QT_NO_TRANSFORMATIONS

	if (( ctm [1] < -0.1 ) || ( ctm [1] > 0.1 ) || ( ctm [2] < -0.1 ) || ( ctm [2] > 0.1 )) {
		QPDFDBG( printf ( "### ROTATED / SHEARED / ETC -- CANNOT DISPLAY THIS IMAGE\n" ));
	}
	else {
		int x = round ( ctm [4] );
		int y = round ( ctm [5] );

		int w = round ( ctm [0] );
		int h = round ( ctm [3] );

		if ( w < 0 ) {
			x += w;
			w = -w;
		}
		if ( h < 0 ) {
			y += h;
			h = -h;
		}

		QPDFDBG( printf ( "DRAWING IMAGE: %d/%d - %dx%d\n", x, y, w, h ));

		img = img. smoothScale ( w, h );
		m_painter-> drawImage ( x, y, img );
	}

#endif


	delete imgStr;
}



bool QOutputDevPixmap::findText ( const QString &str, QRect &r, bool top, bool bottom )
{
	int l, t, w, h;
	r. rect ( &l, &t, &w, &h );

	bool res = findText ( str, l, t, w, h, top, bottom );

	r. setRect ( l, t, w, h );
	return res;
}

bool QOutputDevPixmap::findText ( const QString &str, int &l, int &t, int &w, int &h, bool top, bool bottom )
{
	bool found = false;
	uint len = str. length ( );
	Unicode *s = new Unicode [len];

	for ( uint i = 0; i < len; i++ )
		s [i] = str [i]. unicode ( );

	fp_t x1 = (fp_t) l;
	fp_t y1 = (fp_t) t;
	fp_t x2 = (fp_t) l + w - 1;
	fp_t y2 = (fp_t) t + h - 1;

	if ( m_text-> findText ( s, len, top, bottom, &x1, &y1, &x2, &y2 )) {
		l = round ( x1 );
		t = round ( y1 );
		w = round ( x2 ) - l + 1;
		h = round ( y2 ) - t + 1;
		found = true;
	}
	delete [] s;

	return found;
}

GBool QOutputDevPixmap::findText ( Unicode *s, int len, GBool top, GBool bottom, int *xMin, int *yMin, int *xMax, int *yMax )
{
	bool found = false;
	fp_t xMin1 = (double) *xMin;
	fp_t yMin1 = (double) *yMin;
	fp_t xMax1 = (double) *xMax;
	fp_t yMax1 = (double) *yMax;

	if ( m_text-> findText ( s, len, top, bottom, &xMin1, &yMin1, &xMax1, &yMax1 )) {
		*xMin = round ( xMin1 );
		*xMax = round ( xMax1 );
		*yMin = round ( yMin1 );
		*yMax = round ( yMax1 );
		found = true;
	}
	return found;
}

QString QOutputDevPixmap::getText ( int l, int t, int w, int h )
{
	GString *gstr = m_text-> getText ( l, t, l + w - 1, t + h - 1 );
	QString str = gstr-> getCString ( );
	delete gstr;
	return str;
}

QString QOutputDevPixmap::getText ( const QRect &r )
{
	return getText ( r. left ( ), r. top ( ), r. width ( ), r. height ( ));
}
