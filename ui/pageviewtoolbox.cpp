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
#include "conf/settings.h"

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
static const int AnchorRBMargin = 2;

PageViewToolbox::PageViewToolbox( QWidget * parent, QWidget * anchorWidget )
    : QWidget( parent, "", Qt::WNoAutoErase ), m_anchor( anchorWidget ),
    m_side( Left ), m_timerID( 0 ), m_hiding( false )
{
}

void PageViewToolbox::showItems( Side side, const QValueList<ToolboxItem> & items )
{
    // cache side
    m_side = side;

    // delete buttons if already present
    if ( !m_buttons.isEmpty() )
    {
        QValueList< ToolboxButton * >::iterator it = m_buttons.begin(), end = m_buttons.end();
        for ( ; it != end; ++it )
            delete *it;
        m_buttons.clear();
    }

    // create new buttons for given items
    QValueList<ToolboxItem>::const_iterator it = items.begin(), end = items.end();
    for ( ; it != end; ++it )
    {
        ToolboxButton * button = new ToolboxButton( this, *it, m_pixmap );
        connect( button, SIGNAL( clicked() ), this, SLOT( slotButtonClicked() ) );
        m_buttons.append( button );
    }

    // rebuild pixmap and mask
    buildGfx();
    QWidget::show();

    // set scroll parameters
    m_hiding = false;
    m_currentPosition = getStartPoint();
    m_endPosition = getEndPoint();
    move( m_currentPosition );

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
        showFinal();
}


void PageViewToolbox::mousePressEvent( QMouseEvent * e )
{
    // set 'dragging' cursor
    if ( e->button() == Qt::LeftButton )
        setCursor( sizeAllCursor );
}

void PageViewToolbox::mouseMoveEvent( QMouseEvent * e )
{
    if ( e->state() != Qt::LeftButton )
        return;

    // compute the nearest side
    QPoint parentPos = mapToParent( e->pos() );
    float nX = (float)parentPos.x() / (float)m_anchor->width(),
          nY = (float)parentPos.y() / (float)m_anchor->height();
    if ( nX > 0.3 && nX < 0.7 && nY > 0.3 && nY < 0.7 )
        return;
    bool LT = nX < (1.0 - nY);
    bool LB = nX < (nY);
    Side side = LT ? ( LB ? Left : Top ) : ( LB ? Bottom : Right );

    // if side changed, update stuff
    if ( side != m_side )
    {
        // inform subclasses about orientation change
        orientationChanged( m_side = side );

        // rebuild and display the widget
        showFinal();
    }
}

