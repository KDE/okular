/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qbitmap.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <kiconloader.h>
#include <kimageeffect.h>
#include <klocale.h>
#include <kdeversion.h>
#include <kaccelmanager.h>
#include <kdebug.h>

// system includes
#include <math.h>

// local includes
#include "pageviewtoolbox.h"
//#include "conf/settings.h"

/** ToolboxButton **/

static const int ToolboxButtonIcon = 32;
static const int ToolboxButtonSize = 40;

// a flat button that enlights on hover and has an ID
class ToolboxButton : public QPushButton
{
    public:
        ToolboxButton( QWidget * parent, const ToolboxItem & item, const QPixmap & backPixmap );
        int toolboxID() const;

    protected:
        void mouseMoveEvent( QMouseEvent * e );
        void mouseReleaseEvent( QMouseEvent * e );
        void paintEvent( QPaintEvent * e );

    private:
        int m_id;
        bool m_hovering;
        const QPixmap & m_background;
};

ToolboxButton::ToolboxButton( QWidget * parent, const ToolboxItem & item, const QPixmap & pix )
    : QPushButton( parent ), m_id( item.id ), m_hovering( false ), m_background( pix )
{
    setMouseTracking( true );
    setToggleButton( true );
    resize( ToolboxButtonSize, ToolboxButtonSize );
    setPixmap( DesktopIcon( item.pixmap, ToolboxButtonIcon ) );
    QToolTip::add( this, item.text );
    setWFlags( Qt::WNoAutoErase );
#if KDE_IS_VERSION(3,3,90)
    KAcceleratorManager::setNoAccel( this );
#endif
}

int ToolboxButton::toolboxID() const
{
    return m_id;
}

void ToolboxButton::mouseMoveEvent( QMouseEvent * e )
{
    // check for mouse hovering
    const QRect myGeom( 0,0, width(), height() );
    bool hover = myGeom.contains( e->pos() );

    // if hover state changed update gfx
    if ( m_hovering != hover )
    {
        m_hovering = hover;
        update();
    }
}

void ToolboxButton::mouseReleaseEvent( QMouseEvent * e )
{
    // call default handler
    QPushButton::mouseReleaseEvent( e );

    // reset hover state when clicking
    m_hovering = false;
    update();
}

void ToolboxButton::paintEvent( QPaintEvent * e )
{
    // always not hovering in disabled state
    if ( !isEnabled() )
        m_hovering = false;

    // paint button in different flavours
    if ( isOn() || m_hovering )
    {
        // if the button is pressed or we're hovering it, use QPushButton style
        QPushButton::paintEvent( e );
    }
    else
    {
        // else draw button's pixmap over the parent's background (fake transparency)
        QPainter p( this );
        QRect backRect = e->rect();
        backRect.moveBy( x(), y() );
        p.drawPixmap( e->rect().topLeft(), m_background, backRect );
        drawButtonLabel( &p );
    }
}

/** PageViewToolbox **/

static const int ToolboxGridSize = 40;

PageViewToolbox::PageViewToolbox( QWidget * parent, QWidget * anchorWidget, bool vertical )
    : QWidget( parent, "", Qt::WNoAutoErase ), m_anchor( anchorWidget ),
    m_vertical( vertical ), m_timerID( 0 ), m_hiding( false )
{
}

void PageViewToolbox::setItems( const QValueList<ToolboxItem> & items )
{
#ifndef NDEBUG
    if ( !m_buttons.isEmpty() )
        kdDebug() << "PageViewToolbox: adding items over existing ones" << endl;
#endif
    QValueList<ToolboxItem>::const_iterator it = items.begin(), end = items.end();
    for ( ; it != end; ++it )
    {
        ToolboxButton * button = new ToolboxButton( this, *it, m_pixmap );
        connect( button, SIGNAL( clicked() ), this, SLOT( slotButtonClicked() ) );
        m_buttons.append( button );
    }
}

void PageViewToolbox::show()
{
    // rebuild pixmap and mask
    buildGfx();
    QWidget::show();

    // set scroll parameters
    m_hiding = false;
    m_currentPosition = getStartPoint();
    m_endPosition = getEndPoint();
    move( m_currentPosition );

    // deselect all buttons
    QValueList< ToolboxButton * >::iterator it = m_buttons.begin(), end = m_buttons.end();
    for ( ; it != end; ++it )
        (*it)->setOn( false );

    // start scrolling in
    if ( m_timerID )
        killTimer( m_timerID );
    m_timerID = startTimer( 20 );
}

void PageViewToolbox::hideAndDestroy()
{
    // set scroll parameters
    m_hiding = true;
    m_endPosition = getStartPoint();

    // start scrolling out
    if ( m_timerID )
        killTimer( m_timerID );
    m_timerID = startTimer( 10 );
}

void PageViewToolbox::anchorChanged()
{
    // stop timer
    if ( m_timerID )
    {
        killTimer( m_timerID );
        m_timerID = 0;
    }
    // if was hiding delete this
    if ( m_hiding )
        delete this;
    else
    {
        // else rebuild widget and set it completely visible
        buildGfx();
        m_currentPosition = getEndPoint();
        move( m_currentPosition );
        // repaint all buttons (to update background)
        QValueList< ToolboxButton * >::iterator it = m_buttons.begin(), end = m_buttons.end();
        for ( ; it != end; ++it )
            (*it)->update();
    }
}

void PageViewToolbox::paintEvent( QPaintEvent * e )
{
    // paint the internal pixmap over the widget
    bitBlt( this, e->rect().topLeft(), &m_pixmap, e->rect() );
}

