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
#include <kcursor.h>
#include <ktoolbar.h>
#include <kdebug.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kimageeffect.h>

// system includes
#include <stdlib.h>

// local includes
#include "presentationwidget.h"
#include "core/document.h"   // for PRESENTATION_ID
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


PresentationWidget::PresentationWidget( KPDFDocument * doc )
    : QWidget( 0, "presentationWidget", WDestructiveClose | WStyle_NoBorder |
    WStyle_StaysOnTop | WShowModal ), m_document( doc ), m_frameIndex( -1 )
{
    // set look and geometry
    setBackgroundMode( Qt::NoBackground );
    QDesktopWidget * d = QApplication::desktop();
    m_width = d->width();
    m_height = d->height();

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

    // misc stuff
    setMouseTracking( true );
    m_transitionTimer = new QTimer( this );
    connect( m_transitionTimer, SIGNAL( timeout() ), this, SLOT( slotTransitionStep() ) );
    m_overlayHideTimer = new QTimer( this );
    connect( m_overlayHideTimer, SIGNAL( timeout() ), this, SLOT( slotHideOverlay() ) );

    // register this observer in document
    m_document->addObserver( this );

    // show widget and take control
    showFullScreen();
    if ( Settings::slidesShowSummary() )
        generatePage();
    else
        slotNextPage();

    KCursor::setAutoHideCursor( this, true );
    KCursor::setHideCursorDelay( 3000 );
}

PresentationWidget::~PresentationWidget()
{
    // remove this widget from document observer
    m_document->removeObserver( this );

    // delete frames
    QValueVector< PresentationFrame * >::iterator fIt = m_frames.begin(), fEnd = m_frames.end();
    for ( ; fIt != fEnd; ++fIt )
        delete *fIt;
}


void PresentationWidget::pageSetup( const QValueVector<KPDFPage*> & pageSet, bool /*changed*/ )
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
        if ( !info->title.isNull() )
            m_metaStrings += i18n( "Title: %1" ).arg( info->title );
        if ( !info->title.isNull() )
            m_metaStrings += i18n( "Author: %1" ).arg( info->author );
    }
    m_metaStrings += i18n( "Pages: %1" ).arg( m_document->pages() );
    m_metaStrings += i18n( "Click to begin" );
}

bool PresentationWidget::canUnloadPixmap( int pageNumber )
{
    // can unload all pixmaps except for the currently visible one
    return pageNumber != m_frameIndex;
}

void PresentationWidget::notifyPixmapChanged( int pageNumber )
{
    // check if it's the last requested pixmap. if so update the widget.
    if ( pageNumber == m_frameIndex )
        generatePage();
}


// <widget events>
void PresentationWidget::keyPressEvent( QKeyEvent * e )
{
    if ( e->key() == Key_Left || e->key() == Key_Backspace )
        slotPrevPage();
    else if ( e->key() == Key_Right || e->key() == Key_Space )
        slotNextPage();
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
            slotNextPage();
    }
    else if ( div < 0 )
    {
        if ( div < -3 )
            div = -3;
        while ( div++ )
            slotPrevPage();
    }
}

void PresentationWidget::mousePressEvent( QMouseEvent * e )
{
    if ( e->button() == Qt::LeftButton )
        slotNextPage();
    else if ( e->button() == Qt::RightButton )
        slotPrevPage();
}

void PresentationWidget::mouseMoveEvent( QMouseEvent * e )
{
    if ( m_topBar->isShown() )
    {
        if ( e->y() > ( m_topBar->height() + 1 ) )
            m_topBar->hide();
    }
    else if ( !e->y() )
        m_topBar->show();
}

