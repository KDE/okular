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
	delete m_pixmap;
	delete m_text;
}

void KPDFOutputDev::setParams( int width, int height, bool generateText )
{
	m_pixmapWidth = width;
	m_pixmapHeight = height;
	if ( m_pixmap )
	{
		delete m_pixmap;
		m_pixmap = 0;
	}

	delete m_text;
	m_text = generateText ? new TextPage( gFalse ) : 0;
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

void KPDFOutputDev::drawLink(Link * /*l*/, Catalog */*catalog*/)
{
/*	double x1,y1, x2,y2;
	l->getRect( &x1,&y1, &x2,&y2 );
	LinkAction * a = l->getAction();
	pri NOWARN ntf("LINK %x ok:%d t:%d rect:[%f,%f,%f,%f] \n", (uint)l, (int)l->isOk(),
		(int)a->getKind(), x1,y2, x2-x1, y2-y1 );
*/}

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

void KPDFTextDev::drawChar(GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, Unicode *u, int uLen)
{
	m_text->addChar(state, x, y, dx, dy, code, u, uLen);
}
// END KPDFTextDev 
