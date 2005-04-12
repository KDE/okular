/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qtimer.h>
#include <qimage.h>
#include <qpainter.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#include <kapplication.h>
#include <kcursor.h>
#include <ktoolbar.h>
#include <kdebug.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kimageeffect.h>
#include <kwin.h>

// system includes
#include <stdlib.h>
#include <math.h>

// local includes
#include "presentationwidget.h"
#include "pagepainter.h"
#include "core/generator.h"
#include "core/page.h"
#include "conf/settings.h"


// comment this to disable the top-right progress indicator
#define ENABLE_PROGRESS_OVERLAY


// a frame contains a pointer to the page object, its geometry and the
// transition effect to the next frame
struct PresentationFrame
{
    const KPDFPage * page;
    QRect geometry;
};


PresentationWidget::PresentationWidget( QWidget * parent, KPDFDocument * doc )
    : QDialog( parent, "presentationWidget", true, WDestructiveClose | WStyle_NoBorder), m_document( doc ), m_frameIndex( -1 )
{
    // set look and geometry
    setBackgroundMode( Qt::NoBackground );

    m_width = -1;

    // show widget and take control
    showFullScreen();

    // misc stuff
    setMouseTracking( true );
    m_transitionTimer = new QTimer( this );
    connect( m_transitionTimer, SIGNAL( timeout() ), this, SLOT( slotTransitionStep() ) );
    m_overlayHideTimer = new QTimer( this );
    connect( m_overlayHideTimer, SIGNAL( timeout() ), this, SLOT( slotHideOverlay() ) );

    // handle cursor appearance as specified in configuration
    if ( Settings::slidesCursor() == Settings::EnumSlidesCursor::HiddenDelay )
    {
        KCursor::setAutoHideCursor( this, true );
        KCursor::setHideCursorDelay( 3000 );
    }
    else if ( Settings::slidesCursor() == Settings::EnumSlidesCursor::Hidden )
    {
        setCursor( KCursor::blankCursor() );
    }
}

PresentationWidget::~PresentationWidget()
{
    // remove this widget from document observer
    m_document->removeObserver( this );

    // set a new viewport in document if page number differs
    if ( m_frameIndex != -1 && m_frameIndex != m_document->viewport().pageNumber )
        m_document->setViewportPage( m_frameIndex/*, PRESENTATION_ID*/ );

    // delete frames
    QValueVector< PresentationFrame * >::iterator fIt = m_frames.begin(), fEnd = m_frames.end();
    for ( ; fIt != fEnd; ++fIt )
        delete *fIt;
}


void PresentationWidget::notifySetup( const QValueVector< KPDFPage * > & pageSet, bool /*documentChanged*/ )
{
    // delete previous frames (if any (shouldn't be))
    QValueVector< PresentationFrame * >::iterator fIt = m_frames.begin(), fEnd = m_frames.end();
    for ( ; fIt != fEnd; ++fIt )
        delete *fIt;
    if ( !m_frames.isEmpty() )
        kdWarning() << "Frames setup changed while a Presentation is in progress." << endl;
    m_frames.clear();

    // create the new frames
    QValueVector< KPDFPage * >::const_iterator setIt = pageSet.begin(), setEnd = pageSet.end();
    float screenRatio = (float)m_height / (float)m_width;
    for ( ; setIt != setEnd; ++setIt )
    {
        PresentationFrame * frame = new PresentationFrame();
        frame->page = *setIt;
        // calculate frame geometry keeping constant aspect ratio
        float pageRatio = frame->page->ratio();
        int pageWidth = m_width,
            pageHeight = m_height;
        if ( pageRatio > screenRatio )
            pageWidth = (int)( (float)pageHeight / pageRatio );
        else
            pageHeight = (int)( (float)pageWidth * pageRatio );
        frame->geometry.setRect( (m_width - pageWidth) / 2,
                                 (m_height - pageHeight) / 2,
                                 pageWidth, pageHeight );
        // add the frame to the vector
        m_frames.push_back( frame );
    }

    // get metadata from the document
    m_metaStrings.clear();
    const DocumentInfo * info = m_document->documentInfo();
    if ( info )
    {
        if ( !info->get( "title" ).isNull() )
            m_metaStrings += i18n( "Title: %1" ).arg( info->get( "title" ) );
        if ( !info->get( "author" ).isNull() )
            m_metaStrings += i18n( "Author: %1" ).arg( info->get( "author" ) );
    }
    m_metaStrings += i18n( "Pages: %1" ).arg( m_document->pages() );
    m_metaStrings += i18n( "Click to begin" );
}

void PresentationWidget::notifyViewportChanged( bool /*smoothMove*/ )
{
    // discard notifications if displaying the summary
    if ( m_frameIndex == -1 && Settings::slidesShowSummary() )
        return;

    // display the current page
    changePage( m_document->viewport().pageNumber );

    // auto advance to the next page if set
    if ( Settings::slidesAdvance() )
        QTimer::singleShot( Settings::slidesAdvanceTime() * 1000, this, SLOT( slotNextPage() ) );
}

