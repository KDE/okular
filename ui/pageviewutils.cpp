/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qapplication.h>
#include <qbitmap.h>
#include <qgraphicsscene.h>
#include <qimage.h>
#include <qpainter.h>
#include <qevent.h>
#include <qtimer.h>
#include <qpushbutton.h>
#include <QStyleOptionButton>
#include <kacceleratormanager.h>
#include <kiconloader.h>
#include <kimageeffect.h>
#include <klocale.h>

// system includes
#include <math.h>

// local includes
#include "pagepainter.h"
#include "pageviewannotator.h"
#include "pageviewutils.h"
#include "core/observer.h"
#include "core/page.h"
#include "settings.h"

/*********************/
/** PageViewItem     */
/*********************/

static int pageflags = PagePainter::Accessibility | PagePainter::EnhanceLinks |
                       PagePainter::EnhanceImages | PagePainter::Highlights |
                       PagePainter::TextSelection | PagePainter::Annotations;

PageViewItem::PageViewItem( const KPDFPage * page )
    : QGraphicsItem(), m_page( page ), m_zoomFactor( 1.0 ),
      m_size( QSize( 0, 0 ) ), m_globalSize( QSize( 0, 0 ) ), m_annotator( 0 )
{
    setFlags( QGraphicsItem::GraphicsItemFlags() );
}

QRectF PageViewItem::boundingRect() const
{
    return QRectF( QPointF( 0, 0 ), QSizeF( m_globalSize ) );
}

void PageViewItem::paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * )
{
    PagePainter::paintPageOnPainter( painter, m_page, PAGEVIEW_ID, pageflags,
        width(), height(), option->exposedRect.toRect() );

    const QBrush & bgbrush = scene()->backgroundBrush();
    painter->fillRect( width() + 1, 0, 2, 2, bgbrush );
    painter->fillRect( 0, height() + 1, 2, 2, bgbrush );

    painter->setPen( Qt::black );
    painter->drawRect( 0, 0, width(), height() );

    // draw bottom/right gradient
    static int levels = 2;
    int r = QColor(Qt::gray).red() / (levels + 2),
        g = QColor(Qt::gray).green() / (levels + 2),
        b = QColor(Qt::gray).blue() / (levels + 2);
    for ( int i = 1; i <= levels; ++i )
    {
        painter->setPen( QColor( r * ( i + 1 ), g * ( i + 1 ), b * ( i + 1 ) ) );
        painter->drawLine( i, i + height(), i + width(), i + height() );
        painter->drawLine( i + width(), i, i + width(), i + height() );
    }

    if ( m_annotator && m_annotator->routePaints( option->exposedRect.toRect() ) )
    {
        m_annotator->routePaint( painter, option->exposedRect.toRect() );
    }
}

int PageViewItem::type() const
{
    return UserType + 1;
}

const KPDFPage * PageViewItem::page() const
{
    return m_page;
}

int PageViewItem::pageNumber() const
{
    return m_page->number();
}

QRectF PageViewItem::geometry() const
{
    return QRectF( pos(), QSizeF( m_globalSize ) );
}

int PageViewItem::width() const
{
    return m_size.width();
}

int PageViewItem::height() const
{
    return m_size.height();
}

double PageViewItem::zoomFactor() const
{
    return m_zoomFactor;
}

void PageViewItem::setGeometry( int x, int y, int width, int height )
{
    m_size = QSize( width, height );
    m_globalSize = m_size + QSize( 2, 2 );
    setPos( x, y );
    show();
}

void PageViewItem::setWHZ( int w, int h, double z )
{
    m_size = QSize( w, h );
    m_globalSize = m_size + QSize( 2, 2 );
    m_zoomFactor = z;
    show();
}

void PageViewItem::moveTo( int x, int y )
{
    setPos( x, y );
}

void PageViewItem::invalidate()
{
    m_size = QSize( 0, 0 );
    m_globalSize = QSize( 0, 0 );
    hide();
}

void PageViewItem::setAnnotator( PageViewAnnotator * annotator )
{
    m_annotator = annotator;
}

/*********************/
/** RubberBandItem   */
/*********************/

