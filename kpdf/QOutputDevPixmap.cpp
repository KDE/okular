/***************************************************************************
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Helio Chissini de Castro                        *
 *                           <helio@conectiva.com.br>                      *
 *   Copyright (C) 2003 by Dirk Mueller <mueller@kde.org>                  *
 *   Copyright (C) 2003 by Scott Wheeler <wheeler@kde.org>                 *
 *   Copyright (C) 2003 by Ingo Klöcker <kloecker@kde.org>                 *
 *   Copyright (C) 2003 by Andy Goossens <andygoossens@telenet.be>         *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
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

#include <kdebug.h>

#include "QOutputDevPixmap.h"

//------------------------------------------------------------------------
// Constants and macros
//------------------------------------------------------------------------

#ifndef KDE_USE_FINAL
static inline QColor q_col ( const GfxRGB &rgb )
{
	return QColor ( qRound ( rgb. r * 255 ), qRound ( rgb. g * 255 ), qRound ( rgb. b * 255 ));
}
#endif


//------------------------------------------------------------------------
// QOutputDevPixmap
//------------------------------------------------------------------------

QOutputDevPixmap::QOutputDevPixmap(SplashColor paperColor) : QOutputDev(0, paperColor), m_pixmap(0)
{
}

QOutputDevPixmap::~QOutputDevPixmap( )
{
        delete m_pixmap;
}

void QOutputDevPixmap::startPage ( int pageNum, GfxState *state )
{
	QOutputDev::startPage( pageNum, state);
	delete m_painter;
	delete m_pixmap;

	m_pixmap = new QPixmap ( qRound ( state->getPageWidth ( )), qRound ( state->getPageHeight ( )));
	m_painter = new QPainter ( m_pixmap );

	//printf ( "NEW PIXMAP (%ld x %ld)\n", qRound ( state-> getPageWidth ( )),  qRound ( state-> getPageHeight ( )));

	m_pixmap-> fill ( Qt::white ); // clear pixmap
}

void QOutputDevPixmap::endPage ( )
{
	QOutputDev::endPage();
	delete m_painter;
	m_painter = 0;
}