void PresentationWidget::notifyPageChanged( int pageNumber, int changedFlags )
{
    // check if it's the last requested pixmap. if so update the widget.
    if ( (changedFlags & DocumentObserver::Pixmap) && pageNumber == m_frameIndex )
        generatePage();
}

bool PresentationWidget::canUnloadPixmap( int pageNumber )
{
    // can unload all pixmaps except for the currently visible one
    return pageNumber != m_frameIndex;
}


// <widget events>
bool PresentationWidget::event ( QEvent * e )
{
    if (e -> type() == QEvent::WindowDeactivate) KWin::clearState(winId(), NET::StaysOnTop);
    else if (e -> type() == QEvent::WindowActivate) KWin::setState(winId(), NET::StaysOnTop);
    return QDialog::event(e);
}

void PresentationWidget::keyPressEvent( QKeyEvent * e )
{
    if (m_width == -1) return;
	
    if ( e->key() == Key_Left || e->key() == Key_Backspace || e->key() == Key_Prior )
        slotPrevPage();
    else if ( e->key() == Key_Right || e->key() == Key_Space || e->key() == Key_Next )
        slotNextPage();
    else if ( e->key() == Key_Home )
        slotFirstPage();
    else if ( e->key() == Key_End )
        slotLastPage();
    else if ( e->key() == Key_Escape )
    {
        if ( m_topBar->isShown() )
            m_topBar->hide();
        else
            close();
    }
}

void PresentationWidget::wheelEvent( QWheelEvent * e )
{
    // performance note: don't remove the clipping
    int div = e->delta() / 120;
    if ( div > 0 )
    {
        if ( div > 3 )
            div = 3;
        while ( div-- )
            slotPrevPage();
    }
    else if ( div < 0 )
    {
        if ( div < -3 )
            div = -3;
        while ( div++ )
            slotNextPage();
    }
}

void PresentationWidget::mousePressEvent( QMouseEvent * e )
{
    // pressing left button
    if ( e->button() == Qt::LeftButton )
    {
        if ( m_overlayGeometry.contains( e->pos() ) )
            overlayClick( e->pos() );
        else
            slotNextPage();
    }
    // pressing right button
    else if ( e->button() == Qt::RightButton )
        slotPrevPage();
}

void PresentationWidget::mouseMoveEvent( QMouseEvent * e )
{
    if (m_width == -1) return;
	
    // hide a shown bar when exiting the area
    if ( m_topBar->isShown() )
    {
        if ( e->y() > ( m_topBar->height() + 1 ) )
            m_topBar->hide();
    }
    // show a hidden bar if mouse reaches the top of the screen
    else if ( !e->y() )
        m_topBar->show();
    // change page if dragging the mouse over the 'wheel'
    else if ( e->state() == Qt::LeftButton && m_overlayGeometry.contains( e->pos() ) )
            overlayClick( e->pos() );
}

void PresentationWidget::paintEvent( QPaintEvent * pe )
{
    if (m_width == -1)
    {
        QRect d = KGlobalSettings::desktopGeometry(this);
        m_width = d.width();
        m_height = d.height();

        // create top toolbar
        m_topBar = new KToolBar( this, "presentationBar" );
        m_topBar->setIconSize( 32 );
        m_topBar->setMovingEnabled( false );
        m_topBar->insertButton( "1leftarrow", 2, SIGNAL( clicked() ), this, SLOT( slotPrevPage() ) );
        m_topBar->insertButton( "1rightarrow", 3, SIGNAL( clicked() ), this, SLOT( slotNextPage() ) );
        m_topBar->insertButton( "exit", 1, SIGNAL( clicked() ), this, SLOT( close() ) );
        m_topBar->setGeometry( 0, 0, m_width, 32 + 10 );
        m_topBar->alignItemRight( 1 );
        m_topBar->hide();
        // change topbar background color
        QPalette p = m_topBar->palette();
        p.setColor( QPalette::Active, QColorGroup::Button, Qt::gray );
        p.setColor( QPalette::Active, QColorGroup::Background, Qt::darkGray );
        m_topBar->setPalette( p );

        // register this observer in document. events will come immediately
        m_document->addObserver( this );

        // show summary if requested
        if ( Settings::slidesShowSummary() )
            generatePage();
    }

    // check painting rect consistancy
    QRect r = pe->rect().intersect( geometry() );
    if ( r.isNull() || m_lastRenderedPixmap.isNull() )
        return;

    // blit the pixmap to the screen
    QMemArray<QRect> allRects = pe->region().rects();
    uint numRects = allRects.count();
    for ( uint i = 0; i < numRects; i++ )
    {
        const QRect & r = allRects[i];
        if ( !r.isValid() )
            continue;
#ifdef ENABLE_PROGRESS_OVERLAY
        if ( Settings::slidesShowProgress() && r.intersects( m_overlayGeometry ) )
        {
            // backbuffer the overlay operation
            QPixmap backPixmap( r.size() );
            QPainter pixPainter( &backPixmap );

            // first draw the background on the backbuffer
            pixPainter.drawPixmap( QPoint(0,0), m_lastRenderedPixmap, r );

            // then blend the overlay (a piece of) over the background
            QRect ovr = m_overlayGeometry.intersect( r );
            pixPainter.drawPixmap( ovr.left() - r.left(), ovr.top() - r.top(),
                m_lastRenderedOverlay, ovr.left() - m_overlayGeometry.left(),
                ovr.top() - m_overlayGeometry.top(), ovr.width(), ovr.height() );

            // finally blit the pixmap to the screen
            pixPainter.end();
            bitBlt( this, r.topLeft(), &backPixmap, backPixmap.rect() );
        } else
#endif
        // copy the rendered pixmap to the screen
        bitBlt( this, r.topLeft(), &m_lastRenderedPixmap, r );
    }
}
// </widget events>


