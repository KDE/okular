/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
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
#include <kactioncollection.h>
#include <kcolorscheme.h>
#include <kiconloader.h>
#include <KLocalizedString>

// system includes
#include <math.h>

// local includes
#include "formwidgets.h"
#include "pageview.h"
#include "videowidget.h"
#include "core/movie.h"
#include "core/page.h"
#include "core/form.h"
#include "settings.h"

/*********************/
/** PageViewItem     */
/*********************/

PageViewItem::PageViewItem( const Okular::Page * page )
    : m_page( page ), m_zoomFactor( 1.0 ), m_visible( true ),
    m_formsVisible( false ), m_crop( 0., 0., 1., 1. )
{
}

PageViewItem::~PageViewItem()
{
    qDeleteAll( m_formWidgets );
    qDeleteAll( m_videoWidgets );
}

const Okular::Page * PageViewItem::page() const
{
    return m_page;
}

int PageViewItem::pageNumber() const
{
    return m_page->number();
}

const QRect& PageViewItem::croppedGeometry() const
{
    return m_croppedGeometry;
}

int PageViewItem::croppedWidth() const
{
    return m_croppedGeometry.width();
}

int PageViewItem::croppedHeight() const
{
    return m_croppedGeometry.height();
}

const QRect& PageViewItem::uncroppedGeometry() const
{
    return m_uncroppedGeometry;
}

int PageViewItem::uncroppedWidth() const
{
    return m_uncroppedGeometry.width();
}

int PageViewItem::uncroppedHeight() const
{
    return m_uncroppedGeometry.height();
}

const Okular::NormalizedRect & PageViewItem::crop() const
{
    return m_crop;
}

double PageViewItem::zoomFactor() const
{
    return m_zoomFactor;
}

double PageViewItem::absToPageX( double absX ) const
{
    return ( absX - m_uncroppedGeometry.left() ) / m_uncroppedGeometry.width();
}

double PageViewItem::absToPageY( double absY ) const
{
    return ( absY - m_uncroppedGeometry.top() ) / m_uncroppedGeometry.height();
}

bool PageViewItem::isVisible() const
{
    return m_visible;
}

QSet<FormWidgetIface*>& PageViewItem::formWidgets()
{
    return m_formWidgets;
}

QHash< Okular::Movie *, VideoWidget* >& PageViewItem::videoWidgets()
{
    return m_videoWidgets;
}

void PageViewItem::setWHZC( int w, int h, double z, const Okular:: NormalizedRect & c )
{
    m_croppedGeometry.setWidth( w );
    m_croppedGeometry.setHeight( h );
    m_zoomFactor = z;
    m_crop = c;
    m_uncroppedGeometry.setWidth( qRound( w / ( c.right - c.left ) ) );
    m_uncroppedGeometry.setHeight( qRound( h / ( c.bottom - c.top ) ) );
    foreach(FormWidgetIface *fwi, m_formWidgets)
    {
        Okular::NormalizedRect r = fwi->rect();
        fwi->setWidthHeight(
            qRound( fabs( r.right - r.left ) * m_uncroppedGeometry.width() ),
            qRound( fabs( r.bottom - r.top ) * m_uncroppedGeometry.height() ) );
    }
    Q_FOREACH ( VideoWidget *vw, m_videoWidgets )
    {
        const Okular::NormalizedRect r = vw->normGeometry();
        vw->resize(
            qRound( fabs( r.right - r.left ) * m_uncroppedGeometry.width() ),
            qRound( fabs( r.bottom - r.top ) * m_uncroppedGeometry.height() ) );
    }
}

