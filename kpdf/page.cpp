/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt includes
#include <qapplication.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qpainter.h>

// local includes
#include "TextOutputDev.h"
#include "page.h"

class PageOverlay { /*fake temp class*/ };

#include <qimage.h>
KPDFPage::KPDFPage( uint page, float w, float h, int r )
    : m_number( page ), m_width( w ), m_height( h ), m_rotate( r ),
    m_pixmap( 0 ), m_thumbnail( 0 ), m_text( 0 ), m_overlay( 0 )
{
/*    m_thumbnail = new QPixmap( "/a.png", "PNG" );
	QImage im = m_thumbnail->convertToImage();
	im = im.smoothScale(100,100);
	im.setAlphaBuffer( false );
	m_thumbnail->convertFromImage(im);
*/
}

KPDFPage::~KPDFPage()
{
    delete m_pixmap;
    delete m_thumbnail;
    delete m_text;
    delete m_overlay;
}

//BEGIN drawing functions
// drawing functions draw the pixmap directly if it has the right size, or
// else set a scale matrix to the painter and paint a quick 'zoomed' pixmap
//TODO drawing: if !pix but thumb still draw pix scaling thumb and vice-versa.
void KPDFPage::drawPixmap( QPainter * p, const QRect & limits, int width, int height ) const
{
    if ( m_pixmap )
    {
        if ( m_pixmap->width() == width && m_pixmap->height() == height )
            p->drawPixmap( limits.topLeft(), *m_pixmap, limits );
        else
        {
            p->scale( width / (double)m_pixmap->width(), height / (double)m_pixmap->height() );
            p->drawPixmap( 0,0, *m_pixmap, 0,0, m_pixmap->width(), m_pixmap->height() );
            p->setPen( Qt::black );
            p->drawLine( 0, 0, 20, 0 );
            p->drawLine( 20, 0, 0, 20 );
            p->drawLine( 0, 20, 0, 0 );
        }
        // draw selection (FIXME Enrico: move selection stuff inside PAGE!!)
        /*if ( there is something to hilght )
        p->setBrush(Qt::SolidPattern);
        p->setPen(QPen(Qt::black, 1)); // should not be necessary bug a Qt bug makes it necessary
        p->setRasterOp(Qt::NotROP);
        p->drawRect(qRound(m_xMin*m_zoomFactor), qRound(m_yMin*m_zoomFactor), qRound((m_xMax- m_xMin)*m_zoomFactor), qRound((m_yMax- m_yMin)*m_zoomFactor));
        */
    }
    else
        p->fillRect( limits, Qt::white /*FIXME change to the page bg color*/ );
}

void KPDFPage::drawThumbnail( QPainter * p, const QRect & limits, int width, int height ) const // OK
{
    if ( m_thumbnail )
    {
        if ( m_thumbnail->width() == width && m_thumbnail->height() == height )
            p->drawPixmap( limits.topLeft(), *m_thumbnail, limits );
        else
        {
            p->scale( width / (double)m_thumbnail->width(), height / (double)m_thumbnail->height() );
            p->drawPixmap( 0,0, *m_thumbnail, 0,0, m_thumbnail->width(), m_thumbnail->height() );
        }
    }
    else
        p->fillRect( limits, QApplication::palette().active().base() );
}
//END drawing functions

//BEGIN contents set methods
bool KPDFPage::hasPixmap( int width, int height ) const
{
    return m_pixmap ? ( m_pixmap->width() == width && m_pixmap->height() == height ) : false;
}

bool KPDFPage::hasThumbnail( int width, int height ) const
{
    return m_thumbnail ? ( m_thumbnail->width() == width && m_thumbnail->height() == height ) : false;
}

bool KPDFPage::hasLink( int mouseX, int mouseY ) const
{
    //TODO this.
    //Sample implementation using a small rect as 'active' link zone
    return QRect( 50,50, 100,100 ).contains( mouseX, mouseY );
}

void KPDFPage::setPixmap( const QImage & image )
{
    delete m_pixmap;
    m_pixmap = new QPixmap( image );
}

/*
void KPDFPage::setPixmapOverlaySelection( const QRect & normalizedRect );
{   //TODO this
}
void KPDFPage::setPixmapOverlayNotations( ..DOMdescription.. )
{   //TODO this
}
*/

void KPDFPage::setThumbnail( const QImage & image )
{
    delete m_thumbnail;
    m_thumbnail = new QPixmap( image );
}

/*
void KPDFPage::setTextPage( TextOutputDev * textPage )
{
    delete m_text;
    m_text = 0;
    if ( m_text )
        m_text = textPage;
}

void KPDFPage::setLinks( ..SomeStruct.. )
{
}
*/
//END contents set methods

//BEGIN [FIND]
/*bool KPDFPage::hasText( QString & text )
{   //TODO this
    return text.isNull();
}

const QRect & KPDFPage::textPosition()
{   //TODO this
    return QRect();
}*/
//END [FIND]