void PresentationWidget::overlayClick( const QPoint & position )
{
    // clicking the progress indicator
    int xPos = position.x() - m_overlayGeometry.x() - m_overlayGeometry.width() / 2,
        yPos = m_overlayGeometry.height() / 2 - position.y();
    if ( !xPos && !yPos )
        return;

    // compute angle relative to indicator (note coord transformation)
    float angle = 0.5 + 0.5 * atan2( -xPos, -yPos ) / M_PI;
    int pageIndex = (int)( angle * ( m_frames.count() - 1 ) + 0.5 );

    // go to selected page
    changePage( pageIndex );
}

void PresentationWidget::changePage( int newPage )
{
    if ( m_frameIndex == newPage )
        return;

    // check if pixmap exists or else request it
    m_frameIndex = newPage;
    PresentationFrame * frame = m_frames[ m_frameIndex ];
    int pixW = frame->geometry.width();
    int pixH = frame->geometry.height();

    // if pixmap not inside the KPDFPage we request it and wait for
    // notifyPixmapChanged call or else we can proceed to pixmap generation
    if ( !frame->page->hasPixmap( PRESENTATION_ID, pixW, pixH ) )
    {
        QValueList< PixmapRequest * > request;
        request.push_back( new PixmapRequest( PRESENTATION_ID, m_frameIndex, pixW, pixH, PRESENTATION_PRIO ) );
        m_document->requestPixmaps( request );
    }
    else
        generatePage();
}

void PresentationWidget::generatePage()
{
    if ( m_lastRenderedPixmap.isNull() )
        m_lastRenderedPixmap.resize( m_width, m_height );

    // opens the painter over the pixmap
    QPainter pixmapPainter;
    pixmapPainter.begin( &m_lastRenderedPixmap );
    // generate welcome page
    if ( m_frameIndex == -1 )
        generateIntroPage( pixmapPainter );
    // generate a normal pixmap with extended margin filling
    if ( m_frameIndex >= 0 && m_frameIndex < (int)m_document->pages() )
        generateContentsPage( m_frameIndex, pixmapPainter );
    pixmapPainter.end();

    // generate the top-right corner overlay
#ifdef ENABLE_PROGRESS_OVERLAY
    if ( Settings::slidesShowProgress() && m_frameIndex != -1 )
        generateOverlay();
#endif

    // start transition on pages that have one
    const KPDFPageTransition * transition = m_frameIndex != -1 ?
        m_frames[ m_frameIndex ]->page->getTransition() : 0;
    if ( transition )
        initTransition( transition );
    else {
        KPDFPageTransition trans = defaultTransition();
        initTransition( &trans );
    }
}

void PresentationWidget::generateIntroPage( QPainter & p )
{
    // use a vertical gray gradient background
    int blend1 = m_height / 10,
        blend2 = 9 * m_height / 10;
    int baseTint = Qt::gray.red();
    for ( int i = 0; i < m_height; i++ )
    {
        int k = baseTint;
        if ( i < blend1 )
            k -= (int)( baseTint * (i-blend1)*(i-blend1) / (float)(blend1 * blend1) );
        if ( i > blend2 )
            k += (int)( (255-baseTint) * (i-blend2)*(i-blend2) / (float)(blend1 * blend1) );
        p.fillRect( 0, i, m_width, 1, QColor( k, k, k ) );
    }

    // draw kpdf logo in the four corners
    QPixmap logo = DesktopIcon( "kpdf", 64 );
    if ( !logo.isNull() )
    {
        p.drawPixmap( 5, 5, logo );
        p.drawPixmap( m_width - 5 - logo.width(), 5, logo );
        p.drawPixmap( m_width - 5 - logo.width(), m_height - 5 - logo.height(), logo );
        p.drawPixmap( 5, m_height - 5 - logo.height(), logo );
    }

    // draw metadata text (the last line is 'click to begin')
    int strNum = m_metaStrings.count(),
        strHeight = m_height / ( strNum + 4 ),
        fontHeight = 2 * strHeight / 3;
    QFont font( p.font() );
    font.setPixelSize( fontHeight );
    QFontMetrics metrics( font );
    for ( int i = 0; i < strNum; i++ )
    {
        // set a font to fit text width
        float wScale = (float)metrics.boundingRect( m_metaStrings[i] ).width() / (float)m_width;
        QFont f( font );
        if ( wScale > 1.0 )
            f.setPixelSize( (int)( (float)fontHeight / (float)wScale ) );
        p.setFont( f );

        // text shadow
        p.setPen( Qt::darkGray );
        p.drawText( 2, m_height / 4 + strHeight * i + 2, m_width, strHeight,
                    AlignHCenter | AlignVCenter, m_metaStrings[i] );
        // text body
        p.setPen( 128 + (127 * i) / strNum );
        p.drawText( 0, m_height / 4 + strHeight * i, m_width, strHeight,
                    AlignHCenter | AlignVCenter, m_metaStrings[i] );
    }
}