void PresentationWidget::paintEvent( QPaintEvent * pe )
{
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
    else
        update();
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
        if ( Settings::slidesShowGrayBack() )
        {
            // use a vertical gray gradient background
            int baseTint = Qt::gray.red(),
                blendLevel = 9 * m_height / 10,
                blendLeft = r.left(),
                blendWidth = r.width();
            float blendDiv = (m_height * m_height) / 100; // use 100 to fade to pure white
            QColor baseColor( baseTint, baseTint, baseTint );
            for ( int i = r.top(); i <= r.bottom(); i++ )
            {
                if ( i <= blendLevel )
                    p.fillRect( blendLeft, i, blendWidth, 1, baseColor );
                else
                {
                    int k = i - blendLevel;
                    k = baseTint + (int)( (255-baseTint) * (k * k) / blendDiv );
                    p.fillRect( blendLeft, i, blendWidth, 1, QColor( baseTint, baseTint, k ) );
                }
            }
        }
        else
        {
            // use the black color that 'crops' images on beamers
            p.fillRect( r, Qt::black );
        }
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
    {   // draw continous slices
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

    // generate a monochrome pixmap using grey level as alpha channel
    int pixels = image.width() * image.height();
    unsigned int * data = (unsigned int *)image.bits();
    for( int i = 0; i < pixels; ++i )
        data[i] = qRgba( 0, 0, 0, data[i] & 0xFF ); // base color can be changed here
    m_lastRenderedOverlay.convertFromImage( image );

    // start the autohide timer
    repaint( m_overlayGeometry, false /*clear*/ ); // toggle with next line
    //update( m_overlayGeometry );
    m_overlayHideTimer->start( 1400, true );
#endif
}



void PresentationWidget::slotNextPage()
{
    if ( m_frameIndex < (int)m_frames.count() - 1 )
    {
        // go to next page
        ++m_frameIndex;

        // check if pixmap exists or else request it
        PresentationFrame * frame = m_frames[ m_frameIndex ];
        int pixW = frame->geometry.width();
        int pixH = frame->geometry.height();
        // if pixmap not inside the KPDFPage we request it and wait for
        // notifyPixmapChanged call or else we proceed to pixmap generation
        if ( !frame->page->hasPixmap( PRESENTATION_ID, pixW, pixH ) )
        {
            QValueList< PixmapRequest * > request;
            request.push_back( new PixmapRequest( PRESENTATION_ID, m_frameIndex, pixW, pixH ) );
            m_document->requestPixmaps( request, false );
        }
        else
            generatePage();
    }
    else if ( m_transitionTimer->isActive() )
    {
        m_transitionTimer->stop();
        update();
    }

    setFocus();
}

void PresentationWidget::slotPrevPage()
{
    if ( m_frameIndex > 0 )
    {
        // go to previous page
        --m_frameIndex;

        // check if pixmap exists or else request it
        PresentationFrame * frame = m_frames[ m_frameIndex ];
        int pixW = frame->geometry.width();
        int pixH = frame->geometry.height();
        // if pixmap not inside the KPDFPage we request it and wait for
        // notifyPixmapChanged call or else we can proceed to pixmap generation
        if ( !frame->page->hasPixmap( PRESENTATION_ID, pixW, pixH ) )
        {
            QValueList< PixmapRequest * > request;
            request.push_back( new PixmapRequest( PRESENTATION_ID, m_frameIndex, pixW, pixH ) );
            m_document->requestPixmaps( request, false );
        }
        else
            generatePage();
    }
    else if ( m_transitionTimer->isActive() )
    {
        m_transitionTimer->stop();
        update();
    }
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
        return;

    for ( int i = 0; i < m_transitionMul && !m_transitionRects.empty(); i++ )
    {
        update( m_transitionRects.first() );
        m_transitionRects.pop_front();
    }
    m_transitionTimer->start( m_transitionDelay, true );
}


/** ONLY the TRANSITIONS GENERATION function from here on **/
void PresentationWidget::initTransition( const KPDFPageTransition *transition )
{
    m_transitionRects.clear();
    const int gridXstep = 50;
    const int gridYstep = 38;
    switch( transition->type() )
    {
         // TODO: implement missing transitions
        case KPDFPageTransition::Replace:
            update();
            return;
        case KPDFPageTransition::Split:
            update();
            return;
        case KPDFPageTransition::Blinds:
            update();
            return;
        case KPDFPageTransition::Box:
            update();
            return;
        case KPDFPageTransition::Wipe:
            update();
            return;
        case KPDFPageTransition::Dissolve:
            update();
            return;
        case KPDFPageTransition::Glitter: {
            int oldX = 0,
                oldY = 0;
            // create a grid of gridXstep by gridYstep QRects
            for ( int y = 0; y < gridYstep; y++ )
            {
                int newY = (int)( m_height * ((float)(y+1) / (float)gridYstep) );
                for ( int x = 0; x < gridXstep; x++ )
                {
                    int newX = (int)( m_width * ((float)(x+1) / (float)gridXstep) );
                    m_transitionRects.push_back( QRect( oldX, oldY, newX - oldX, newY - oldY ) );
                    oldX = newX;
                }
                oldX = 0;
                oldY = newY;
            }
            // randomize the grid
            int steps = gridXstep * gridYstep;
            for ( int i = 0; i < steps; i++ )
            {
                int n1 = (int)(steps * drand48());
                int n2 = (int)(steps * drand48());
                if ( n1 != n2 )
                {
                    //swap items
                    QRect r = m_transitionRects[ n2 ];
                    m_transitionRects[ n2 ] = m_transitionRects[ n1 ];
                    m_transitionRects[ n1 ] = r;
                }
            }
            // set global transition parameters
            m_transitionMul = 40;
            m_transitionDelay = (m_transitionMul * 500) / steps;
            } break;
        case KPDFPageTransition::Fly:
            update();
            return;
        case KPDFPageTransition::Push:
            update();
            return;
        case KPDFPageTransition::Cover:
            update();
            return;
        case KPDFPageTransition::Uncover:
            update();
            return;
        case KPDFPageTransition::Fade:
            update();
            return;
    }

    // send the first start to the timer
    m_transitionTimer->start( 0, true );
}


#include "presentationwidget.moc"
