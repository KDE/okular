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
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qevent.h>
#include <qstyle.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <kacceleratormanager.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kimageeffect.h>
#include <klocale.h>

// system includes
#include <math.h>

// local includes
#include "formwidgets.h"
#include "pageviewutils.h"
#include "core/page.h"
#include "settings.h"

/*********************/
/** PageViewItem     */
/*********************/

PageViewItem::PageViewItem( const Okular::Page * page )
    : m_page( page ), m_zoomFactor( 1.0 )
{
}

PageViewItem::~PageViewItem()
{
    QHash<QString, FormWidgetIface*>::iterator it = m_formWidgets.begin(), itEnd = m_formWidgets.end();
    for ( ; it != itEnd; ++it )
        delete *it;
}

const Okular::Page * PageViewItem::page() const
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

QHash<QString, FormWidgetIface*>& PageViewItem::formWidgets()
{
    return m_formWidgets;
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
    QHash<QString, FormWidgetIface*>::iterator it = m_formWidgets.begin(), itEnd = m_formWidgets.end();
    for ( ; it != itEnd; ++it )
    {
        Okular::NormalizedRect r = (*it)->rect();
        (*it)->setWidthHeight( qRound( fabs( r.right - r.left ) * w ), qRound( fabs( r.bottom - r.top ) * h ) );
    }
}

void PageViewItem::moveTo( int x, int y )
{
    m_geometry.moveLeft( x );
    m_geometry.moveTop( y );
    QHash<QString, FormWidgetIface*>::iterator it = m_formWidgets.begin(), itEnd = m_formWidgets.end();
    for ( ; it != itEnd; ++it )
    {
        Okular::NormalizedRect r = (*it)->rect();
        (*it)->moveTo( qRound( x + m_geometry.width() * r.left ) + 1, qRound( y + m_geometry.height() * r.top ) + 1 );
    }
}

void PageViewItem::invalidate()
{
    m_geometry.setRect( 0, 0, 0, 0 );
}

bool PageViewItem::setFormWidgetsVisible( bool visible )
{
    bool somehadfocus = false;
    QHash<QString, FormWidgetIface*>::iterator it = m_formWidgets.begin(), itEnd = m_formWidgets.end();
    for ( ; it != itEnd; ++it )
        somehadfocus = somehadfocus || (*it)->setVisibility( visible );
    return somehadfocus;
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
    // if the layout is LtR, we can safely place it in the right position
    if ( QApplication::isLeftToRight() )
        move( 10, 10 );
    resize( 0, 0 );
    hide();
}

void PageViewMessage::display( const QString & message, Icon icon, int durationMs )
// give Caesar what belongs to Caesar: code taken from Amarok's osd.h/.cpp
// "redde (reddite, pl.) cesari quae sunt cesaris", just btw.  :)
{
    if ( !Okular::Settings::showOSD() )
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
    maskPainter.end();
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

    // if the layout is RtL, we can move it to the right place only after we
    // know how much size it will take
    if ( QApplication::isRightToLeft() )
        move( parentWidget()->width() - geometry.width() - 10 - 1, 10 );

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


/************************/
/** PageViewTopMessage  */
/************************/

PageViewTopMessage::PageViewTopMessage( QWidget * parent )
    : QWidget( parent )
{
    setAutoFillBackground( true );
    QPalette pal = palette();
    pal.setColor( QPalette::Active, QPalette::Window, Qt::yellow );
    pal.setColor( QPalette::Inactive, QPalette::Window, Qt::yellow );
    pal.setColor( QPalette::Disabled, QPalette::Window, Qt::yellow );
    setPalette( pal );
    QHBoxLayout * lay = new QHBoxLayout( this );
    lay->setMargin( 4 );
    m_icon = new QLabel( this );
    lay->addWidget( m_icon );
    m_icon->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    m_label = new QLabel( this );
    lay->addWidget( m_label );
    m_label->setWordWrap( true );
    connect( m_label, SIGNAL( linkActivated( const QString& ) ), this, SIGNAL( action() ) );
    m_button = new QToolButton( this );
    lay->addWidget( m_button );
    QToolButton * closeButton = new QToolButton( this );
    lay->addWidget( closeButton );
    closeButton->setAutoRaise( true );
    closeButton->setIcon( closeButton->style()->standardIcon( QStyle::SP_TitleBarCloseButton ) );
    closeButton->setIconSize( QSize( 32, 32 ) );
    closeButton->setToolTip( i18n( "Close this message" ) );
    connect( closeButton, SIGNAL( clicked() ), this, SLOT( hide() ) );
    setIconSize( 32 );
    hide();
}

void PageViewTopMessage::setup( const QString & message, const KIcon& icon )
{
    m_label->setText( message );
    resize( minimumSizeHint() );
    if ( icon.isNull() )
    {
        m_icon->setPixmap( QPixmap() );
    }
    else
    {
        m_icon->setPixmap( icon.pixmap( m_icon->size() ) );
    }
}

void PageViewTopMessage::setIconSize( int size )
{
    m_icon->resize( size, size );
}

void PageViewTopMessage::setActionButton( QAction * action )
{
    m_button->setDefaultAction( action );
    m_button->setVisible( action != 0 );
}

/*********************/
/** PageViewToolBar  */
/*********************/

class ToolBarButton : public QToolButton
{
    public:
        static const int iconSize = 32;
        static const int buttonSize = 40;

        ToolBarButton( QWidget * parent, const ToolBarItem & item );
        int buttonID() const { return m_id; }

    private:
        int m_id;
};

ToolBarButton::ToolBarButton( QWidget * parent, const ToolBarItem & item )
    : QToolButton( parent ), m_id( item.id )
{
    setCheckable( true );
    setAutoRaise( true );
    resize( buttonSize, buttonSize );
    setIconSize( QSize( iconSize, iconSize ) );
    setIcon( KIcon( item.pixmap ) );
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
        ToolBarButton * button = new ToolBarButton( this, *it );
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
            vertical ? QSize( myWidth + 1, 1 ) : QSize( 1, myHeight + 1 ), fromColor, toColor,
            vertical ? KImageEffect::HorizontalGradient : KImageEffect::VerticalGradient );
    bufferPainter.drawTiledPixmap( 0, 0, myWidth + 1, myHeight + 1, QPixmap::fromImage(gradientPattern) );
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
    QLinkedList< ToolBarButton * >::const_iterator it = d->buttons.begin(), end = d->buttons.end();
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
    QLinkedList< ToolBarButton * >::const_iterator it = d->buttons.begin(), end = d->buttons.end();
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
        QLinkedList< ToolBarButton * >::const_iterator it = d->buttons.begin(), end = d->buttons.end();
        for ( ; it != end; ++it )
            if ( *it != button )
                (*it)->setChecked( false );
        // emit signal (-1 if button has been unselected)
        emit toolSelected( button->isChecked() ? button->buttonID() : -1 );
    }
}

#include "pageviewutils.moc"