void PresentationWidget::generateContentsPage( int pageNum, QPainter & p )
{
    PresentationFrame * frame = m_frames[ pageNum ];

    // translate painter and contents rect
    QRect geom( frame->geometry );
    p.translate( geom.left(), geom.top() );
    geom.moveBy( -geom.left(), -geom.top() );

    // draw the page using the shared PagePainter class
    int flags = PagePainter::Accessibility;
    PagePainter::paintPageOnPainter( frame->page, PRESENTATION_ID, flags,
                                     &p, geom, geom.width(), geom.height() );

    // restore painter
    p.translate( -frame->geometry.left(), -frame->geometry.top() );

    // fill unpainted areas with background color
    QRegion unpainted( QRect( 0, 0, m_width, m_height ) );
    QMemArray<QRect> rects = unpainted.subtract( frame->geometry ).rects();
    for ( uint i = 0; i < rects.count(); i++ )
    {
        const QRect & r = rects[i];
        p.fillRect( r, Settings::slidesBackgroundColor() );
    }
}

void PresentationWidget::generateOverlay()
{
#ifdef ENABLE_PROGRESS_OVERLAY
    // calculate overlay geometry and resize pixmap if needed
    int side = m_width / 16;
    m_overlayGeometry.setRect( m_width - side, 0, side, side );
    if ( m_lastRenderedOverlay.width() != side )
        m_lastRenderedOverlay.resize( side, side );

    // note: to get a sort of antialiasing, we render the pixmap double sized
    // and the resulting image is smoothly scaled down. So here we open a
    // painter on the double sized pixmap.
    side *= 2;
    QPixmap doublePixmap( side, side );
    doublePixmap.fill( Qt::black );
    QPainter pixmapPainter( &doublePixmap );

    // draw PIE SLICES in blue levels (the levels will then be the alpha component)
    int pages = m_document->pages();
    if ( pages > 36 )
    {   // draw continuous slices
        int degrees = (int)( 360 * (float)(m_frameIndex + 1) / (float)pages );
        pixmapPainter.setPen( 0x20 );
        pixmapPainter.setBrush( 0x10 );
        pixmapPainter.drawPie( 2, 2, side - 4, side - 4, 90*16, (360-degrees)*16 );
        pixmapPainter.setBrush( 0xC0 );
        pixmapPainter.drawPie( 2, 2, side - 4, side - 4, 90*16, -degrees*16 );
    }
    else
    {   // draw discrete slices
        float oldCoord = -90;
        for ( int i = 0; i < pages; i++ )
        {
            float newCoord = -90 + 360 * (float)(i + 1) / (float)pages;
            pixmapPainter.setPen( i <= m_frameIndex ? 0x40 : 0x05 );
            pixmapPainter.setBrush( i <= m_frameIndex ? 0xC0 : 0x10 );
            pixmapPainter.drawPie( 2, 2, side - 4, side - 4,
                                   (int)( -16*(oldCoord + 1) ), (int)( -16*(newCoord - (oldCoord + 2)) ) );
            oldCoord = newCoord;
        }
    }
    int circleOut = side / 4;
    pixmapPainter.setPen( Qt::black );
    pixmapPainter.setBrush( Qt::black );
    pixmapPainter.drawEllipse( circleOut, circleOut, side - 2*circleOut, side - 2*circleOut );

    // draw TEXT using maximum opacity
    QFont f( pixmapPainter.font() );
    f.setPixelSize( side / 4 );
    pixmapPainter.setFont( f );
    pixmapPainter.setPen( 0xFF );
    // use a little offset to prettify output
    pixmapPainter.drawText( 2, 2, side, side, Qt::AlignCenter, QString::number( m_frameIndex + 1 ) );

    // end drawing pixmap and halve image
    pixmapPainter.end();
    side /= 2;
    QImage image( doublePixmap.convertToImage().smoothScale( side, side ) );
    image.setAlphaBuffer( true );

    // generate a monochrome pixmap using grey level as alpha channel and
    // a saturated hilight color as base color
    int hue, sat, val;
    palette().active().highlight().getHsv( &hue, &sat, &val );
    sat = (sat + 255) / 2;
    const QColor & color = QColor( hue, sat, val, QColor::Hsv );
    int red = color.red(), green = color.green(), blue = color.blue(),
        pixels = image.width() * image.height();
    unsigned int * data = (unsigned int *)image.bits();
    for( int i = 0; i < pixels; ++i )
        data[i] = qRgba( red, green, blue, data[i] & 0xFF );
    m_lastRenderedOverlay.convertFromImage( image );

    // start the autohide timer
    repaint( m_overlayGeometry, false /*clear*/ ); // toggle with next line
    //update( m_overlayGeometry );
    m_overlayHideTimer->start( 2500, true );
#endif
}



