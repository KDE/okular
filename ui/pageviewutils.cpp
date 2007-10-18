/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "pageviewutils.h"

// qt/kde includes
#include <qapplication.h>
#include <qbitmap.h>
#include <qbrush.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qevent.h>
#include <qstyle.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <kacceleratormanager.h>
#include <kiconloader.h>
#include <klocale.h>

// system includes
#include <math.h>

// local includes
#include "formwidgets.h"
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
    foreach(FormWidgetIface *fwi, m_formWidgets)
    {
        Okular::NormalizedRect r = fwi->rect();
        fwi->setWidthHeight( qRound( fabs( r.right - r.left ) * w ), qRound( fabs( r.bottom - r.top ) * h ) );
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
    {
        bool hadfocus = (*it)->setVisibility( visible );
        somehadfocus = somehadfocus || hadfocus;
    }
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
                symbol = SmallIcon( "zoom-original" );
                break;
            case Error:
                symbol = SmallIcon( "dialog-error" );
                break;
            case Warning:
                symbol = SmallIcon( "dialog-warning" );
                break;
            default:
                symbol = SmallIcon( "dialog-information" );
                break;
        }
        textXOffset = 2 + symbol.width();
        width += textXOffset;
        height = qMax( height, symbol.height() );
    }
    QRect geometry( 0, 0, width + 10, height + 8 );

    // resize pixmap, mask and widget
    QBitmap mask( geometry.size() + QSize( 1, 1 ) );
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
    m_button->setVisible( false );
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

ToolBarButton::ToolBarButton( QWidget * parent, const AnnotationToolItem &item )
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

class ToolBarPrivate
{
public:
    ToolBarPrivate( PageViewToolBar * qq )
        : q( qq )
    {
    }

    // rebuild contents and reposition then widget
    void buildToolBar();
    void reposition();
    // compute the visible and hidden positions along current side
    QPoint getInnerPoint() const;
    QPoint getOuterPoint() const;
    void selectButton( ToolBarButton * button );

    PageViewToolBar * q;

    // anchored widget and side
    QWidget * anchorWidget;
    PageViewToolBar::Side anchorSide;

    // slide in/out stuff
    QTimer * animTimer;
    QPoint currentPosition;
    QPoint endPosition;
    bool hiding;
    bool visible;

    // background pixmap and buttons
    QPixmap backgroundPixmap;
    QLinkedList< ToolBarButton * > buttons;
};