void PageViewToolbox::mouseReleaseEvent( QMouseEvent * e )
{
    // set normal cursor
    if ( e->button() == Qt::LeftButton )
        setCursor( arrowCursor );
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
    dX = dX / 6 + QMAX( -1, QMIN( 1, dX) );
    dY = dY / 6 + QMAX( -1, QMIN( 1, dY) );
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


void PageViewToolbox::showFinal()
{
    // the following gives a better gfx result on repositioning, but ends
    // drag operation
    //QWidget::hide();

    // rebuild widget and move it to its final place
    buildGfx();
    m_currentPosition = getEndPoint();
    move( m_currentPosition );

    // repaint all buttons (to update background)
    QValueList< ToolboxButton * >::iterator it = m_buttons.begin(), end = m_buttons.end();
    for ( ; it != end; ++it )
        (*it)->update();

    //QWidget::show();
}

void PageViewToolbox::buildGfx()
{
    int buttonsNumber = m_buttons.count(),
        parentWidth = m_anchor->width(),
        parentHeight = m_anchor->height(),
        myCols = 1,
        myRows = 1;

    // 1. find out columns and rows we're going to use
    bool m_vertical = m_side == Left || m_side == Right;
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
        myHeight += 16;
        myWidth += 4;
        yOffset += 12;
        if ( m_side == Right )
            xOffset += 4;
    }
    else
    {
        myWidth += 16;
        myHeight += 4;
        xOffset += 12;
        if ( m_side == Bottom )
            yOffset += 4;
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
    if ( m_side == Left )
        maskPainter.drawRoundRect( -10, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else if ( m_side == Top )
        maskPainter.drawRoundRect( 0, -10, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );
    else if ( m_side == Right )
        maskPainter.drawRoundRect( 0, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else // ( m_side == Bottom )
        maskPainter.drawRoundRect( 0, 0, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );
    setMask( mask );

    // 5. draw background
    QPainter bufferPainter( &m_pixmap );
    QPixmap gradientPattern;
    // 5.1. draw horizontal/vertical gradient
    if ( m_side == Left )
        gradientPattern = KImageEffect::gradient( QSize( myWidth, 1),
            palette().active().button(), palette().active().light(),
            KImageEffect::HorizontalGradient );
    else if ( m_side == Top )
        gradientPattern = KImageEffect::gradient( QSize( 1, myHeight ),
            palette().active().button(), palette().active().light(),
            KImageEffect::VerticalGradient );
    else if ( m_side == Right )
        gradientPattern = KImageEffect::gradient( QSize( myWidth, 1),
            palette().active().light(), palette().active().button(),
            KImageEffect::HorizontalGradient );
    else // ( m_side == Bottom )
        gradientPattern = KImageEffect::gradient( QSize( 1, myHeight ),
            palette().active().light(), palette().active().button(),
            KImageEffect::VerticalGradient );
    bufferPainter.drawTiledPixmap( 0, 0, myWidth, myHeight, gradientPattern );
    // 5.2. draw rounded border
    bufferPainter.setPen( palette().active().dark() );
    if ( m_side == Left )
        bufferPainter.drawRoundRect( -10, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else if ( m_side == Top )
        bufferPainter.drawRoundRect( 0, -10, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );
    else if ( m_side == Right )
        bufferPainter.drawRoundRect( 0, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else // ( m_side == Bottom )
        bufferPainter.drawRoundRect( 0, 0, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );
    // 5.3. draw handle
    bufferPainter.setPen( palette().active().mid() );
    if ( m_vertical )
    {
        int dx = m_side == Left ? 2 : 4;
        bufferPainter.drawLine( dx, 6, dx + myWidth - 8, 6 );
        bufferPainter.drawLine( dx, 9, dx + myWidth - 8, 9 );
        bufferPainter.setPen( palette().active().light() );
        bufferPainter.drawLine( dx + 1, 7, dx + myWidth - 7, 7 );
        bufferPainter.drawLine( dx + 1, 10, dx + myWidth - 7, 10 );
    }
    else
    {
        int dy = m_side == Top ? 2 : 4;
        bufferPainter.drawLine( 6, dy, 6, dy + myHeight - 8 );
        bufferPainter.drawLine( 9, dy, 9, dy + myHeight - 8 );
        bufferPainter.setPen( palette().active().light() );
        bufferPainter.drawLine( 7, dy + 1, 7, dy + myHeight - 7 );
        bufferPainter.drawLine( 10, dy + 1, 10, dy + myHeight - 7 );
    }

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

QPoint PageViewToolbox::getEndPoint()
{
    // returns the final position of the widget
    if ( m_side == Left )
        return QPoint( 0, (m_anchor->height() - height()) / 2 );
    if ( m_side == Top )
        return QPoint( (m_anchor->width() - width()) / 2, 0 );
    if ( m_side == Right )
        return QPoint( m_anchor->width() - width() + AnchorRBMargin, (m_anchor->height() - height()) / 2 );
    return QPoint( (m_anchor->width() - width()) / 2, m_anchor->height() - height() + AnchorRBMargin );
}

QPoint PageViewToolbox::getStartPoint()
{
    // returns the point from which the transition starts
    if ( m_side == Left )
        return QPoint( -width(), (m_anchor->height() - height()) / 2 );
    if ( m_side == Top )
        return QPoint( (m_anchor->width() - width()) / 2, -height() );
    if ( m_side == Right )
        return QPoint( m_anchor->width() + AnchorRBMargin, (m_anchor->height() - height()) / 2 );
    return QPoint( (m_anchor->width() - width()) / 2, m_anchor->height() + AnchorRBMargin );
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
        // emit signal (-1 if button has been unselected)
        emit toolSelected( button->isOn() ? button->toolboxID() : -1 );
    }
}


/** PageViewEditTools **/

PageViewEditTools::PageViewEditTools( QWidget * parent, QWidget * anchorWidget )
    : PageViewToolbox( parent, anchorWidget )
{
    // create the ToolboxItems and show it on last place
    QValueList<ToolboxItem> items;
    items.push_back( ToolboxItem(  1, i18n("Text"), "pinnote" ) );
    items.push_back( ToolboxItem(  2, i18n("Yellow Highlighter"), "highlight_yellow" ) );
    items.push_back( ToolboxItem(  3, i18n("Orange Highlighter"), "highlight_orange" ) );
    items.push_back( ToolboxItem(  4, i18n("Green Highlighter"), "highlight_green" ) );
    items.push_back( ToolboxItem(  5, i18n("Pencil"), "pencil" ) );
    items.push_back( ToolboxItem(  6, i18n("sbiancante"), "coprex-white" ) );
    showItems( (Side)Settings::editToolboxPlacement(), items );
}

void PageViewEditTools::orientationChanged( Side side )
{
    // save position to the conficuration file
    Settings::setEditToolboxPlacement( (int)side );
}

#include "pageviewtoolbox.moc"