void PresentationWidget::slotNextPage()
{
    // loop when configured
    if ( m_frameIndex == (int)m_frames.count() - 1 && Settings::slidesLoop() )
        m_frameIndex = -1;

    if ( m_frameIndex < (int)m_frames.count() - 1 )
    {
        // go to next page
        changePage( m_frameIndex + 1 );
    }
    else if ( m_transitionTimer->isActive() )
    {
        m_transitionTimer->stop();
        update();
    }

    // we need the setFocus() call here to let KCursor::autoHide() work correctly
    setFocus();

    // auto advance to the next page if set
    if ( Settings::slidesAdvance() )
        QTimer::singleShot( Settings::slidesAdvanceTime() * 1000, this, SLOT( slotNextPage() ) );
}

void PresentationWidget::slotPrevPage()
{
    if ( m_frameIndex > 0 )
    {
        // go to previous page
        changePage( m_frameIndex - 1 );
    }
    else if ( m_transitionTimer->isActive() )
    {
        m_transitionTimer->stop();
        update();
    }
}

void PresentationWidget::slotFirstPage()
{
  changePage( 0 );
}

void PresentationWidget::slotLastPage()
{
  changePage( (int)m_frames.count() - 1 );
}

void PresentationWidget::slotHideOverlay()
{
    QRect geom( m_overlayGeometry );
    m_overlayGeometry.setCoords( 0, 0, -1, -1 );
    update( geom );
}

void PresentationWidget::slotTransitionStep()
{
    if ( m_transitionRects.empty() )
    {
        // it's better to fix the transition to cover the whole screen than
        // enabling the following line that wastes cpu for nothing
        //update();
        return;
    }

    for ( int i = 0; i < m_transitionMul && !m_transitionRects.empty(); i++ )
    {
        update( m_transitionRects.first() );
        m_transitionRects.pop_front();
    }
    m_transitionTimer->start( m_transitionDelay, true );
}

const KPDFPageTransition PresentationWidget::defaultTransition() const
{
    return defaultTransition( Settings::slidesTransition() );
}

