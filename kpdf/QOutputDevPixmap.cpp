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
#include "QOutputDevPixmap.moc"

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

QOutputDevPixmap::QOutputDevPixmap () : QOutputDev(0), m_pixmap(0)
{
}

QOutputDevPixmap::~QOutputDevPixmap ( )
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