RubberBandItem::RubberBandItem( const QColor & color )
    : QGraphicsItem(), m_size( QSize( 0, 0 ) ), m_color( color )
{
    setFlags( QGraphicsItem::GraphicsItemFlags() );
    setZValue( 10 );
    show();
}

QRectF RubberBandItem::boundingRect() const
{
    return QRectF( QPointF( 0, 0 ), QSizeF( m_size ) );
}

void RubberBandItem::paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * )
{
    QColor color = ( abs( m_size.width() ) <= 8 && abs( m_size.height() ) <= 8 ) ? Qt::red : m_color;

    if ( KpdfSettings::enableCompositing() )
    {
        painter->fillRect( option->exposedRect, QBrush( color, Qt::Dense4Pattern ) );
    }
    painter->setPen( color );
    painter->drawRect( boundingRect() );
}

int RubberBandItem::type() const
{
    return UserType + 2;
}

QRectF RubberBandItem::geometry() const
{
    return QRectF( pos(), QSizeF( m_size ) ).normalized();
}

void RubberBandItem::resize( int width, int height )
{
    QRectF updatedRect = geometry();

    m_size = QSize( width, height );
    updatedRect |= geometry();

    scene()->update( updatedRect );
}

void RubberBandItem::resizeTo( const QPointF & finalPoint )
{
    QRectF updatedRect = geometry();

    QPointF delta = finalPoint - pos();
    m_size = QSize( (int)delta.x(), (int)delta.y() );

    updatedRect |= geometry();
    scene()->update( updatedRect );
}

/*********************/
/** PageViewMessage  */
/*********************/

PageViewMessage::PageViewMessage( QWidget * parent )
    : QWidget( parent ), m_timer( 0 )
{
    setObjectName( "pageViewMessage" );
    setFocusPolicy( Qt::NoFocus );
    setAttribute( Qt::WA_OpaquePaintEvent );
    QPalette pal = palette();
    pal.setColor( QPalette::Active, QPalette::Window, QApplication::palette().color( QPalette::Active, QPalette::Window ) );
    setPalette( pal );
    move( 10, 10 );
    resize( 0, 0 );
    hide();
}

void PageViewMessage::display( const QString & message, Icon icon, int durationMs )
// give Caesar what belongs to Caesar: code taken from Amarok's osd.h/.cpp
// "redde (reddite, pl.) cesari quae sunt cesaris", just btw.  :)
{
    if ( !KpdfSettings::showOSD() )
    {
        hide();
        return;
    }

    // determine text rectangle
    QRect textRect = fontMetrics().boundingRect( message );
    textRect.translate( -textRect.left(), -textRect.top() );
    textRect.adjust( 0, 0, 2, 2 );
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
            case Annotation:
                symbol = SmallIcon( "pencil" );
                break;
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
        textXOffset = 2 + symbol.width();
        width += textXOffset;
        height = qMax( height, symbol.height() );
    }
    QRect geometry( 0, 0, width + 10, height + 8 );

    // resize pixmap, mask and widget
    static QBitmap mask;
    mask = QBitmap( geometry.size() + QSize( 1, 1 ) );
    m_pixmap = QPixmap( geometry.size() + QSize( 1, 1 ) );
    resize( geometry.size() + QSize( 1, 1 ) );

    // create and set transparency mask
    QPainter maskPainter( &mask);
    mask.fill( Qt::white );
    maskPainter.setBrush( Qt::black );
    maskPainter.drawRoundRect( geometry, 1600 / geometry.width(), 1600 / geometry.height() );
    setMask( mask );

    // draw background
    QPalette pal = palette();
    QPainter bufferPainter( &m_pixmap );
    bufferPainter.setPen( Qt::black );
    bufferPainter.setBrush( pal.color( QPalette::Window ) );
    bufferPainter.drawRoundRect( geometry, 1600 / geometry.width(), 1600 / geometry.height() );

    // draw icon if present
    if ( !symbol.isNull() )
        bufferPainter.drawPixmap( 5, 4, symbol, 0, 0, symbol.width(), symbol.height() );

    // draw shadow and text
    int yText = geometry.height() - height / 2;
    bufferPainter.setPen( pal.color( QPalette::Window ).dark( 115 ) );
    bufferPainter.drawText( 5 + textXOffset + shadowOffset, yText + 1, message );
    bufferPainter.setPen( pal.color( QPalette::WindowText ) );
    bufferPainter.drawText( 5 + textXOffset, yText, message );

    // show widget and schedule a repaint
    show();
    update();

    // close the message window after given mS
    if ( durationMs > 0 )
    {
        if ( !m_timer )
        {
            m_timer = new QTimer( this );
            m_timer->setSingleShot( true );
            connect( m_timer, SIGNAL( timeout() ), SLOT( hide() ) );
        }
        m_timer->start( durationMs );
    } else if ( m_timer )
        m_timer->stop();
}