void PageViewToolbox::timerEvent( QTimerEvent * )
{
    // move currentPosition towards endPosition
    int dX = m_endPosition.x() - m_currentPosition.x(),
        dY = m_endPosition.y() - m_currentPosition.y();
    dX = dX / 6 + QMAX( -2, QMIN( 2, dX) );
    dY = dY / 6 + QMAX( -2, QMIN( 2, dY) );
    m_currentPosition.setX( m_currentPosition.x() + dX );
    m_currentPosition.setY( m_currentPosition.y() + dY );

    // move the widget
    move( m_currentPosition );

    // handle arrival to the end
    if ( m_currentPosition == m_endPosition )
    {
        killTimer( m_timerID );
        m_timerID = 0;
        if ( m_hiding )
            delete this;
    }
}

QPoint PageViewToolbox::getStartPoint()
{
    if ( m_vertical )
        return QPoint( -width(), (m_anchor->height() - height()) / 2 );
    return QPoint( (m_anchor->width() - width()) / 2, -height() );
}

QPoint PageViewToolbox::getEndPoint()
{
    if ( m_vertical )
        return QPoint( 0, (m_anchor->height() - height()) / 2 );
    return QPoint( (m_anchor->width() - width()) / 2, 0 );
}

void PageViewToolbox::buildGfx()
{
    int buttonsNumber = m_buttons.count(),
        parentWidth = m_anchor->width(),
        parentHeight = m_anchor->height(),
        myCols = 1,
        myRows = 1;

    // 1. find out columns and rows we're going to use
    if ( m_vertical )
    {
        myCols = 1 + (buttonsNumber * ToolboxGridSize) /
                 (parentHeight - ToolboxGridSize);
        myRows = (int)ceilf( (float)buttonsNumber / (float)myCols );
    }
    else
    {
        myRows = 1 + (buttonsNumber * ToolboxGridSize) /
                 (parentWidth - ToolboxGridSize);
        myCols = (int)ceilf( (float)buttonsNumber / (float)myRows );
    }

    // 2. compute widget size (from rows/cols)
    int myWidth = myCols * ToolboxGridSize,
        myHeight = myRows * ToolboxGridSize,
        xOffset = (ToolboxGridSize - ToolboxButtonSize) / 2,
        yOffset = (ToolboxGridSize - ToolboxButtonSize) / 2;

    if ( m_vertical )
    {
        myHeight += 8;
         myWidth += 4;
        yOffset += 4;
    }
    else
    {
        myWidth += 8;
         myHeight += 4;
        xOffset += 4;
    }

    // 3. resize pixmap, mask and widget
    static QBitmap mask;
    mask.resize( myWidth, myHeight );
    m_pixmap.resize( myWidth, myHeight );
    resize( myWidth, myHeight );

    // 4. create and set transparency mask
    QPainter maskPainter( &mask);
    mask.fill( Qt::black );
    maskPainter.setBrush( Qt::white );
    if ( m_vertical )
        maskPainter.drawRoundRect( -10, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else
        maskPainter.drawRoundRect( 0, -10, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );
    setMask( mask );

    // 5. draw background
    QPainter bufferPainter( &m_pixmap );
    QPixmap gradientPattern;
    // 5.1. draw horizontal/vertical gradient
    if ( m_vertical )
        gradientPattern = KImageEffect::gradient( QSize( myWidth, 1),
            palette().active().button(), palette().active().light(),
            KImageEffect::HorizontalGradient );
    else
        gradientPattern = KImageEffect::gradient( QSize( 1, myHeight ),
            palette().active().button(), palette().active().light(),
            KImageEffect::VerticalGradient );
    bufferPainter.drawTiledPixmap( 0, 0, myWidth, myHeight, gradientPattern );
    // 5.2. draw rounded border
    bufferPainter.setPen( palette().active().dark() );
    if ( m_vertical )
        bufferPainter.drawRoundRect( -10, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else
        bufferPainter.drawRoundRect( 0, -10, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );

    // 6. reposition buttons (in rows/col grid)
    int gridX = 0,
        gridY = 0;
    QValueList< ToolboxButton * >::iterator it = m_buttons.begin(), end = m_buttons.end();
    for ( ; it != end; ++it )
    {
        ToolboxButton * button = *it;
        button->move( gridX * ToolboxGridSize + xOffset,
                      gridY * ToolboxGridSize + yOffset );
        if ( ++gridX == myCols )
        {
            gridX = 0;
            gridY++;
        }
    }
}

void PageViewToolbox::slotButtonClicked()
{
    ToolboxButton * button = (ToolboxButton *)sender();
    if ( button )
    {
        // deselect other buttons
        QValueList< ToolboxButton * >::iterator it = m_buttons.begin(), end = m_buttons.end();
        for ( ; it != end; ++it )
            if ( *it != button )
                (*it)->setOn( false );
        // emit signal
        if ( button->isOn() )
            emit toolSelected( button->toolboxID() );
        else
            button->setOn( true );
    }
}


/** PageViewEditTools **/

PageViewEditTools::PageViewEditTools( QWidget * parent, QWidget * anchorWidget )
    : PageViewToolbox( parent, anchorWidget, true )
{
    QValueList<ToolboxItem> items;
    items.push_back( ToolboxItem(  1, i18n("Text"), "pinnote" ) );
    items.push_back( ToolboxItem(  2, i18n("Yellow Highlighter"), "highlight_yellow" ) );
    items.push_back( ToolboxItem(  3, i18n("Orange Highlighter"), "highlight_orange" ) );
    items.push_back( ToolboxItem(  4, i18n("Green Highlighter"), "highlight_green" ) );
    items.push_back( ToolboxItem(  5, i18n("Pencil"), "pencil" ) );
    setItems( items );
    show();
}

#include "pageviewtoolbox.moc"