const KPDFPageTransition PresentationWidget::defaultTransition( int type ) const
{
    switch ( type )
    {
        case Settings::EnumSlidesTransition::BlindsHorizontal:
        {
            KPDFPageTransition transition( KPDFPageTransition::Blinds );
            transition.setAlignment( KPDFPageTransition::Horizontal );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::BlindsVertical:
        {
            KPDFPageTransition transition( KPDFPageTransition::Blinds );
            transition.setAlignment( KPDFPageTransition::Vertical );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::BoxIn:
        {
            KPDFPageTransition transition( KPDFPageTransition::Box );
            transition.setDirection( KPDFPageTransition::Inward );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::BoxOut:
        {
            KPDFPageTransition transition( KPDFPageTransition::Box );
            transition.setDirection( KPDFPageTransition::Outward );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::Dissolve:
        {
            return KPDFPageTransition( KPDFPageTransition::Dissolve );
            break;
        }
        case Settings::EnumSlidesTransition::GlitterDown:
        {
            KPDFPageTransition transition( KPDFPageTransition::Glitter );
            transition.setAngle( 270 );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::GlitterRight:
        {
            KPDFPageTransition transition( KPDFPageTransition::Glitter );
            transition.setAngle( 0 );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::GlitterRightDown:
        {
            KPDFPageTransition transition( KPDFPageTransition::Glitter );
            transition.setAngle( 315 );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::Random:
        {
            return defaultTransition( KApplication::random() % 18 );
            break;
        }
        case Settings::EnumSlidesTransition::SplitHorizontalIn:
        {
            KPDFPageTransition transition( KPDFPageTransition::Split );
            transition.setAlignment( KPDFPageTransition::Horizontal );
            transition.setDirection( KPDFPageTransition::Inward );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::SplitHorizontalOut:
        {
            KPDFPageTransition transition( KPDFPageTransition::Split );
            transition.setAlignment( KPDFPageTransition::Horizontal );
            transition.setDirection( KPDFPageTransition::Outward );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::SplitVerticalIn:
        {
            KPDFPageTransition transition( KPDFPageTransition::Split );
            transition.setAlignment( KPDFPageTransition::Vertical );
            transition.setDirection( KPDFPageTransition::Inward );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::SplitVerticalOut:
        {
            KPDFPageTransition transition( KPDFPageTransition::Split );
            transition.setAlignment( KPDFPageTransition::Vertical );
            transition.setDirection( KPDFPageTransition::Outward );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::WipeDown:
        {
            KPDFPageTransition transition( KPDFPageTransition::Wipe );
            transition.setAngle( 270 );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::WipeRight:
        {
            KPDFPageTransition transition( KPDFPageTransition::Wipe );
            transition.setAngle( 0 );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::WipeLeft:
        {
            KPDFPageTransition transition( KPDFPageTransition::Wipe );
            transition.setAngle( 180 );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::WipeUp:
        {
            KPDFPageTransition transition( KPDFPageTransition::Wipe );
            transition.setAngle( 90 );
            return transition;
            break;
        }
        case Settings::EnumSlidesTransition::Replace:
        default:
            return KPDFPageTransition( KPDFPageTransition::Replace );
            break;
    }
}

/** ONLY the TRANSITIONS GENERATION function from here on **/
void PresentationWidget::initTransition( const KPDFPageTransition *transition )
{
    // if it's just a 'replace' transition, repaint the screen
    if ( transition->type() == KPDFPageTransition::Replace )
    {
        update();
        return;
    }

    const bool isInward = transition->direction() == KPDFPageTransition::Inward;
    const bool isHorizontal = transition->alignment() == KPDFPageTransition::Horizontal;
    const float totalTime = transition->duration();

    m_transitionRects.clear();

    switch( transition->type() )
    {
            // split: horizontal / vertical and inward / outward
        case KPDFPageTransition::Split:
        {
            const int steps = isHorizontal ? 100 : 75;
            if ( isHorizontal )
            {
                if ( isInward )
                {
                    int xPosition = 0;
                    for ( int i = 0; i < steps; i++ )
                    {
                        int xNext = ((i + 1) * m_width) / (2 * steps);
                        m_transitionRects.push_back( QRect( xPosition, 0, xNext - xPosition, m_height ) );
                        m_transitionRects.push_back( QRect( m_width - xNext, 0, xNext - xPosition, m_height ) );
                        xPosition = xNext;
                    }
                }
                else
                {
                    int xPosition = m_width / 2;
                    for ( int i = 0; i < steps; i++ )
                    {
                        int xNext = ((steps - (i + 1)) * m_width) / (2 * steps);
                        m_transitionRects.push_back( QRect( xNext, 0, xPosition - xNext, m_height ) );
                        m_transitionRects.push_back( QRect( m_width - xPosition, 0, xPosition - xNext, m_height ) );
                        xPosition = xNext;
                    }
                }
            }
            else
            {
                if ( isInward )
                {
                    int yPosition = 0;
                    for ( int i = 0; i < steps; i++ )
                    {
                        int yNext = ((i + 1) * m_height) / (2 * steps);
                        m_transitionRects.push_back( QRect( 0, yPosition, m_width, yNext - yPosition ) );
                        m_transitionRects.push_back( QRect( 0, m_height - yNext, m_width, yNext - yPosition ) );
                        yPosition = yNext;
                    }
                }
                else
                {
                    int yPosition = m_height / 2;
                    for ( int i = 0; i < steps; i++ )
                    {
                        int yNext = ((steps - (i + 1)) * m_height) / (2 * steps);
                        m_transitionRects.push_back( QRect( 0, yNext, m_width, yPosition - yNext ) );
                        m_transitionRects.push_back( QRect( 0, m_height - yPosition, m_width, yPosition - yNext ) );
                        yPosition = yNext;
                    }
                }
            }
            m_transitionMul = 2;
            m_transitionDelay = (int)( (totalTime * 1000) / steps );
        } break;

            // blinds: horizontal(l-to-r) / vertical(t-to-b)
        case KPDFPageTransition::Blinds:
        {
            const int blinds = isHorizontal ? 8 : 6;
            const int steps = m_width / (4 * blinds);
            if ( isHorizontal )
            {
                int xPosition[ 8 ];
                for ( int b = 0; b < blinds; b++ )
                    xPosition[ b ] = (b * m_width) / blinds;

                for ( int i = 0; i < steps; i++ )
                {
                    int stepOffset = (int)( ((float)i * (float)m_width) / ((float)blinds * (float)steps) );
                    for ( int b = 0; b < blinds; b++ )
                    {
                        m_transitionRects.push_back( QRect( xPosition[ b ], 0, stepOffset, m_height ) );
                        xPosition[ b ] = stepOffset + (b * m_width) / blinds;
                    }
                }
            }
            else
            {
                int yPosition[ 6 ];
                for ( int b = 0; b < blinds; b++ )
                    yPosition[ b ] = (b * m_height) / blinds;

                for ( int i = 0; i < steps; i++ )
                {
                    int stepOffset = (int)( ((float)i * (float)m_height) / ((float)blinds * (float)steps) );
                    for ( int b = 0; b < blinds; b++ )
                    {
                        m_transitionRects.push_back( QRect( 0, yPosition[ b ], m_width, stepOffset ) );
                        yPosition[ b ] = stepOffset + (b * m_height) / blinds;
                    }
                }
            }
            m_transitionMul = blinds;
            m_transitionDelay = (int)( (totalTime * 1000) / steps );
        } break;

            // box: inward / outward
        case KPDFPageTransition::Box:
        {
            const int steps = m_width / 10;
            if ( isInward )
            {
                int L = 0, T = 0, R = m_width, B = m_height;
                for ( int i = 0; i < steps; i++ )
                {
                    // compure shrinked box coords
                    int newL = ((i + 1) * m_width) / (2 * steps);
                    int newT = ((i + 1) * m_height) / (2 * steps);
                    int newR = m_width - newL;
                    int newB = m_height - newT;
                    // add left, right, topcenter, bottomcenter rects
                    m_transitionRects.push_back( QRect( L, T, newL - L, B - T ) );
                    m_transitionRects.push_back( QRect( newR, T, R - newR, B - T ) );
                    m_transitionRects.push_back( QRect( newL, T, newR - newL, newT - T ) );
                    m_transitionRects.push_back( QRect( newL, newB, newR - newL, B - newB ) );
                    L = newL; T = newT; R = newR, B = newB;
                }
            }
            else
            {
                int L = m_width / 2, T = m_height / 2, R = L, B = T;
                for ( int i = 0; i < steps; i++ )
                {
                    // compure shrinked box coords
                    int newL = ((steps - (i + 1)) * m_width) / (2 * steps);
                    int newT = ((steps - (i + 1)) * m_height) / (2 * steps);
                    int newR = m_width - newL;
                    int newB = m_height - newT;
                    // add left, right, topcenter, bottomcenter rects
                    m_transitionRects.push_back( QRect( newL, newT, L - newL, newB - newT ) );
                    m_transitionRects.push_back( QRect( R, newT, newR - R, newB - newT ) );
                    m_transitionRects.push_back( QRect( L, newT, R - L, T - newT ) );
                    m_transitionRects.push_back( QRect( L, B, R - L, newB - B ) );
                    L = newL; T = newT; R = newR, B = newB;
                }
            }
            m_transitionMul = 4;
            m_transitionDelay = (int)( (totalTime * 1000) / steps );
        } break;

            // wipe: implemented for 4 canonical angles
        case KPDFPageTransition::Wipe:
        {
            const int angle = transition->angle();
            const int steps = (angle == 0) || (angle == 180) ? m_width / 8 : m_height / 8;
            if ( angle == 0 )
            {
                int xPosition = 0;
                for ( int i = 0; i < steps; i++ )
                {
                    int xNext = ((i + 1) * m_width) / steps;
                    m_transitionRects.push_back( QRect( xPosition, 0, xNext - xPosition, m_height ) );
                    xPosition = xNext;
                }
            }
            else if ( angle == 90 )
            {
                int yPosition = m_height;
                for ( int i = 0; i < steps; i++ )
                {
                    int yNext = ((steps - (i + 1)) * m_height) / steps;
                    m_transitionRects.push_back( QRect( 0, yNext, m_width, yPosition - yNext ) );
                    yPosition = yNext;
                }
            }
            else if ( angle == 180 )
            {
                int xPosition = m_width;
                for ( int i = 0; i < steps; i++ )
                {
                    int xNext = ((steps - (i + 1)) * m_width) / steps;
                    m_transitionRects.push_back( QRect( xNext, 0, xPosition - xNext, m_height ) );
                    xPosition = xNext;
                }
            }
            else if ( angle == 270 )
            {
                int yPosition = 0;
                for ( int i = 0; i < steps; i++ )
                {
                    int yNext = ((i + 1) * m_height) / steps;
                    m_transitionRects.push_back( QRect( 0, yPosition, m_width, yNext - yPosition ) );
                    yPosition = yNext;
                }
            }
            else
            {
                update();
                return;
            }
            m_transitionMul = 1;
            m_transitionDelay = (int)( (totalTime * 1000) / steps );
        } break;

            // dissolve: replace 'random' rects
        case KPDFPageTransition::Dissolve:
        {
            const int gridXsteps = 50;
            const int gridYsteps = 38;
            const int steps = gridXsteps * gridYsteps;
            int oldX = 0;
            int oldY = 0;
            // create a grid of gridXstep by gridYstep QRects
            for ( int y = 0; y < gridYsteps; y++ )
            {
                int newY = (int)( m_height * ((float)(y+1) / (float)gridYsteps) );
                for ( int x = 0; x < gridXsteps; x++ )
                {
                    int newX = (int)( m_width * ((float)(x+1) / (float)gridXsteps) );
                    m_transitionRects.push_back( QRect( oldX, oldY, newX - oldX, newY - oldY ) );
                    oldX = newX;
                }
                oldX = 0;
                oldY = newY;
            }
            // randomize the grid
            for ( int i = 0; i < steps; i++ )
            {
                int n1 = (int)(steps * drand48());
                int n2 = (int)(steps * drand48());
                // swap items if index differs
                if ( n1 != n2 )
                {
                    QRect r = m_transitionRects[ n2 ];
                    m_transitionRects[ n2 ] = m_transitionRects[ n1 ];
                    m_transitionRects[ n1 ] = r;
                }
            }
            // set global transition parameters
            m_transitionMul = 40;
            m_transitionDelay = (int)( (m_transitionMul * 1000 * totalTime) / steps );
        } break;

            // glitter: similar to dissolve but has a direction
        case KPDFPageTransition::Glitter:
        {
            const int gridXsteps = 50;
            const int gridYsteps = 38;
            const int steps = gridXsteps * gridYsteps;
            const int angle = transition->angle();
            // generate boxes using a given direction
            if ( angle == 90 )
            {
                int yPosition = m_height;
                for ( int i = 0; i < gridYsteps; i++ )
                {
                    int yNext = ((gridYsteps - (i + 1)) * m_height) / gridYsteps;
                    int xPosition = 0;
                    for ( int j = 0; j < gridXsteps; j++ )
                    {
                        int xNext = ((j + 1) * m_width) / gridXsteps;
                        m_transitionRects.push_back( QRect( xPosition, yNext, xNext - xPosition, yPosition - yNext ) );
                        xPosition = xNext;
                    }
                    yPosition = yNext;
                }
            }
            else if ( angle == 180 )
            {
                int xPosition = m_width;
                for ( int i = 0; i < gridXsteps; i++ )
                {
                    int xNext = ((gridXsteps - (i + 1)) * m_width) / gridXsteps;
                    int yPosition = 0;
                    for ( int j = 0; j < gridYsteps; j++ )
                    {
                        int yNext = ((j + 1) * m_height) / gridYsteps;
                        m_transitionRects.push_back( QRect( xNext, yPosition, xPosition - xNext, yNext - yPosition ) );
                        yPosition = yNext;
                    }
                    xPosition = xNext;
                }
            }
            else if ( angle == 270 )
            {
                int yPosition = 0;
                for ( int i = 0; i < gridYsteps; i++ )
                {
                    int yNext = ((i + 1) * m_height) / gridYsteps;
                    int xPosition = 0;
                    for ( int j = 0; j < gridXsteps; j++ )
                    {
                        int xNext = ((j + 1) * m_width) / gridXsteps;
                        m_transitionRects.push_back( QRect( xPosition, yPosition, xNext - xPosition, yNext - yPosition ) );
                        xPosition = xNext;
                    }
                    yPosition = yNext;
                }
            }
            else // if angle is 0 or 315
            {
                int xPosition = 0;
                for ( int i = 0; i < gridXsteps; i++ )
                {
                    int xNext = ((i + 1) * m_width) / gridXsteps;
                    int yPosition = 0;
                    for ( int j = 0; j < gridYsteps; j++ )
                    {
                        int yNext = ((j + 1) * m_height) / gridYsteps;
                        m_transitionRects.push_back( QRect( xPosition, yPosition, xNext - xPosition, yNext - yPosition ) );
                        yPosition = yNext;
                    }
                    xPosition = xNext;
                }
            }
            // add a 'glitter' (1 over 10 pieces is randomized)
            int randomSteps = steps / 20;
            for ( int i = 0; i < randomSteps; i++ )
            {
                int n1 = (int)(steps * drand48());
                int n2 = (int)(steps * drand48());
                // swap items if index differs
                if ( n1 != n2 )
                {
                    QRect r = m_transitionRects[ n2 ];
                    m_transitionRects[ n2 ] = m_transitionRects[ n1 ];
                    m_transitionRects[ n1 ] = r;
                }
            }
            // set global transition parameters
            m_transitionMul = (angle == 90) || (angle == 270) ? gridYsteps : gridXsteps;
            m_transitionMul /= 2;
            m_transitionDelay = (int)( (m_transitionMul * 1000 * totalTime) / steps );
        } break;

        // implement missing transitions (a binary raster engine needed here)
        case KPDFPageTransition::Fly:

        case KPDFPageTransition::Push:

        case KPDFPageTransition::Cover:

        case KPDFPageTransition::Uncover:

        case KPDFPageTransition::Fade:

        default:
            update();
            return;
    }

    // send the first start to the timer
    m_transitionTimer->start( 0, true );
}


#include "presentationwidget.moc"
