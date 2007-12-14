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
#include <kapplication.h>
#include <kimageeffect.h>
#include <kiconloader.h>

// local includes
#include "pageviewutils.h"
#include "core/page.h"
#include "conf/settings.h"

PageViewMessage::PageViewMessage( QWidget * parent )
    : QWidget( parent, "pageViewMessage" ), m_timer( 0 )
{
    setFocusPolicy( NoFocus );
    setBackgroundMode( NoBackground );
    setPaletteBackgroundColor(kapp->palette().color(QPalette::Active, QColorGroup::Background));
    // if the layout is LtR, we can safely place it in the right position
    if ( !QApplication::reverseLayout() )
        move( 10, 10 );
    resize( 0, 0 );
    hide();
}

void PageViewMessage::display( const QString & message, Icon icon, int durationMs )
// give to Caesar what Caesar owns: code taken from Amarok's osd.h/.cpp
// "redde (reddite, pl.) cesari quae sunt cesaris", just btw. ;)
{
    if ( !KpdfSettings::showOSD() )
    {
        hide();
        return;
    }

    // determine text rectangle
    QRect textRect = fontMetrics().boundingRect( message );
    textRect.moveBy( -textRect.left(), -textRect.top() );
    textRect.addCoords( 0, 0, 2, 2 );
    int width = textRect.width(),
        height = textRect.height(),
        textXOffset = 0,
        iconXOffset = 0,
        shadowOffset = 1;

    // load icon (if set) and update geometry
    QPixmap symbol;
    if ( icon != None )
    {
        switch ( icon )
        {
            case Find:
                symbol = SmallIcon( "viewmag" );
                break;
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
        if ( QApplication::reverseLayout() )
        {
            iconXOffset = 2 + textRect.width();
        }
        else
        {
            textXOffset = 2 + symbol.width();
        }
        width += 2 + symbol.width();
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
    bufferPainter.setBrush( paletteBackgroundColor() );
    bufferPainter.drawRoundRect( geometry, 1600 / geometry.width(), 1600 / geometry.height() );

    // draw icon if present
    if ( !symbol.isNull() )
        bufferPainter.drawPixmap( 5 + iconXOffset, 4, symbol, 0, 0, symbol.width(), symbol.height() );

    // draw shadow and text
    int yText = geometry.height() - height / 2;
    bufferPainter.setPen( paletteBackgroundColor().dark( 115 ) );
    bufferPainter.drawText( 5 + textXOffset + shadowOffset, yText + 1, message );
    bufferPainter.setPen( foregroundColor() );
    bufferPainter.drawText( 5 + textXOffset, yText, message );

    // if the layout is RtL, we can move it to the right place only after we
    // know how much size it will take
    if ( QApplication::reverseLayout() )
        move( parentWidget()->width() - geometry.width() - 10, 10 );

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
    } else if ( m_timer )
        m_timer->stop();
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



PageViewItem::PageViewItem( const KPDFPage * page )
    : m_page( page ), m_zoomFactor( 1.0 )
{
}

const KPDFPage * PageViewItem::page() const
{
    return m_page;
}

int PageViewItem::pageNumber() const
{
    return m_page->number();
}

const QRect& PageViewItem::geometry() const
{
    return m_geometry;
}

int PageViewItem::width() const
{
    return m_geometry.width();
}

int PageViewItem::height() const
{
    return m_geometry.height();
}

double PageViewItem::zoomFactor() const
{
    return m_zoomFactor;
}

void PageViewItem::setGeometry( int x, int y, int width, int height )
{
    m_geometry.setRect( x, y, width, height );
}

void PageViewItem::setWHZ( int w, int h, double z )
{
    m_geometry.setWidth( w );
    m_geometry.setHeight( h );
    m_zoomFactor = z;
}

void PageViewItem::moveTo( int x, int y )
{
    m_geometry.moveLeft( x );
    m_geometry.moveTop( y );
}
