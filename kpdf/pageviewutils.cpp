/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qbitmap.h>
#include <qpainter.h>
#include <qimage.h>
#include <qtimer.h>
#include <kimageeffect.h>
#include <kiconloader.h>

// local includes
#include "pageviewutils.h"


PageViewMessage::PageViewMessage( QWidget * parent )
    : QWidget( parent, "pageViewMessage" ), m_timer( 0 )
{
    setFocusPolicy( NoFocus );
    setBackgroundMode( NoBackground );
    move( 10, 10 );
    resize( 0, 0 );
    hide();
}

void PageViewMessage::display( const QString & message, Icon icon, int durationMs )
// give to Caesar what Caesar owns: code taken from Amarok's osd.h/.cpp
{
    // determine text rectangle
    QRect textRect = fontMetrics().boundingRect( message );
    textRect.moveBy( -textRect.left(), -textRect.top() );
    textRect.addCoords( 0, 0, 2, 2 );
    int width = textRect.width(),
        height = textRect.height(),
        textXOffset = 0,
        shadowOffset = message.isRightToLeft() ? -1 : 1;

    // load icon (if set) and update geometry
    QPixmap symbol;
    if ( icon != None )
    {
        switch ( icon )
        {
            case Error:
                symbol = SmallIcon( "messagebox_critical" );
                break;
            case Warning:
                symbol = SmallIcon( "messagebox_warning" );
                break;
            default:
                symbol = SmallIcon( "messagebox_info" );
                break;
        }
        textXOffset = 2 + symbol.width();
        width += textXOffset;
        height = QMAX( height, symbol.height() );
    }
    QRect geometry( 0, 0, width + 10, height + 8 );

    // resize pixmap, mask and widget
    static QBitmap mask;
    mask.resize( geometry.size() );
    m_pixmap.resize( geometry.size() );
    resize( geometry.size() );

    // create and set transparency mask
    QPainter maskPainter( &mask);
    mask.fill( Qt::black );
    maskPainter.setBrush( Qt::white );
    maskPainter.drawRoundRect( geometry, 1600 / geometry.width(), 1600 / geometry.height() );
    setMask( mask );

    // draw background
    QPainter bufferPainter( &m_pixmap );
    bufferPainter.setPen( Qt::black );
    bufferPainter.setBrush( backgroundColor() );
    bufferPainter.drawRoundRect( geometry, 1600 / geometry.width(), 1600 / geometry.height() );

    // draw icon if present
    if ( !symbol.isNull() )
        bufferPainter.drawPixmap( 5, 4, symbol, 0, 0, symbol.width(), symbol.height() );

    // draw shadow and text
    int yText = ( geometry.height() - textRect.height() ) / 2;
    bufferPainter.setPen( backgroundColor().dark( 115 ) );
    bufferPainter.drawText( 5 + textXOffset + shadowOffset, yText + 1,
                            textRect.width(), textRect.height(), AlignCenter, message );
    bufferPainter.setPen( foregroundColor() );
    bufferPainter.drawText( 5 + textXOffset, yText,
                            textRect.width(), textRect.height(), AlignCenter, message );

    // show widget and schedule a repaint
    show();
    update();

    // close the message window after given mS
    if ( durationMs > 0 )
    {
        if ( !m_timer )
        {
            m_timer = new QTimer( this );
            connect( m_timer, SIGNAL( timeout() ), SLOT( hide() ) );
        }
        m_timer->start( durationMs, true );
    }
}

void PageViewMessage::paintEvent( QPaintEvent * e )
{
    QPainter p( this );
    p.drawPixmap( e->rect().topLeft(), m_pixmap, e->rect() );
}

void PageViewMessage::mousePressEvent( QMouseEvent * /*e*/ )
{
    if ( m_timer )
        m_timer->stop();
    hide();
}



// window placed in overlay when selecting a window to zoom into
PageViewOverlay::PageViewOverlay( QWidget * parent, OverlayOperation /*op*/ )
    : QWidget( parent, "overlayWindow", WNoAutoErase | WPaintUnclipped )
{
    // grab underlying contents
    QPoint topLeft = mapToGlobal( QPoint( 0, 0 ) );
    m_backPixmap = QPixmap::grabWindow( qt_xrootwin(), topLeft.x(), topLeft.y(), parent->width(), parent->height() );

    // resize window
    setBackgroundMode( Qt::NoBackground );
    resize( parent->width(), parent->height() );
    show();
}

void PageViewOverlay::setBeginCorner( int x, int y )
{
    m_startX = x;
    m_startY = y;
}

void PageViewOverlay::setEndCorner( int x, int y )
{
    // set previous area to be redrawn
    if ( !m_currentRect.isNull() )
        update(m_currentRect);

    // update current area and schedule for redrawing
    m_currentRect.setRect( m_startX, m_startY, x - m_startX, y - m_startY );
    update(m_currentRect = m_currentRect.normalize());
}

const QRect & PageViewOverlay::selectedRect()
{
    return m_currentRect;
}

void PageViewOverlay::paintEvent( QPaintEvent * )
{
    QColor blendColor = palette().active().highlight();
    QPainter p( this );

    // draw uncovered background (subtracting current from old rect)
    QMemArray<QRect> transparentRects = QRegion( m_oldRect ).subtract( m_currentRect ).rects();
    for ( uint i = 0; i < transparentRects.count(); i++ )
    {
        QRect r = transparentRects[i];
        p.drawPixmap( r.topLeft(), m_backPixmap, r );
    }

    // draw opaque rects (subtracting old from current rect)
    m_oldRect.addCoords( 1, 1, -1, -1 );
    QMemArray<QRect> opaqueRects = QRegion( m_currentRect ).subtract( m_oldRect ).rects();
    for ( uint i = 0; i < opaqueRects.count(); i++ )
    {
        QRect r = opaqueRects[i];
        // skip rectangles covered by the border from painting
        if ( r.width() <= 1 || r.height() <= 1 )
            continue;
        QPixmap blendedPixmap( r.width(), r.height() );
        copyBlt( &blendedPixmap, 0,0, &m_backPixmap, r.left(),r.top(), r.width(),r.height() );
        QImage blendedImage = blendedPixmap.convertToImage();
        KImageEffect::blend( blendColor.dark(140), blendedImage, 0.2 );
        p.drawPixmap( r.left(),r.top(), blendedImage, 0,0,r.width(),r.height() );
    }

    // draw border
    if ( m_currentRect.width() > 20 && m_currentRect.height() > 20 )
        p.setPen( blendColor );
    else
        p.setPen( Qt::red );
    p.drawRect( m_currentRect );
    m_oldRect = m_currentRect;
}

