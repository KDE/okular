/* This file is part of the KDE libraries
   Copyright (C) 2004 Albert Astals Cid <tsdgeos@terra.es>
   Copyright (C) 2001, 2003 Christophe Devriese <oelewapperke@kde.org>
   Copyright 1996 Derek B. Noonburg
   CopyRight 2002 Robert Griebl

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifdef __GNUC__
#pragma implementation
#endif

#include <qimage.h>
#include <qpainter.h>

#include "SplashBitmap.h"
#include "SplashTypes.h"
#include "TextOutputDev.h"

#include "QOutputDev.h"

//------------------------------------------------------------------------
// QOutputDev
//------------------------------------------------------------------------

QOutputDev::QOutputDev(QPainter *p, SplashColor paperColor)
	: SplashOutputDev(splashModeRGB8, false, paperColor), m_painter(p)
{
	// create text object
	m_text = new TextPage ( gFalse );
}

QOutputDev::~QOutputDev ( )
{
	delete m_text;
}

void QOutputDev::drawChar(GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, Unicode *u, int uLen)
{
	m_text->addChar(state, x, y, dx, dy, code, u, uLen);
	SplashOutputDev::drawChar(state, x, y, dx, dy, originX, originY, code, u, uLen);
}

GBool QOutputDev::beginType3Char(GfxState *state, double x, double y, double dx, double dy, CharCode code, Unicode *u, int uLen)
{
	m_text->addChar(state, x, y, dx, dy, code, u, uLen);
	return SplashOutputDev::beginType3Char(state, x, y, dx, dy, code, u, uLen);
}

void QOutputDev::clear()
{
	startDoc(NULL);
	startPage(0, NULL);
}

void QOutputDev::startPage(int pageNum, GfxState *state)
{
	SplashOutputDev::startPage(pageNum, state);
	m_text->startPage(state);
}

void QOutputDev::endPage()
{
	SplashOutputDev::endPage();
	m_text->coalesce(gTrue);
	draw();
}

void QOutputDev::updateFont(GfxState *state)
{
	SplashOutputDev::updateFont(state);
	m_text->updateFont(state);
}

void QOutputDev::draw()
{
	SplashColorPtr dataPtr;
	int bh, bw;
	
	if (!m_painter) return;
	
	bh = getBitmap()->getHeight();
	bw = getBitmap()->getWidth();
	dataPtr = getBitmap()->getDataPtr();
	
	m_painter->drawImage(0, 0, QImage((uchar*)dataPtr.rgb8, bw, bh, 32, 0, 0, QImage::IgnoreEndian));
}