void PageViewMessage::paintEvent( QPaintEvent * e )
{
    // paint the internal pixmap over the widget
    QPainter p( this );
    p.drawPixmap( e->rect().topLeft(), m_pixmap, e->rect() );
}

void PageViewMessage::mousePressEvent( QMouseEvent * /*e*/ )
{
    if ( m_timer )
        m_timer->stop();
    hide();
}


/*********************/
/** PageViewToolBar  */
/*********************/

class ToolBarButton : public QPushButton
{
    public:
        static const int iconSize = 32;
        static const int buttonSize = 40;

        ToolBarButton( QWidget * parent, const ToolBarItem & item, const QPixmap & parentPix );
        int buttonID() const { return m_id; }

    protected:
        void mouseMoveEvent( QMouseEvent * e );
        void paintEvent( QPaintEvent * e );

    private:
        int m_id;
        bool m_hovering;
        const QPixmap & m_background;
};

ToolBarButton::ToolBarButton( QWidget * parent, const ToolBarItem & item, const QPixmap & pix )
    : QPushButton( parent ), m_id( item.id ), m_hovering( false ), m_background( pix )
{
    setMouseTracking( true );
    setCheckable( true );
    resize( buttonSize, buttonSize );
    setIconSize( QSize( iconSize, iconSize ) );
    setIcon( DesktopIconSet( item.pixmap, iconSize ) );
    setAttribute( Qt::WA_OpaquePaintEvent );
    // set shortcut if defined
    if ( !item.shortcut.isEmpty() )
        setShortcut( QKeySequence( item.shortcut ) );
    else
        KAcceleratorManager::setNoAccel( this );

    // if accel is set display it along name
    QString accelString = shortcut().toString( QKeySequence::NativeText );
    if ( !accelString.isEmpty() )
        setToolTip( QString("%1 [%2]").arg( item.text ).arg( accelString ) );
    else
        setToolTip( item.text );
}

void ToolBarButton::mouseMoveEvent( QMouseEvent * e )
{
    // if hovering changes, update gfx
    bool hover = QRect( 0, 0, width(), height() ) .contains( e->pos() );
    if ( m_hovering != hover )
    {
        m_hovering = hover;
        update();
    }
}

void ToolBarButton::paintEvent( QPaintEvent * e )
{
    // if the button is pressed or we're hovering it, use QPushButton style
    if ( isChecked() || m_hovering )
    {
        QPushButton::paintEvent( e );
        return;
    }

    // draw button's pixmap over the parent's background (fake transparency)
    QPainter p( this );
    QRect backRect = e->rect();
    backRect.translate( x(), y() );
    p.drawPixmap( e->rect().topLeft(), m_background, backRect );
    QStyleOptionButton opt;
    opt.initFrom( this );
    opt.features = QStyleOptionButton::Flat;
    opt.icon = icon();
    opt.iconSize = QSize( iconSize, iconSize );
    opt.text = text();
    style()->drawControl( QStyle::CE_PushButton, &opt, &p, this );
}

/* PageViewToolBar */

static const int toolBarGridSize = 40;
static const int toolBarRBMargin = 2;

struct ToolBarPrivate
{
    // anchored widget and side
    QWidget * anchorWidget;
    PageViewToolBar::Side anchorSide;

    // slide in/out stuff
    QTimer * animTimer;
    QPoint currentPosition;
    QPoint endPosition;
    bool hiding;

    // background pixmap and buttons
    QPixmap backgroundPixmap;
    QLinkedList< ToolBarButton * > buttons;
};