void PageViewItem::moveTo( int x, int y )
// Assumes setWHZC() has already been called
{
    m_croppedGeometry.moveLeft( x );
    m_croppedGeometry.moveTop( y );
    m_uncroppedGeometry.moveLeft( qRound( x - m_crop.left * m_uncroppedGeometry.width() ) );
    m_uncroppedGeometry.moveTop( qRound( y - m_crop.top * m_uncroppedGeometry.height() ) );
    QSet<FormWidgetIface*>::iterator it = m_formWidgets.begin(), itEnd = m_formWidgets.end();
    for ( ; it != itEnd; ++it )
    {
        Okular::NormalizedRect r = (*it)->rect();
        (*it)->moveTo(
            qRound( x + m_uncroppedGeometry.width() * r.left ) + 1,
            qRound( y + m_uncroppedGeometry.height() * r.top ) + 1 );
    }
    Q_FOREACH ( VideoWidget *vw, m_videoWidgets )
    {
        const Okular::NormalizedRect r = vw->normGeometry();
        vw->move(
            qRound( x + m_uncroppedGeometry.width() * r.left ) + 1,
            qRound( y + m_uncroppedGeometry.height() * r.top ) + 1 );
    }
}

void PageViewItem::setVisible( bool visible )
{
    setFormWidgetsVisible( visible && m_formsVisible );
    m_visible = visible;
}

void PageViewItem::invalidate()
{
    m_croppedGeometry.setRect( 0, 0, 0, 0 );
    m_uncroppedGeometry.setRect( 0, 0, 0, 0 );
}

bool PageViewItem::setFormWidgetsVisible( bool visible )
{
    m_formsVisible = visible;

    if ( !m_visible )
        return false;

    bool somehadfocus = false;
    QSet<FormWidgetIface*>::iterator it = m_formWidgets.begin(), itEnd = m_formWidgets.end();
    for ( ; it != itEnd; ++it )
    {
        bool hadfocus = (*it)->setVisibility( visible && (*it)->formField()->isVisible() &&
                                              FormWidgetsController::shouldFormWidgetBeShown((*it)->formField()) );
        somehadfocus = somehadfocus || hadfocus;
    }
    return somehadfocus;
}

void PageViewItem::reloadFormWidgetsState()
{
    foreach(FormWidgetIface *fwi, m_formWidgets)
    {
        fwi->setVisibility( fwi->formField()->isVisible() && FormWidgetsController::shouldFormWidgetBeShown(fwi->formField()));
    }
}

/*********************/
/** PageViewMessage  */
/*********************/

PageViewMessage::PageViewMessage( QWidget * parent )
    : QWidget( parent ), m_timer( nullptr )
    , m_lineSpacing( 0 )
{
    setObjectName( QStringLiteral( "pageViewMessage" ) );
    setFocusPolicy( Qt::NoFocus );
    QPalette pal = palette();
    pal.setColor( QPalette::Active, QPalette::Window, QApplication::palette().color( QPalette::Active, QPalette::Window ) );
    setPalette( pal );
    // if the layout is LtR, we can safely place it in the right position
    if ( layoutDirection() == Qt::LeftToRight )
        move( 10, 10 );
    resize( 0, 0 );
    hide();
}

void PageViewMessage::display( const QString & message, const QString & details, Icon icon, int durationMs )
// give Caesar what belongs to Caesar: code taken from Amarok's osd.h/.cpp
// "redde (reddite, pl.) cesari quae sunt cesaris", just btw.  :)
// The code has been heavily modified since then.
{
    if ( !Okular::Settings::showOSD() )
    {
        hide();
        return;
    }

    // set text
    m_message = message;
    m_details = details;
    // reset vars
    m_lineSpacing = 0;

    // load icon (if set)
    m_symbol = QPixmap();
    if ( icon != None )
    {
        switch ( icon )
        {
            case Annotation:
                m_symbol = SmallIcon( QStringLiteral("draw-freehand") );
                break;
            case Find:
                m_symbol = SmallIcon( QStringLiteral("zoom-original") );
                break;
            case Error:
                m_symbol = SmallIcon( QStringLiteral("dialog-error") );
                break;
            case Warning:
                m_symbol = SmallIcon( QStringLiteral("dialog-warning") );
                break;
            default:
                m_symbol = SmallIcon( QStringLiteral("dialog-information") );
                break;
        }

    }

    computeSizeAndResize();
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
            connect(m_timer, &QTimer::timeout, this, &PageViewMessage::hide);
        }
        m_timer->start( durationMs );
    } else if ( m_timer )
        m_timer->stop();

    qobject_cast<QAbstractScrollArea*>(parentWidget())->viewport()->installEventFilter(this);

}

