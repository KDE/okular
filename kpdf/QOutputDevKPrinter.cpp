/***************************************************************************
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Christophe Devriese                             *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "SplashBitmap.h"
#include "SplashTypes.h"
 
#include "QOutputDevKPrinter.h"

#include <kprinter.h>
#include <qpainter.h>
#include <qimage.h>

QOutputDevKPrinter::QOutputDevKPrinter(QPainter& painter, SplashColor paperColor, KPrinter& printer )
  : SplashOutputDev(splashModeRGB8, false, paperColor), m_printer( printer ), m_painter( painter )
{
}

QOutputDevKPrinter::~QOutputDevKPrinter()
{
}

void QOutputDevKPrinter::startPage(int page, GfxState *state)
{
	// TODO: page size ?
	SplashOutputDev::startPage( page, state);
}

void QOutputDevKPrinter::endPage()
{
	SplashColorPtr dataPtr;
	int bh, bw;
	
	SplashOutputDev::endPage();
	bh = getBitmap()->getHeight();
	bw = getBitmap()->getWidth();
	dataPtr = getBitmap()->getDataPtr();
	m_painter.drawPixmap(0, 0, QImage((uchar*)dataPtr.rgb8, bw, bh, 32, 0, 0, QImage::IgnoreEndian));
}