PageViewToolBar::PageViewToolBar( QWidget * parent, QWidget * anchorWidget )
    : QWidget( parent ), d( new ToolBarPrivate )
{
    setAttribute( Qt::WA_OpaquePaintEvent, true );
    setMouseTracking( true );

    // initialize values of the private data storage structure
    d->anchorWidget = anchorWidget;
    d->anchorSide = Left;
    d->hiding = false;

    // create the animation timer
    d->animTimer = new QTimer( this );
    connect( d->animTimer, SIGNAL( timeout() ), this, SLOT( slotAnimate() ) );

    // apply a filter to get notified when anchor changes geometry
    d->anchorWidget->installEventFilter( this );
}

PageViewToolBar::~PageViewToolBar()
{
    // delete the private data storage structure
    delete d;
}

void PageViewToolBar::showItems( Side side, const QLinkedList<ToolBarItem> & items )
{
    // set parameters for sliding in
    d->anchorSide = side;
    d->hiding = false;

    // delete buttons if already present
    if ( !d->buttons.isEmpty() )
    {
        QLinkedList< ToolBarButton * >::iterator it = d->buttons.begin(), end = d->buttons.end();
        for ( ; it != end; ++it )
            delete *it;
        d->buttons.clear();
    }

    // create new buttons for given items
    QLinkedList<ToolBarItem>::const_iterator it = items.begin(), end = items.end();
    for ( ; it != end; ++it )
    {
        ToolBarButton * button = new ToolBarButton( this, *it, d->backgroundPixmap );
        connect( button, SIGNAL( clicked() ), this, SLOT( slotButtonClicked() ) );
        d->buttons.append( button );
    }

    // rebuild toolbar shape and contents
    buildToolBar();
    d->currentPosition = getOuterPoint();
    d->endPosition = getInnerPoint();
    move( d->currentPosition );
    show();

    // start scrolling in
    d->animTimer->start( 20 );
}

void PageViewToolBar::hideAndDestroy()
{
    // set parameters for sliding out
    d->hiding = true;
    d->endPosition = getOuterPoint();

    // start scrolling out
    d->animTimer->start( 20 );
}

bool PageViewToolBar::eventFilter( QObject * obj, QEvent * e )
{
    // if anchorWidget changed geometry reposition toolbar
    if ( obj == d->anchorWidget && e->type() == QEvent::Resize )
    {
        d->animTimer->stop();
        if ( d->hiding )
            deleteLater();
        else
            reposition();
    }

    // don't block event
    return false;
}

void PageViewToolBar::paintEvent( QPaintEvent * e )
{
    // paint the internal pixmap over the widget
    QPainter p( this );
    p.drawPixmap( e->rect().topLeft(), d->backgroundPixmap, e->rect() );
}

void PageViewToolBar::mousePressEvent( QMouseEvent * e )
{
    // set 'dragging' cursor
    if ( e->button() == Qt::LeftButton )
        setCursor( Qt::SizeAllCursor );
}

void PageViewToolBar::mouseMoveEvent( QMouseEvent * e )
{
    if ( ( QApplication::mouseButtons() & Qt::LeftButton ) != Qt::LeftButton )
        return;

    // compute the nearest side to attach the widget to
    QPoint parentPos = mapToParent( e->pos() );
    float nX = (float)parentPos.x() / (float)d->anchorWidget->width(),
          nY = (float)parentPos.y() / (float)d->anchorWidget->height();
    if ( nX > 0.3 && nX < 0.7 && nY > 0.3 && nY < 0.7 )
        return;
    bool LT = nX < (1.0 - nY);
    bool LB = nX < (nY);
    Side side = LT ? ( LB ? Left : Top ) : ( LB ? Bottom : Right );

    // check if side changed
    if ( side == d->anchorSide )
        return;

    d->anchorSide = side;
    reposition();
    emit orientationChanged( (int)side );
}

void PageViewToolBar::mouseReleaseEvent( QMouseEvent * e )
{
    // set normal cursor
    if ( e->button() == Qt::LeftButton )
        setCursor( Qt::ArrowCursor );
}