QRect PageViewMessage::computeTextRect( const QString & message, int extra_width ) const
// Return the QRect which embeds the text
{
    int charSize = fontMetrics().averageCharWidth();
    /* width of the viewport, minus 20 (~ size removed by further resizing),
       minus the extra size (usually the icon width), minus (a bit empirical)
       twice the mean width of a character to ensure that the bounding box is
       really smaller than the container.
     */
    const int boundingWidth = qobject_cast<QAbstractScrollArea*>(parentWidget())->viewport()->width() - 20 - ( extra_width > 0 ? 2 + extra_width : 0 ) - 2*charSize;
    QRect textRect = fontMetrics().boundingRect( 0, 0, boundingWidth, 0,
                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, message );
    textRect.translate( -textRect.left(), -textRect.top() );
    textRect.adjust( 0, 0, 2, 2 );

    return textRect;
}

void PageViewMessage::computeSizeAndResize()
{
    // determine text rectangle
    const QRect textRect = computeTextRect( m_message, m_symbol.width() );
    int width = textRect.width(),
        height = textRect.height();

    if ( !m_details.isEmpty() )
    {
        // determine details text rectangle
        const QRect detailsRect = computeTextRect( m_details, m_symbol.width() );
        width = qMax( width, detailsRect.width() );
        height += detailsRect.height();

        // plus add a ~60% line spacing
        m_lineSpacing = static_cast< int >( fontMetrics().height() * 0.6 );
        height += m_lineSpacing;
    }

    // update geometry with icon information
    if ( ! m_symbol.isNull() )
    {
        width += 2 + m_symbol.width();
        height = qMax( height, m_symbol.height() );
    }

    // resize widget
    resize( QRect( 0, 0, width + 10, height + 8 ).size() );

    // if the layout is RtL, we can move it to the right place only after we
    // know how much size it will take
    if ( layoutDirection() == Qt::RightToLeft )
        move( parentWidget()->width() - geometry().width() - 10 - 1, 10 );
}


bool PageViewMessage::eventFilter(QObject * obj, QEvent * event )
{
    /* if the parent object (scroll area) resizes, the message should
       resize as well */
    if (event->type() == QEvent::Resize)
    {
        QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
        if ( resizeEvent->oldSize() != resizeEvent->size() )
        {
           computeSizeAndResize();
        }
    }
    // standard event processing
    return QObject::eventFilter(obj, event);
}

void PageViewMessage::paintEvent( QPaintEvent * /* e */ )
{
    const QRect textRect = computeTextRect( m_message, m_symbol.width() );

    QRect detailsRect;
    if ( !m_details.isEmpty() )
    {
        detailsRect = computeTextRect( m_details, m_symbol.width() );
    }

    int textXOffset = 0,
        // add 2 to account for the reduced drawRoundRect later
        textYOffset = ( geometry().height() - textRect.height() - detailsRect.height() - m_lineSpacing + 2 ) / 2,
        iconXOffset = 0,
        iconYOffset = !m_symbol.isNull() ? ( geometry().height() - m_symbol.height() ) / 2 : 0,
        shadowOffset = 1;

    if ( layoutDirection() == Qt::RightToLeft )
        iconXOffset = 2 + textRect.width();
    else
        textXOffset = 2 + m_symbol.width();

    // draw background
    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing, true );
    painter.setPen( Qt::black );
    painter.setBrush( palette().color( QPalette::Window ) );
    painter.translate( 0.5, 0.5 );
    painter.drawRoundRect( 1, 1, width()-2, height()-2, 1600 / width(), 1600 / height() );

    // draw icon if present
    if ( !m_symbol.isNull() )
        painter.drawPixmap( 5 + iconXOffset, iconYOffset, m_symbol, 0, 0, m_symbol.width(), m_symbol.height() );

    const int xStartPoint = 5 + textXOffset;
    const int yStartPoint = textYOffset;
    const int textDrawingFlags = Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap;

    // draw shadow and text
    painter.setPen( palette().color( QPalette::Window ).darker( 115 ) );
    painter.drawText( xStartPoint + shadowOffset, yStartPoint + shadowOffset, textRect.width(), textRect.height(), textDrawingFlags, m_message );
    if ( !m_details.isEmpty() )
        painter.drawText( xStartPoint + shadowOffset, yStartPoint + textRect.height() + m_lineSpacing + shadowOffset, textRect.width(), detailsRect.height(), textDrawingFlags, m_details );
    painter.setPen( palette().color( QPalette::WindowText ) );
    painter.drawText( xStartPoint, yStartPoint, textRect.width(), textRect.height(), textDrawingFlags, m_message );
    if ( !m_details.isEmpty() )
        painter.drawText( xStartPoint + shadowOffset, yStartPoint + textRect.height() + m_lineSpacing, textRect.width(), detailsRect.height(), textDrawingFlags, m_details );
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