PageViewToolBar::PageViewToolBar( QWidget * parent, QWidget * anchorWidget )
    : QWidget( parent ), d( new ToolBarPrivate( this ) )
{
    setAttribute( Qt::WA_OpaquePaintEvent, true );

    // initialize values of the private data storage structure
    d->anchorWidget = anchorWidget;
    d->anchorSide = Left;
    d->hiding = false;
    d->visible = false;

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

void PageViewToolBar::setItems( const QLinkedList<AnnotationToolItem> &items )
{
    // delete buttons if already present
    if ( !d->buttons.isEmpty() )
    {
        QLinkedList< ToolBarButton * >::iterator it = d->buttons.begin(), end = d->buttons.end();
        for ( ; it != end; ++it )
            delete *it;
        d->buttons.clear();
    }

    // create new buttons for given items
    QLinkedList<AnnotationToolItem>::const_iterator it = items.begin(), end = items.end();
    for ( ; it != end; ++it )
    {
        ToolBarButton * button = new ToolBarButton( this, *it );
        connect( button, SIGNAL( clicked() ), this, SLOT( slotButtonClicked() ) );
        d->buttons.append( button );
    }

    // rebuild toolbar shape and contents
    d->reposition();
}

void PageViewToolBar::setSide( Side side )
{
    d->anchorSide = side;

    d->reposition();
}

void PageViewToolBar::showAndAnimate()
{
    // set parameters for sliding in
    d->hiding = false;

    show();

    // start scrolling in
    d->animTimer->start( 20 );
}

void PageViewToolBar::hideAndDestroy()
{
    // set parameters for sliding out
    d->hiding = true;
    d->endPosition = d->getOuterPoint();

    // start scrolling out
    d->animTimer->start( 20 );
}

void PageViewToolBar::selectButton( int id )
{
    ToolBarButton * button = 0;
    if ( id >= 0 && id < d->buttons.count() )
        button = *(d->buttons.begin() + id);
    else
    {
        QLinkedList< ToolBarButton * >::const_iterator it = d->buttons.begin(), end = d->buttons.end();
        for ( ; !button && it != end; ++it )
            if ( (*it)->isChecked() )
                button = *it;
        if ( button )
            button->setChecked( false );
    }
    d->selectButton( button );
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
            d->reposition();
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
    d->reposition();
    emit orientationChanged( (int)side );
}

void PageViewToolBar::mouseReleaseEvent( QMouseEvent * e )
{
    // set normal cursor
    if ( e->button() == Qt::LeftButton )
        setCursor( Qt::ArrowCursor );
}

void ToolBarPrivate::buildToolBar()
{
    int buttonsNumber = buttons.count(),
        parentWidth = anchorWidget->width(),
        parentHeight = anchorWidget->height(),
        myCols = 1,
        myRows = 1;

    // 1. find out columns and rows we're going to use
    bool topLeft = anchorSide == PageViewToolBar::Left || anchorSide == PageViewToolBar::Top;
    bool vertical = anchorSide == PageViewToolBar::Left || anchorSide == PageViewToolBar::Right;
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
        if ( anchorSide == PageViewToolBar::Right )
            xOffset += 4;
    }
    else
    {
        myWidth += 16;
        myHeight += 4;
        xOffset += 12;
        if ( anchorSide == PageViewToolBar::Bottom )
            yOffset += 4;
    }

    bool prevUpdates = q->updatesEnabled();
    q->setUpdatesEnabled( false );

    // 3. resize pixmap, mask and widget
    QBitmap mask( myWidth + 1, myHeight + 1 );
    backgroundPixmap = QPixmap( myWidth + 1, myHeight + 1 );
    q->resize( myWidth + 1, myHeight + 1 );

    // 4. create and set transparency mask
    QPainter maskPainter( &mask);
    mask.fill( Qt::white );
    maskPainter.setBrush( Qt::black );
    if ( vertical )
        maskPainter.drawRoundRect( topLeft ? -10 : 0, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else
        maskPainter.drawRoundRect( 0, topLeft ? -10 : 0, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );
    maskPainter.end();
    q->setMask( mask );

    // 5. draw background
    QPainter bufferPainter( &backgroundPixmap );
    QPalette pal = q->palette();
    // 5.1. draw horizontal/vertical gradient
    QLinearGradient grad;
    switch ( anchorSide )
    {
        case PageViewToolBar::Left:
            grad = QLinearGradient( 0, 1, myWidth + 1, 1 );
            break;
        case PageViewToolBar::Right:
            grad = QLinearGradient( myWidth + 1, 1, 0, 1 );
            break;
        case PageViewToolBar::Top:
            grad = QLinearGradient( 1, 0, 1, myHeight + 1 );
            break;
        case PageViewToolBar::Bottom:
            grad = QLinearGradient( 1, myHeight + 1, 0, 1 );
            break;
    }
    grad.setColorAt( 0, pal.color( QPalette::Active, QPalette::Button ) );
    grad.setColorAt( 1, pal.color( QPalette::Active, QPalette::Light ) );
    bufferPainter.fillRect( 0, 0, myWidth + 1, myHeight + 1, grad );
    // 5.2. draw rounded border
    bufferPainter.setPen( pal.color( QPalette::Active, QPalette::Dark ) );
    bufferPainter.setRenderHints( QPainter::Antialiasing );
    if ( vertical )
        bufferPainter.drawRoundRect( topLeft ? -10 : 0, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else
        bufferPainter.drawRoundRect( 0, topLeft ? -10 : 0, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );
    // 5.3. draw handle
    bufferPainter.setPen( pal.color( QPalette::Active, QPalette::Mid ) );
    if ( vertical )
    {
        int dx = anchorSide == PageViewToolBar::Left ? 2 : 4;
        bufferPainter.drawLine( dx, 6, dx + myWidth - 8, 6 );
        bufferPainter.drawLine( dx, 9, dx + myWidth - 8, 9 );
        bufferPainter.setPen( pal.color( QPalette::Active, QPalette::Light ) );
        bufferPainter.drawLine( dx + 1, 7, dx + myWidth - 7, 7 );
        bufferPainter.drawLine( dx + 1, 10, dx + myWidth - 7, 10 );
    }
    else
    {
        int dy = anchorSide == PageViewToolBar::Top ? 2 : 4;
        bufferPainter.drawLine( 6, dy, 6, dy + myHeight - 8 );
        bufferPainter.drawLine( 9, dy, 9, dy + myHeight - 8 );
        bufferPainter.setPen( pal.color( QPalette::Active, QPalette::Light ) );
        bufferPainter.drawLine( 7, dy + 1, 7, dy + myHeight - 7 );
        bufferPainter.drawLine( 10, dy + 1, 10, dy + myHeight - 7 );
    }

    // 6. reposition buttons (in rows/col grid)
    int gridX = 0,
        gridY = 0;
    QLinkedList< ToolBarButton * >::const_iterator it = buttons.begin(), end = buttons.end();
    for ( ; it != end; ++it )
    {
        ToolBarButton * button = *it;
        button->move( gridX * toolBarGridSize + xOffset,
                      gridY * toolBarGridSize + yOffset );
        button->show();
        if ( ++gridX == myCols )
        {
            gridX = 0;
            gridY++;
        }
    }

    q->setUpdatesEnabled( prevUpdates );
}

void ToolBarPrivate::reposition()
{
    // note: hiding widget here will gives better gfx, but ends drag operation
    // rebuild widget and move it to its final place
    buildToolBar();
    if ( !visible )
    {
        currentPosition = getOuterPoint();
        endPosition = getInnerPoint();
    }
    else
    {
        currentPosition = getInnerPoint();
        endPosition = getOuterPoint();
    }
    q->move( currentPosition );

    // repaint all buttons (to update background)
    QLinkedList< ToolBarButton * >::const_iterator it = buttons.begin(), end = buttons.end();
    for ( ; it != end; ++it )
        (*it)->update();
}

QPoint ToolBarPrivate::getInnerPoint() const
{
    // returns the final position of the widget
    if ( anchorSide == PageViewToolBar::Left )
        return QPoint( 0, ( anchorWidget->height() - q->height() ) / 2 );
    if ( anchorSide == PageViewToolBar::Top )
        return QPoint( ( anchorWidget->width() - q->width() ) / 2, 0 );
    if ( anchorSide == PageViewToolBar::Right )
        return QPoint( anchorWidget->width() - q->width() + toolBarRBMargin, ( anchorWidget->height() - q->height() ) / 2 );
    return QPoint( ( anchorWidget->width() - q->width()) / 2, anchorWidget->height() - q->height() + toolBarRBMargin );
}

QPoint ToolBarPrivate::getOuterPoint() const
{
    // returns the point from which the transition starts
    if ( anchorSide == PageViewToolBar::Left )
        return QPoint( -q->width(), ( anchorWidget->height() - q->height() ) / 2 );
    if ( anchorSide == PageViewToolBar::Top )
        return QPoint( ( anchorWidget->width() - q->width() ) / 2, -q->height() );
    if ( anchorSide == PageViewToolBar::Right )
        return QPoint( anchorWidget->width() + toolBarRBMargin, ( anchorWidget->height() - q->height() ) / 2 );
    return QPoint( ( anchorWidget->width() - q->width() ) / 2, anchorWidget->height() + toolBarRBMargin );
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
        {
            d->visible = false;
            deleteLater();
        }
        else
        {
            d->visible = true;
        }
    }
}

void PageViewToolBar::slotButtonClicked()
{
    ToolBarButton * button = qobject_cast<ToolBarButton *>( sender() );
    d->selectButton( button );
}

void ToolBarPrivate::selectButton( ToolBarButton * button )
{
    if ( button )
    {
        // deselect other buttons
        QLinkedList< ToolBarButton * >::const_iterator it = buttons.begin(), end = buttons.end();
        for ( ; it != end; ++it )
            if ( *it != button )
                (*it)->setChecked( false );
        // emit signal (-1 if button has been unselected)
        emit q->toolSelected( button->isChecked() ? button->buttonID() : -1 );
    }
}

#include "pageviewutils.moc"
