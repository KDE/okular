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
#include <qimage.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qpainter.h>
#include <qmap.h>

// local includes
#include "TextOutputDev.h"
#include "page.h"

// TODO add painting effects (plus selection rectangle)
// TODO think about moving rendering ...

KPDFPage::KPDFPage( int page, float w, float h, int r )
    : m_number( page ), m_rotation( r ), m_width( w ), m_height( h ), m_text( 0 )
{
}

KPDFPage::~KPDFPage()
{
    QMap<int,QPixmap *>::iterator it = m_pixmaps.begin(), end = m_pixmaps.end();
    for ( ; it != end; ++it )
        delete *it;
    delete m_text;
}


void KPDFPage::drawPixmap( int id, QPainter * p, const QRect & limits, int width, int height ) const
{
    QPixmap * pixmap = 0;

    // if a pixmap is present for given id, use it
    if ( m_pixmaps.contains( id ) )
        pixmap = m_pixmaps[ id ];

    // else find the closest match using pixmaps of other IDs (great optim!)
    else if ( !m_pixmaps.isEmpty() )
    {
        int minDistance = -1;
        QMap<int,QPixmap *>::const_iterator it = m_pixmaps.begin(), end = m_pixmaps.end();
        for ( ; it != end; ++it )
        {
            int pixWidth = (*it)->width(),
                distance = pixWidth > width ? pixWidth - width : width - pixWidth;
            if ( minDistance == -1 || distance < minDistance )
            {
                pixmap = *it;
                minDistance = distance;
            }
        }
    }

    // if we found a pixmap draw it
    if ( pixmap )
    {
        // fast blit the pixmap if it has the right size..
        if ( pixmap->width() == width && pixmap->height() == height )
            p->drawPixmap( limits.topLeft(), *pixmap, limits );
        // ..else set a scale matrix to the painter and paint a quick 'zoomed' pixmap
        else
        {
            p->save();
            p->scale( width / (double)pixmap->width(), height / (double)pixmap->height() );
            p->drawPixmap( 0,0, *pixmap, 0,0, pixmap->width(), pixmap->height() );
            p->restore();
            // draw a red cross (to hilight that the pixmap has not the right size)
            p->setPen( Qt::red );
            p->drawLine( 0, 0, width, height );
            p->drawLine( 0, height, width, 0 );
        }
        // draw selection (FIXME Enrico: move selection stuff inside PAGE!!)
        /*if ( there is something to hilght )
            p->setBrush(Qt::SolidPattern);
            p->setPen(QPen(Qt::black, 1)); // should not be necessary bug a Qt bug makes it necessary
            p->setRasterOp(Qt::NotROP);
            p->drawRect(qRound(m_xMin*m_zoomFactor), qRound(m_yMin*m_zoomFactor), qRound((m_xMax- m_xMin)*m_zoomFactor), qRound((m_yMax- m_yMin)*m_zoomFactor));
        */
    }
    // else draw a blank area
    else
        p->fillRect( limits, Qt::white /*FIXME change to the page bg color*/ );
}


bool KPDFPage::hasPixmap( int id, int width, int height ) const
{
    if ( !m_pixmaps.contains( id ) )
        return false;
    QPixmap * p = m_pixmaps[ id ];
    return p ? ( p->width() == width && p->height() == height ) : false;
}

bool KPDFPage::hasLink( int mouseX, int mouseY ) const
{
    //TODO this.
    //Sample implementation using a small rect as 'active' link zone
    return QRect( 20,20, 100,50 ).contains( mouseX, mouseY );
}

void KPDFPage::setPixmap( int id, QPixmap * pixmap )
{
    if ( !m_pixmaps.contains( id ) )
        delete m_pixmaps[id];
    m_pixmaps[id] = pixmap;
}

/*
void KPDFPage::setPixmapOverlaySelection( const QRect & normalizedRect );
{   //TODO this
}
void KPDFPage::setPixmapOverlayNotations( ..DOMdescription.. )
{   //TODO this
}
*/

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

/*bool KPDFPage::hasText( QString & text )
{   //TODO this
    return text.isNull();
// FIXME MOVED from the QOutputDev. Find over a textpage.
//bool find(Unicode *s, int len, GBool startAtTop, GBool stopAtBottom, GBool startAtLast, GBool stopAtLast, double *xMin, double *yMin, double *xMax, double *yMax)
//{return m_text -> findText(s, len, startAtTop, stopAtBottom, startAtLast, stopAtLast, xMin, yMin, xMax, yMax);}
}

const QRect & KPDFPage::textPosition()
{   //TODO this
    return QRect();
}*/