ToolBarButton::ToolBarButton( QWidget * parent, const AnnotationToolItem &item )
    : QToolButton( parent ), m_id( item.id ), m_isText( item.isText )
{
    setCheckable( true );
    setAutoRaise( true );
    resize( buttonSize, buttonSize );
    setIconSize( QSize( iconSize, iconSize ) );
    setIcon( QIcon( item.pixmap ) );
    // set shortcut if defined
    if ( !item.shortcut.isEmpty() )
        setShortcut( QKeySequence( item.shortcut ) );
    else
        KAcceleratorManager::setNoAccel( this );

    // if accel is set display it along name
    QString accelString = shortcut().toString( QKeySequence::NativeText );
    if ( !accelString.isEmpty() )
        setToolTip( QStringLiteral("%1 [%2]").arg( item.text, accelString ) );
    else
        setToolTip( item.text );
}

void ToolBarButton::mouseDoubleClickEvent( QMouseEvent * /*event*/ )
{
  emit buttonDoubleClicked( buttonID() );
}

/* PageViewToolBar */

static const int toolBarGridSize = 40;

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

PageViewToolBar::PageViewToolBar( PageView * parent, QWidget * anchorWidget )
    : QWidget( parent ), d( new ToolBarPrivate( this ) )
{
    // initialize values of the private data storage structure
    d->anchorWidget = anchorWidget;
    d->anchorSide = Left;
    d->hiding = false;
    d->visible = false;

    // create the animation timer
    d->animTimer = new QTimer( this );
    connect( d->animTimer, &QTimer::timeout, this, &PageViewToolBar::slotAnimate );

    // apply a filter to get notified when anchor changes geometry
    d->anchorWidget->installEventFilter( this );

    setContextMenuPolicy( Qt::ActionsContextMenu );
    addAction( parent->actionCollection()->action( QStringLiteral("options_configure_annotations") ) );
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
        connect(button, &ToolBarButton::clicked, this, &PageViewToolBar::slotButtonClicked);
        connect(button, &ToolBarButton::buttonDoubleClicked, this, &PageViewToolBar::buttonDoubleClicked);
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

#ifdef OKULAR_ANIMATE_REVIEW_TOOBAR
    // start scrolling in
    d->animTimer->start( 20 );
#else
    d->currentPosition = d->endPosition;

    move( d->currentPosition );

    d->visible = true;
#endif
}

void PageViewToolBar::hideAndDestroy()
{
    // set parameters for sliding out
    d->hiding = true;
    d->endPosition = d->getOuterPoint();

#ifdef OKULAR_ANIMATE_REVIEW_TOOBAR
    // start scrolling out
    d->animTimer->start( 20 );
#else
    d->currentPosition = d->endPosition;

    move( d->currentPosition );

    d->visible = false;
    deleteLater();
#endif
}

