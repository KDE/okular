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

#include "TextOutputDev.h"

#include "QOutputDev.h"

//------------------------------------------------------------------------
// QOutputDev
//------------------------------------------------------------------------

QOutputDev::QOutputDev(SplashColor paperColor)
	: SplashOutputDev(splashModeRGB8, false, paperColor)
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
}

void QOutputDev::updateFont(GfxState *state)
{
	SplashOutputDev::updateFont(state);
	m_text->updateFont(state);
}

bool QOutputDev::find(Unicode *s, int len, GBool startAtTop, GBool stopAtBottom, GBool startAtLast, GBool stopAtLast, double *xMin, double *yMin, double *xMax, double *yMax)
{
	return m_text -> findText(s, len, startAtTop, stopAtBottom, startAtLast, stopAtLast, xMin, yMin, xMax, yMax);
}
