/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

 // qt includes
#include <qpixmap.h>
#include <qstring.h>
#include <qpainter.h>

// local includes
#include "TextOutputDev.h"
#include "page.h"


KPDFPage::KPDFPage( uint i, float w, float h )
    : m_number( i ), m_width( w ), m_height( h ), m_zoom( 1 ),
    m_pixmap( 0 ), m_thumbnail( 0 ), m_text( 0 ), m_overlay( 0 )
{
    printf( "hello %d ", i );
}

KPDFPage::~KPDFPage()
{
    printf( "bye[%d] ", m_number );
    delete m_pixmap;
    delete m_thumbnail;
    delete m_text;
    delete m_overlay;
}

/** DRAWING **/
void KPDFPage::drawPixmap( QPainter * p, const QRect & limits ) const  // MUTEXED
{
    //threadLock.lock();
    
    if ( m_pixmap )
        p->drawPixmap( limits.topLeft(), *m_pixmap, limits );
    else
        p->fillRect( limits, Qt::blue );
    
    //threadLock.unlock();
}

void KPDFPage::drawThumbnail( QPainter * p, const QRect & limits ) const  // MUTEXED
{
    //threadLock.lock();

    if ( m_thumbnail )
        p->drawPixmap( limits.topLeft(), *m_thumbnail, limits );
    else
        p->fillRect( limits, Qt::red );

    //threadLock.unlock();
}


/** FIND **/
bool KPDFPage::hasText( QString & text )
{
    //FIXME
    return text.isNull();
}
/*
const QRect & KPDFPage::textPosition()
{
    //FIXME
    return QRect();
}
*/


/** SLOTS **/
void KPDFPage::slotSetZoom( float /*scale*/ )
{
    
}

void KPDFPage::slotSetContents( QPixmap * pix )   // MUTEXED
{
    if ( !pix )
        return;
        
    threadLock.lock();
    
    delete m_pixmap;
    m_pixmap = new QPixmap( pix->width() + 6, pix->height() + 6 );
    bitBlt( m_pixmap, 1,1, pix, 0,0, pix->width(),pix->height() );
    QPainter paint( m_pixmap );
    paint.drawRect( 0,0, pix->width()+1, pix->height()+1 );
    paint.end();
    //update page size (in pixels)
    m_size = m_pixmap->size();
    
    threadLock.unlock();
}

void KPDFPage::slotSetThumbnail( QPixmap * thumb )  // MUTEXED
{
    if ( !thumb )
        return;
        
    threadLock.lock();

    delete m_thumbnail;
    m_thumbnail = new QPixmap( *thumb );

    threadLock.unlock();
}

void KPDFPage::slotSetOverlay()   // MUTEXED
{
    threadLock.lock();
    //TODO this
    threadLock.unlock();
}

#include "page.moc"