void PageViewToolBar::buildToolBar()
{
    int buttonsNumber = d->buttons.count(),
        parentWidth = d->anchorWidget->width(),
        parentHeight = d->anchorWidget->height(),
        myCols = 1,
        myRows = 1;

    // 1. find out columns and rows we're going to use
    bool topLeft = d->anchorSide == Left || d->anchorSide == Top;
    bool vertical = d->anchorSide == Left || d->anchorSide == Right;
    if ( vertical )
    {
        myCols = 1 + (buttonsNumber * toolBarGridSize) /
                 (parentHeight - toolBarGridSize);
        myRows = (int)ceil( (float)buttonsNumber / (float)myCols );
    }
    else
    {
        myRows = 1 + (buttonsNumber * toolBarGridSize) /
                 (parentWidth - toolBarGridSize);
        myCols = (int)ceil( (float)buttonsNumber / (float)myRows );
    }

    // 2. compute widget size (from rows/cols)
    int myWidth = myCols * toolBarGridSize,
        myHeight = myRows * toolBarGridSize,
        xOffset = (toolBarGridSize - ToolBarButton::buttonSize) / 2,
        yOffset = (toolBarGridSize - ToolBarButton::buttonSize) / 2;

    if ( vertical )
    {
        myHeight += 16;
        myWidth += 4;
        yOffset += 12;
        if ( d->anchorSide == Right )
            xOffset += 4;
    }
    else
    {
        myWidth += 16;
        myHeight += 4;
        xOffset += 12;
        if ( d->anchorSide == Bottom )
            yOffset += 4;
    }

    bool prevUpdates = updatesEnabled();
    setUpdatesEnabled( false );

    // 3. resize pixmap, mask and widget
    static QBitmap mask;
    mask = QBitmap( myWidth + 1, myHeight + 1 );
    d->backgroundPixmap = QPixmap( myWidth + 1, myHeight + 1 );
    resize( myWidth + 1, myHeight + 1 );

    // 4. create and set transparency mask
    QPainter maskPainter( &mask);
    mask.fill( Qt::white );
    maskPainter.setBrush( Qt::black );
    if ( vertical )
        maskPainter.drawRoundRect( topLeft ? -10 : 0, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else
        maskPainter.drawRoundRect( 0, topLeft ? -10 : 0, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );
    setMask( mask );

    // 5. draw background
    QPainter bufferPainter( &d->backgroundPixmap );
    QPalette pal = palette();
    // 5.1. draw horizontal/vertical gradient
    QColor fromColor = topLeft ? pal.color( QPalette::Active, QPalette::Button ) : pal.color( QPalette::Active, QPalette::Light );
    QColor toColor = topLeft ? pal.color( QPalette::Active, QPalette::Light ) : pal.color( QPalette::Active, QPalette::Button );
    QImage gradientPattern = KImageEffect::gradient(
            vertical ? QSize( myWidth, 1) : QSize( 1, myHeight ), fromColor, toColor,
            vertical ? KImageEffect::HorizontalGradient : KImageEffect::VerticalGradient );
    bufferPainter.drawTiledPixmap( 0, 0, myWidth, myHeight, QPixmap::fromImage(gradientPattern) );
    // 5.2. draw rounded border
    bufferPainter.setPen( pal.color( QPalette::Active, QPalette::Dark ) );
    if ( vertical )
        bufferPainter.drawRoundRect( topLeft ? -10 : 0, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else
        bufferPainter.drawRoundRect( 0, topLeft ? -10 : 0, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );
    // 5.3. draw handle
    bufferPainter.setPen( pal.color( QPalette::Active, QPalette::Mid ) );
    if ( vertical )
    {
        int dx = d->anchorSide == Left ? 2 : 4;
        bufferPainter.drawLine( dx, 6, dx + myWidth - 8, 6 );
        bufferPainter.drawLine( dx, 9, dx + myWidth - 8, 9 );
        bufferPainter.setPen( pal.color( QPalette::Active, QPalette::Light ) );
        bufferPainter.drawLine( dx + 1, 7, dx + myWidth - 7, 7 );
        bufferPainter.drawLine( dx + 1, 10, dx + myWidth - 7, 10 );
    }
    else
    {
        int dy = d->anchorSide == Top ? 2 : 4;
        bufferPainter.drawLine( 6, dy, 6, dy + myHeight - 8 );
        bufferPainter.drawLine( 9, dy, 9, dy + myHeight - 8 );
        bufferPainter.setPen( pal.color( QPalette::Active, QPalette::Light ) );
        bufferPainter.drawLine( 7, dy + 1, 7, dy + myHeight - 7 );
        bufferPainter.drawLine( 10, dy + 1, 10, dy + myHeight - 7 );
    }

    // 6. reposition buttons (in rows/col grid)
    int gridX = 0,
        gridY = 0;
    QLinkedList< ToolBarButton * >::iterator it = d->buttons.begin(), end = d->buttons.end();
    for ( ; it != end; ++it )
    {
        ToolBarButton * button = *it;
        button->move( gridX * toolBarGridSize + xOffset,
                      gridY * toolBarGridSize + yOffset );
        if ( ++gridX == myCols )
        {
            gridX = 0;
            gridY++;
        }
    }

    setUpdatesEnabled( prevUpdates );
}

void PageViewToolBar::reposition()
{
    // note: hiding widget here will gives better gfx, but ends drag operation
    // rebuild widget and move it to its final place
    buildToolBar();
    d->currentPosition = getInnerPoint();
    move( d->currentPosition );

    // repaint all buttons (to update background)
    QLinkedList< ToolBarButton * >::iterator it = d->buttons.begin(), end = d->buttons.end();
    for ( ; it != end; ++it )
        (*it)->update();
}

QPoint PageViewToolBar::getInnerPoint()
{
    // returns the final position of the widget
    if ( d->anchorSide == Left )
        return QPoint( 0, (d->anchorWidget->height() - height()) / 2 );
    if ( d->anchorSide == Top )
        return QPoint( (d->anchorWidget->width() - width()) / 2, 0 );
    if ( d->anchorSide == Right )
        return QPoint( d->anchorWidget->width() - width() + toolBarRBMargin, (d->anchorWidget->height() - height()) / 2 );
    return QPoint( (d->anchorWidget->width() - width()) / 2, d->anchorWidget->height() - height() + toolBarRBMargin );
}

QPoint PageViewToolBar::getOuterPoint()
{
    // returns the point from which the transition starts
    if ( d->anchorSide == Left )
        return QPoint( -width(), (d->anchorWidget->height() - height()) / 2 );
    if ( d->anchorSide == Top )
        return QPoint( (d->anchorWidget->width() - width()) / 2, -height() );
    if ( d->anchorSide == Right )
        return QPoint( d->anchorWidget->width() + toolBarRBMargin, (d->anchorWidget->height() - height()) / 2 );
    return QPoint( (d->anchorWidget->width() - width()) / 2, d->anchorWidget->height() + toolBarRBMargin );
}

void PageViewToolBar::slotAnimate()
{
    // move currentPosition towards endPosition
    int dX = d->endPosition.x() - d->currentPosition.x(),
        dY = d->endPosition.y() - d->currentPosition.y();
    dX = dX / 6 + qMax( -1, qMin( 1, dX) );
    dY = dY / 6 + qMax( -1, qMin( 1, dY) );
    d->currentPosition.setX( d->currentPosition.x() + dX );
    d->currentPosition.setY( d->currentPosition.y() + dY );

    // move the widget
    move( d->currentPosition );

    // handle arrival to the end
    if ( d->currentPosition == d->endPosition )
    {
        d->animTimer->stop();
        if ( d->hiding )
            deleteLater();
    }
}

void PageViewToolBar::slotButtonClicked()
{
    ToolBarButton * button = qobject_cast<ToolBarButton *>( sender() );
    if ( button )
    {
        // deselect other buttons
        QLinkedList< ToolBarButton * >::iterator it = d->buttons.begin(), end = d->buttons.end();
        for ( ; it != end; ++it )
            if ( *it != button )
                (*it)->setChecked( false );
        // emit signal (-1 if button has been unselected)
        emit toolSelected( button->isChecked() ? button->buttonID() : -1 );
    }
}

#include "pageviewutils.moc"
