/***************************************************************************
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Andy Goossens <andygoossens@telenet.be>         *
 *   Copyright (C) 2003 by Scott Wheeler <wheeler@kde.org>                 *
 *   Copyright (C) 2003 by Ingo Klöcker <kloecker@kde.org>                 *
 *   Copyright (C) 2003 by Will Andrews <will@csociety.org>                *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Waldo Bastian <bastian@kde.org>                 *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifdef __GNUC__
#pragma implementation
#endif

#include <kdebug.h>
#include <qpixmap.h>
#include <qimage.h>

#include "page.h"
#include "GfxState.h"
#include "SplashBitmap.h"
#include "TextOutputDev.h"
#include "QOutputDev.h"

// NOTE: XPDF/Splash implementation dependant code will be marked with '###'

// BEGIN KPDFOutputDev 
KPDFOutputDev::KPDFOutputDev(SplashColor paperColor)
	: SplashOutputDev(splashModeRGB8, false, paperColor),
	m_pixmapWidth( -1 ), m_pixmapHeight( -1 ), m_pixmap( 0 ), m_text( 0 )
{
}

KPDFOutputDev::~KPDFOutputDev()
{
	QValueList< KPDFLink * >::iterator itL = m_links.begin(), endL = m_links.end();
	for ( ; itL != endL; ++itL )
		delete *itL;
    QValueList< KPDFActiveRect * >::iterator itR = m_rects.begin(), endR = m_rects.end();
    for ( ; itR != endR; ++itR )
        delete *itR;
    delete m_pixmap;
	delete m_text;
}

void KPDFOutputDev::setParams( int width, int height, bool genText, bool /*genLinks*/ )
{
	m_pixmapWidth = width;
	m_pixmapHeight = height;
	if ( m_pixmap )
	{
		delete m_pixmap;
		m_pixmap = 0;
	}

	delete m_text;
	m_text = genText ? new TextPage( gFalse ) : 0;

	QValueList< KPDFLink * >::iterator it = m_links.begin(), end = m_links.end();
	for ( ; it != end; ++it )
		delete *it;
	m_links.clear();
}

QPixmap * KPDFOutputDev::takePixmap()
{
	QPixmap * pix = m_pixmap;
	m_pixmap = 0;
	return pix;
}

TextPage * KPDFOutputDev::takeTextPage()
{
	TextPage * text = m_text;
	m_text = 0;
	return text;
}

QValueList< KPDFLink * > KPDFOutputDev::takeLinks()
{
	QValueList< KPDFLink * > linksCopy( m_links );
	m_links.clear();
	return linksCopy;
}

QValueList< KPDFActiveRect * > KPDFOutputDev::takeActiveRects()
{
    QValueList< KPDFActiveRect * > rectsCopy( m_rects );
    m_rects.clear();
    return rectsCopy;
}

void KPDFOutputDev::startPage(int pageNum, GfxState *state)
{
	m_pageNum = pageNum;
	SplashOutputDev::startPage(pageNum, state);
	if ( m_text )
		m_text->startPage(state);
}

void KPDFOutputDev::endPage()
{
	SplashOutputDev::endPage();
	if ( m_text )
		m_text->coalesce(gTrue);

	// create a QPixmap from page data
	delete m_pixmap;
	int bh = getBitmap()->getHeight(),
	    bw = getBitmap()->getWidth();
	SplashColorPtr dataPtr = getBitmap()->getDataPtr();
	QImage * img = new QImage((uchar*)dataPtr.rgb8, bw, bh, 32, 0, 0, QImage::IgnoreEndian);
	if ( bw != m_pixmapWidth || bh != m_pixmapHeight )
	{
		// it may happen (in fact it doesn't) that we need rescaling
		kdWarning() << "Pixmap at page '" << m_pageNum << "' needed rescale." << endl;
		m_pixmap = new QPixmap( img->smoothScale( m_pixmapWidth, m_pixmapHeight ) );
	}
	else
		m_pixmap = new QPixmap( *img );
	delete img;

	// ### hack: unload memory used by bitmap
	SplashOutputDev::startPage(0, NULL);
}

void KPDFOutputDev::drawLink(Link * link, Catalog * catalog)
{
	if ( !link->isOk() )
		return;

	// create the new KPDFLink ...
	KPDFLink * l = new KPDFLink( link->getAction() );
	double x1, y1, x2, y2;
	link->getRect( &x1, &y1, &x2, &y2 );
	int left, top, right, bottom;
	cvtUserToDev( x1, y1, &left, &top );
	cvtUserToDev( x2, y2, &right, &bottom );
	// ... and assign its coords withing current page geometry
	l->setGeometry( left, top, right, bottom );

	// add the link to the vector container
	m_links.push_back( l );

    // call parent's link handler
    SplashOutputDev::drawLink(link, catalog);
}

void KPDFOutputDev::updateFont(GfxState *state)
{
	SplashOutputDev::updateFont(state);
	if ( m_text )
		m_text->updateFont(state);
}

void KPDFOutputDev::drawChar(GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, Unicode *u, int uLen)
{
	if ( m_text )
		m_text->addChar(state, x, y, dx, dy, code, u, uLen);
	SplashOutputDev::drawChar(state, x, y, dx, dy, originX, originY, code, u, uLen);
}

GBool KPDFOutputDev::beginType3Char(GfxState *state, double x, double y, double dx, double dy, CharCode code, Unicode *u, int uLen)
{
	if ( m_text )
		m_text->addChar(state, x, y, dx, dy, code, u, uLen);
	return SplashOutputDev::beginType3Char(state, x, y, dx, dy, code, u, uLen);
}

void KPDFOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
    int _width, int _height, GfxImageColorMap *colorMap,
    int *maskColors, GBool inlineImg)
{
    // find out image rect from the Coord Transform Matrix
    double * ctm = state->getCTM();
    int left = (int)ctm[4],
        top = (int)ctm[5],
        width = (int)ctm[0],
        height = (int)ctm[3];
    // normalize width
    if ( width < 0 )
    {
        width = -width;
        left -= width;
    }
    // normalize height
    if ( height < 0 )
    {
        height = -height;
        top -= height;
    }
    if ( width > 10 && height > 10 )
    {
        KPDFActiveRect * r = new KPDFActiveRect(left, top, width, height);
        // add the rect to the vector container
        m_rects.push_back( r );
    }

    // call parent's image handler
    SplashOutputDev::drawImage(state, ref, str, _width, _height, colorMap, maskColors, inlineImg);
}
// END KPDFOutputDev 


// BEGIN KPDFTextDev 
KPDFTextDev::KPDFTextDev()
{
	m_text = new TextPage( gFalse );
}

KPDFTextDev::~KPDFTextDev()
{
	delete m_text;
}

TextPage * KPDFTextDev::takeTextPage()
{
	TextPage * t = m_text;
	m_text = 0;
	return t;
}

void KPDFTextDev::startPage(int, GfxState *state)
{
	if ( !m_text )
		m_text = new TextPage( gFalse );
	m_text->startPage(state);
}
	
void KPDFTextDev::endPage()
{
	m_text->endPage();
	m_text->coalesce(gTrue);
}

void KPDFTextDev::updateFont(GfxState *state)
{
	m_text->updateFont(state);
}

void KPDFTextDev::drawChar(GfxState *state, double x, double y, double dx, double dy, double /*originX*/, double /*originY*/, CharCode code, Unicode *u, int uLen)
{
	m_text->addChar(state, x, y, dx, dy, code, u, uLen);
}
// END KPDFTextDev 