void PageViewToolBar::selectButton( int id )
{
    ToolBarButton * button = nullptr;
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
    p.drawImage( e->rect().topLeft(), d->backgroundPixmap.toImage(), e->rect() );
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
    backgroundPixmap.fill(Qt::transparent);
    q->resize( myWidth + 1, myHeight + 1 );

    // 4. create and set transparency mask          // 4. draw background
    QPainter maskPainter( &mask);
    mask.fill( Qt::white );     
    maskPainter.setBrush( Qt::black );          
    if ( vertical )     
        maskPainter.drawRoundRect( topLeft ? -10 : 0, 0, myWidth + 11, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );      
    else        
        maskPainter.drawRoundRect( 0, topLeft ? -10 : 0, myWidth, myHeight + 11, 2000 / myWidth, 2000 / (myHeight + 10) );      
    maskPainter.end();          
    q->setMask( mask );

    // 5. draw background
    QPainter bufferPainter( &backgroundPixmap );
    bufferPainter.translate( 0.5, 0.5 );
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
    bufferPainter.setBrush( QBrush( grad ) );
    // 5.2. draw rounded border
    bufferPainter.setPen( pal.color( QPalette::Active, QPalette::Dark ).lighter( 140 ) );
    bufferPainter.setRenderHints( QPainter::Antialiasing );
    if ( vertical )
        bufferPainter.drawRoundRect( topLeft ? -10 : 0, 0, myWidth + 10, myHeight, 2000 / (myWidth + 10), 2000 / myHeight );
    else
        bufferPainter.drawRoundRect( 0, topLeft ? -10 : 0, myWidth, myHeight + 10, 2000 / myWidth, 2000 / (myHeight + 10) );
    // 5.3. draw handle
    bufferPainter.translate( -0.5, -0.5 );
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
    bufferPainter.end();

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
    QPoint newPos;
    switch ( anchorSide )
    {
        case PageViewToolBar::Left:
            newPos = QPoint( 0, ( anchorWidget->height() - q->height() ) / 2 );
            break;
        case PageViewToolBar::Top:
            newPos = QPoint( ( anchorWidget->width() - q->width() ) / 2, 0 );
            break;
        case PageViewToolBar::Right:
            newPos = QPoint( anchorWidget->width() - q->width(), ( anchorWidget->height() - q->height() ) / 2 );
            break;
        case PageViewToolBar::Bottom:
            newPos = QPoint( ( anchorWidget->width() - q->width()) / 2, anchorWidget->height() - q->height() );
            break;
    }
    return newPos + anchorWidget->pos();
}

QPoint ToolBarPrivate::getOuterPoint() const
{
    // returns the point from which the transition starts
    QPoint newPos;
    switch ( anchorSide )
    {
        case PageViewToolBar::Left:
            newPos = QPoint( -q->width(), ( anchorWidget->height() - q->height() ) / 2 );
            break;
        case PageViewToolBar::Top:
            newPos = QPoint( ( anchorWidget->width() - q->width() ) / 2, -q->height() );
            break;
        case PageViewToolBar::Right:
            newPos = QPoint( anchorWidget->width(), ( anchorWidget->height() - q->height() ) / 2 );
            break;
        case PageViewToolBar::Bottom:
            newPos = QPoint( ( anchorWidget->width() - q->width() ) / 2, anchorWidget->height() );
            break;
    }
    return newPos + anchorWidget->pos();
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

void PageViewToolBar::setToolsEnabled( bool on )
{
    QLinkedList< ToolBarButton * >::const_iterator it = d->buttons.begin(), end = d->buttons.end();
    for ( ; it != end; ++it )
        (*it)->setEnabled( on );
}

void PageViewToolBar::setTextToolsEnabled( bool on )
{
    QLinkedList< ToolBarButton * >::const_iterator it = d->buttons.begin(), end = d->buttons.end();
    for ( ; it != end; ++it )
        if ( (*it)->isText() )
            (*it)->setEnabled( on );
}

#include "moc_pageviewutils.cpp"
