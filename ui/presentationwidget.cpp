/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qevent.h>
#include <qfontmetrics.h>
#include <kicon.h>
#include <qtimer.h>
#include <qimage.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qtooltip.h>
#include <qvalidator.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#include <kcursor.h>
#include <krandom.h>
#include <qtoolbar.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kshortcut.h>
#include <kdialog.h>

// system includes
#include <stdlib.h>
#include <math.h>

// local includes
#include "presentationwidget.h"
#include "annotationtools.h"
#include "pagepainter.h"
#include "presentationsearchbar.h"
#include "core/audioplayer.h"
#include "core/document.h"
#include "core/generator.h"
#include "core/page.h"
#include "core/link.h"
#include "settings.h"


// comment this to disable the top-right progress indicator
#define ENABLE_PROGRESS_OVERLAY


// a frame contains a pointer to the page object, its geometry and the
// transition effect to the next frame
struct PresentationFrame
{
    const Okular::Page * page;
    QRect geometry;
};


// a custom QToolBar that basically does not propagate the event if the widget
// background is not automatically filled
class PresentationToolBar : public QToolBar
{
    public:
        PresentationToolBar( QWidget * parent = 0 )
            : QToolBar( parent )
        {}

    protected:
        void mousePressEvent( QMouseEvent * e )
        {
            QToolBar::mousePressEvent( e );
            e->accept();
        }

        void mouseReleaseEvent( QMouseEvent * e )
        {
            QToolBar::mouseReleaseEvent( e );
            e->accept();
        }
};


PresentationWidget::PresentationWidget( QWidget * parent, Okular::Document * doc )
    : QDialog( parent, Qt::FramelessWindowHint ),
    m_pressedLink( 0 ), m_handCursor( false ), m_drawingEngine( 0 ), m_document( doc ),
    m_frameIndex( -1 ), m_topBar( 0 ), m_pagesEdit( 0 ), m_searchBar( 0 )
{
    setModal( true );
    setAttribute( Qt::WA_DeleteOnClose );
    setAttribute( Qt::WA_OpaquePaintEvent );
    setObjectName( "presentationWidget" );
    QString caption = doc->metaData( "DocumentTitle" ).toString();
    if ( caption.trimmed().isEmpty() )
        caption = doc->currentDocument().fileName();
    setWindowTitle( KDialog::makeStandardCaption( caption ) );

    m_width = -1;

    // show widget and take control
    showFullScreen();

    // misc stuff
    setMouseTracking( true );
    setContextMenuPolicy( Qt::PreventContextMenu );
    m_transitionTimer = new QTimer( this );
    m_transitionTimer->setSingleShot( true );
    connect( m_transitionTimer, SIGNAL( timeout() ), this, SLOT( slotTransitionStep() ) );
    m_overlayHideTimer = new QTimer( this );
    m_overlayHideTimer->setSingleShot( true );
    connect( m_overlayHideTimer, SIGNAL( timeout() ), this, SLOT( slotHideOverlay() ) );
    m_nextPageTimer = new QTimer( this ); 
//    m_nextPageTimer->setSingleShot( true );
    connect( m_nextPageTimer, SIGNAL( timeout() ), this, SLOT( slotNextPage() ) ); 

    // handle cursor appearance as specified in configuration
    if ( Okular::Settings::slidesCursor() == Okular::Settings::EnumSlidesCursor::HiddenDelay )
    {
        KCursor::setAutoHideCursor( this, true );
        KCursor::setHideCursorDelay( 3000 );
    }
    else if ( Okular::Settings::slidesCursor() == Okular::Settings::EnumSlidesCursor::Hidden )
    {
        setCursor( KCursor::blankCursor() );
    }

    QTimer::singleShot( 0, this, SLOT( slotDelayedEvents() ) );
}

PresentationWidget::~PresentationWidget()
{
    // stop the audio playbacks
    Okular::AudioPlayer::instance()->stopPlaybacks();

    // remove our highlights
    if ( m_searchBar )
    {
        m_document->resetSearch( PRESENTATION_SEARCH_ID );
    }

    // remove this widget from document observer
    m_document->removeObserver( this );

    m_document->removePageAnnotations( m_document->viewport().pageNumber, m_currentPageDrawings );
    delete m_drawingEngine;

    // delete frames
    QVector< PresentationFrame * >::iterator fIt = m_frames.begin(), fEnd = m_frames.end();
    for ( ; fIt != fEnd; ++fIt )
        delete *fIt;
}


void PresentationWidget::notifySetup( const QVector< Okular::Page * > & pageSet, bool /*documentChanged*/ )
{
    // delete previous frames (if any (shouldn't be))
    QVector< PresentationFrame * >::iterator fIt = m_frames.begin(), fEnd = m_frames.end();
    for ( ; fIt != fEnd; ++fIt )
        delete *fIt;
    if ( !m_frames.isEmpty() )
        kWarning() << "Frames setup changed while a Presentation is in progress." << endl;
    m_frames.clear();

    // create the new frames
    QVector< Okular::Page * >::const_iterator setIt = pageSet.begin(), setEnd = pageSet.end();
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
    const Okular::DocumentInfo * info = m_document->documentInfo();
    if ( info )
    {
        if ( !info->get( "title" ).isNull() )
            m_metaStrings += i18n( "Title: %1", info->get( "title" ) );
        if ( !info->get( "author" ).isNull() )
            m_metaStrings += i18n( "Author: %1", info->get( "author" ) );
    }
    m_metaStrings += i18n( "Pages: %1", m_document->pages() );
    m_metaStrings += i18n( "Click to begin" );
}

void PresentationWidget::notifyViewportChanged( bool /*smoothMove*/ )
{
    // discard notifications if displaying the summary
    if ( m_frameIndex == -1 && Okular::Settings::slidesShowSummary() )
        return;

    // display the current page
    changePage( m_document->viewport().pageNumber );

    // auto advance to the next page if set
    startAutoChangeTimer();
}

void PresentationWidget::notifyPageChanged( int pageNumber, int changedFlags )
{
    // check if it's the last requested pixmap. if so update the widget.
    if ( (changedFlags & ( DocumentObserver::Pixmap | DocumentObserver::Annotations | DocumentObserver::Highlights ) ) && pageNumber == m_frameIndex )
        generatePage( changedFlags & DocumentObserver::Annotations );
}

bool PresentationWidget::canUnloadPixmap( int pageNumber ) const
{
    // can unload all pixmaps except for the currently visible one
    return pageNumber != m_frameIndex;
}

void PresentationWidget::setupActions( KActionCollection * collection )
{
    m_ac = collection;
}


// <widget events>
bool PresentationWidget::event( QEvent * e )
{
    if ( e->type() == QEvent::ToolTip )
    {
        QHelpEvent * he = (QHelpEvent*)e;

        QRect r;
        const Okular::Link * link = getLink( he->x(), he->y(), &r );

        if ( link )
        {
            QString tip = link->linkTip();
            if ( !tip.isEmpty() )
                QToolTip::showText( he->globalPos(), tip, this, r );
        }
        e->accept();
        return true;
    }
    else
        // do not stop the event
        return QDialog::event( e );
}

void PresentationWidget::keyPressEvent( QKeyEvent * e )
{
    if (m_width == -1) return;

#ifdef __GNUC__
#warning FIX the usage of KActionCollection actions
#endif
    if ( /*e->key() == m_ac->action( "previous_page" )->shortcut().keyQt() || */e->key() == Qt::Key_Left || e->key() == Qt::Key_Backspace || e->key() == Qt::Key_PageUp )
        slotPrevPage();
    else if ( /*e->key() == m_ac->action( "next_page" )->shortcut().keyQt() || */e->key() == Qt::Key_Right || e->key() == Qt::Key_Space || e->key() == Qt::Key_PageDown )
        slotNextPage();
    else if ( /*e->key() == m_ac->action( "first_page" )->shortcut().keyQt() || */e->key() == Qt::Key_Home )
        slotFirstPage();
    else if ( /*e->key() == m_ac->action( "last_page" )->shortcut().keyQt() || */e->key() == Qt::Key_End )
        slotLastPage();
    else if ( e->key() == Qt::Key_Escape )
    {
        if ( !m_topBar->isHidden() )
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
    if ( m_drawingEngine )
    {
        QRect r = routeMouseDrawingEvent( e );
        if ( r.isValid() )
        {
            m_drawingRect |= r.translated( m_frames[ m_frameIndex ]->geometry.topLeft() );
            update( m_drawingRect );
        }
        return;
    }

    // pressing left button
    if ( e->button() == Qt::LeftButton )
    {
        // if pressing on a link, skip other checks
        if ( ( m_pressedLink = getLink( e->x(), e->y() ) ) )
            return;

        // handle clicking on top-right overlay
        if ( m_overlayGeometry.contains( e->pos() ) )
        {
            overlayClick( e->pos() );
            return;
        }

        // if no other actions, go to next page
        slotNextPage();
    }
    // pressing right button
    else if ( e->button() == Qt::RightButton )
        slotPrevPage();
}

void PresentationWidget::mouseReleaseEvent( QMouseEvent * e )
{
    if ( m_drawingEngine )
    {
        QRect r = routeMouseDrawingEvent( e );
        (void)r;
        if ( m_drawingEngine->creationCompleted() )
        {
            QList< Okular::Annotation * > annots = m_drawingEngine->end();
            // manually disable and re-enable the pencil mode, so we can do
            // cleaning of the actual drawer and create a new one just after
            // that - that gives continuous drawing
            togglePencilMode( false );
            togglePencilMode( true );
            foreach( Okular::Annotation * ann, annots )
                m_document->addPageAnnotation( m_frameIndex, ann );
            m_currentPageDrawings << annots;
        }
        return;
    }

    // if releasing on the same link we pressed over, execute it
    if ( m_pressedLink && e->button() == Qt::LeftButton )
    {
        const Okular::Link * link = getLink( e->x(), e->y() );
        if ( link == m_pressedLink )
            m_document->processLink( link );
        m_pressedLink = 0;
    }
}

void PresentationWidget::mouseMoveEvent( QMouseEvent * e )
{
    // safety check
    if ( m_width == -1 )
        return;

    // update cursor and tooltip if hovering a link
    if ( !m_drawingEngine && Okular::Settings::slidesCursor() != Okular::Settings::EnumSlidesCursor::Hidden )
        testCursorOnLink( e->x(), e->y() );

    if ( !m_topBar->isHidden() )
    {
        // hide a shown bar when exiting the area
        if ( e->y() > ( m_topBar->height() + 1 ) )
        {
            m_topBar->hide();
            setFocus( Qt::OtherFocusReason );
        }
    }
    else
    {
        if ( m_drawingEngine && e->buttons() != Qt::NoButton )
        {
            QRect r = routeMouseDrawingEvent( e );
            {
                m_drawingRect |= r.translated( m_frames[ m_frameIndex ]->geometry.topLeft() );
                update( m_drawingRect );
            }
        }
        else
        {
            // show the bar if reaching top 2 pixels
            if ( e->y() <= (geometry().top() + 1) )
                m_topBar->show();
            // handle "dragging the wheel" if clicking on its geometry
            else if ( ( QApplication::mouseButtons() & Qt::LeftButton ) && m_overlayGeometry.contains( e->pos() ) )
                overlayClick( e->pos() );
        }
    }
}

void PresentationWidget::paintEvent( QPaintEvent * pe )
{
    if (m_width == -1)
    {
        QRect d = KGlobalSettings::desktopGeometry(this);
        m_width = d.width();
        m_height = d.height();

        // create top toolbar
        m_topBar = new PresentationToolBar( this );
        m_topBar->setObjectName( "presentationBar" );
        m_topBar->setIconSize( QSize( 32, 32 ) );
        m_topBar->setMovable( false );
        m_topBar->addAction( KIcon( layoutDirection() == Qt::RightToLeft ? "arrow-right" : "arrow-left" ), i18n("Previous Page"), this, SLOT( slotPrevPage() ) );
        m_pagesEdit = new QLineEdit( m_topBar );
        QSizePolicy sp = m_pagesEdit->sizePolicy();
        sp.setHorizontalPolicy( QSizePolicy::Minimum );
        m_pagesEdit->setSizePolicy( sp );
        QFontMetrics fm( m_pagesEdit->font() );
        m_pagesEdit->setMaximumWidth( fm.width( QString::number( m_document->pages() ) ) * 2 );
        QIntValidator *validator = new QIntValidator( 1, m_document->pages(), m_pagesEdit );
        m_pagesEdit->setValidator( validator );
        m_topBar->addWidget( m_pagesEdit );
        connect( m_pagesEdit, SIGNAL( returnPressed() ), this, SLOT( slotPageChanged() ) );
        m_topBar->addAction( KIcon( layoutDirection() == Qt::RightToLeft ? "arrow-left" : "arrow-right" ), i18n("Next Page"), this, SLOT( slotNextPage() ) );
        m_topBar->addSeparator();
        QAction * drawingAct = m_topBar->addAction( KIcon( "pencil" ), i18n( "Toggle Drawing Mode" ) );
        drawingAct->setCheckable( true );
        connect( drawingAct, SIGNAL( toggled( bool ) ), this, SLOT( togglePencilMode( bool ) ) );
        QAction * eraseDrawingAct = m_topBar->addAction( KIcon( "eraser" ), i18n( "Erase Drawings" ) );
        connect( eraseDrawingAct, SIGNAL( triggered() ), this, SLOT( clearDrawings() ) );
        QWidget *spacer = new QWidget(m_topBar);
        spacer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::MinimumExpanding );
        m_topBar->addWidget( spacer );
        m_topBar->addAction( KIcon("application-exit"), i18n("Exit Presentation Mode"), this, SLOT( close() ) );
        m_topBar->setGeometry( 0, 0, m_width, 32 + 10 );
        m_topBar->hide();
        // change topbar background color
        QPalette p = m_topBar->palette();
        p.setColor( QPalette::Active, QPalette::Button, Qt::gray );
        p.setColor( QPalette::Active, QPalette::Background, Qt::darkGray );
        m_topBar->setPalette( p );

        connect( m_document, SIGNAL( linkFind() ), this, SLOT( slotFind() ) );

        // register this observer in document. events will come immediately
        m_document->addObserver( this );

        // show summary if requested
        if ( Okular::Settings::slidesShowSummary() )
            generatePage();
    }

    // check painting rect consistancy
    QRect r = pe->rect().intersect( geometry() );
    if ( r.isNull() || m_lastRenderedPixmap.isNull() )
        return;

    // blit the pixmap to the screen
    QVector<QRect> allRects = pe->region().rects();
    uint numRects = allRects.count();
    QPainter painter( this );
    for ( uint i = 0; i < numRects; i++ )
    {
        const QRect & r = allRects[i];
        if ( !r.isValid() )
            continue;
#ifdef ENABLE_PROGRESS_OVERLAY
        if ( Okular::Settings::slidesShowProgress() && r.intersects( m_overlayGeometry ) )
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
            painter.drawPixmap( r.topLeft(), backPixmap, backPixmap.rect() );
        } else
#endif
        // copy the rendered pixmap to the screen
        painter.drawPixmap( r.topLeft(), m_lastRenderedPixmap, r );
    }
    if ( m_drawingEngine && m_drawingRect.intersects( pe->rect() ) )
    {
        const QRect & geom = m_frames[ m_frameIndex ]->geometry;
        painter.save();
        painter.translate( geom.topLeft() );
        m_drawingEngine->paint( &painter, geom.width(), geom.height(), m_drawingRect.intersect( pe->rect() ) );
        painter.restore();
    }
    painter.end();
}
// </widget events>


const Okular::Link * PresentationWidget::getLink( int x, int y, QRect * geometry ) const
{
    // no links on invalid pages
    if ( geometry && !geometry->isNull() )
        geometry->setRect( 0, 0, 0, 0 );
    if ( m_frameIndex < 0 || m_frameIndex >= (int)m_frames.size() )
        return 0;

    // get frame, page and geometry
    const PresentationFrame * frame = m_frames[ m_frameIndex ];
    const Okular::Page * page = frame->page;
    const QRect & frameGeometry = frame->geometry;

    // compute normalized x and y
    double nx = (double)(x - frameGeometry.left()) / (double)frameGeometry.width();
    double ny = (double)(y - frameGeometry.top()) / (double)frameGeometry.height();

    // no links outside the pages
    if ( nx < 0 || nx > 1 || ny < 0 || ny > 1 )
        return 0;

    // check if 1) there is an object and 2) it's a link
    QRect d = KGlobalSettings::desktopGeometry( const_cast< PresentationWidget * >( this ) );
    const Okular::ObjectRect * object = page->objectRect( Okular::ObjectRect::Link, nx, ny, d.width(), d.height() );
    if ( !object )
        return 0;

    // compute link geometry if destination rect present
    if ( geometry )
    {
        *geometry = object->boundingRect( frameGeometry.width(), frameGeometry.height() );
        geometry->translate( frameGeometry.left(), frameGeometry.top() );
    }

    // return the link pointer
    return (Okular::Link *)object->object();
}

void PresentationWidget::testCursorOnLink( int x, int y )
{
    const Okular::Link * link = getLink( x, y, 0 );

    // only react on changes (in/out from a link)
    if ( (link && !m_handCursor) || (!link && m_handCursor) )
    {
        // change cursor shape
        m_handCursor = link != 0;
        setCursor( m_handCursor ? KCursor::handCursor() : KCursor::arrowCursor() );
    }
}

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

    bool signalsBlocked = m_pagesEdit->signalsBlocked();
    m_pagesEdit->blockSignals( true );
    m_pagesEdit->setText( QString::number( m_frameIndex + 1 ) );
    m_pagesEdit->blockSignals( signalsBlocked );

    // if pixmap not inside the Okular::Page we request it and wait for
    // notifyPixmapChanged call or else we can proceed to pixmap generation
    if ( !frame->page->hasPixmap( PRESENTATION_ID, pixW, pixH ) )
    {
        // operation will take long: set busy cursor
        QApplication::setOverrideCursor( KCursor::workingCursor() );
        // request the pixmap
        QLinkedList< Okular::PixmapRequest * > requests;
        requests.push_back( new Okular::PixmapRequest( PRESENTATION_ID, m_frameIndex, pixW, pixH, PRESENTATION_PRIO, false ) );
        // restore cursor
        QApplication::restoreOverrideCursor();
        // ask for next and previous page if not in low memory usage setting
        if (Okular::Settings::memoryLevel() != Okular::Settings::EnumMemoryLevel::Low && Okular::Settings::enableThreading()) {
            if (newPage + 1 < (int)m_document->pages())
            {
                PresentationFrame *nextFrame = m_frames[ newPage + 1 ];
                pixW = nextFrame->geometry.width();
                pixH = nextFrame->geometry.height();
                if ( !nextFrame->page->hasPixmap( PRESENTATION_ID, pixW, pixH ) )
                    requests.push_back( new Okular::PixmapRequest( PRESENTATION_ID, newPage + 1, pixW, pixH, PRESENTATION_PRELOAD_PRIO, true ) );
            }
            if (newPage - 1 >= 0)
            {
                PresentationFrame *prevFrame = m_frames[ newPage - 1 ];
                pixW = prevFrame->geometry.width();
                pixH = prevFrame->geometry.height();
                if ( !prevFrame->page->hasPixmap( PRESENTATION_ID, pixW, pixH ) )
                    requests.push_back( new Okular::PixmapRequest( PRESENTATION_ID, newPage - 1, pixW, pixH, PRESENTATION_PRELOAD_PRIO, true ) );
            }
        }
        m_document->requestPixmaps( requests );
    }
    else
    {
        // make the background pixmap
        generatePage();
    }

    // set a new viewport in document if page number differs
    if ( m_frameIndex != -1 && m_frameIndex != m_document->viewport().pageNumber )
    {
        // stop the audio playback, if any
        Okular::AudioPlayer::instance()->stopPlaybacks();
        // perform the page closing action, if any
        if ( m_document->page( m_document->viewport().pageNumber )->pageAction( Okular::Page::Closing ) )
            m_document->processLink( m_document->page( m_document->viewport().pageNumber )->pageAction( Okular::Page::Closing ) );

        // remove the drawing on the old page before switching
        clearDrawings();
        m_document->setViewportPage( m_frameIndex, PRESENTATION_ID );

        // perform the page opening action, if any
        if ( m_document->page( m_frameIndex)->pageAction( Okular::Page::Opening ) )
            m_document->processLink( m_document->page( m_frameIndex )->pageAction( Okular::Page::Opening ) );

    }
}

void PresentationWidget::generatePage( bool disableTransition )
{
    if ( m_lastRenderedPixmap.isNull() )
        m_lastRenderedPixmap = QPixmap( m_width, m_height );

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
    if ( Okular::Settings::slidesShowProgress() && m_frameIndex != -1 )
        generateOverlay();
#endif

    // start transition on pages that have one
    if ( !disableTransition )
    {
        const Okular::PageTransition * transition = m_frameIndex != -1 ?
            m_frames[ m_frameIndex ]->page->transition() : 0;
        if ( transition )
            initTransition( transition );
        else {
            Okular::PageTransition trans = defaultTransition();
            initTransition( &trans );
        }
    }
    else
    {
        Okular::PageTransition trans = defaultTransition( Okular::Settings::EnumSlidesTransition::Replace );
        initTransition( &trans );
    }

    // update cursor + tooltip
    if ( Okular::Settings::slidesCursor() != Okular::Settings::EnumSlidesCursor::Hidden )
    {
        QPoint p = mapFromGlobal( QCursor::pos() );
        testCursorOnLink( p.x(), p.y() );
    }
}

void PresentationWidget::generateIntroPage( QPainter & p )
{
    // use a vertical gray gradient background
    int blend1 = m_height / 10,
        blend2 = 9 * m_height / 10;
    int baseTint = QColor(Qt::gray).red();
    for ( int i = 0; i < m_height; i++ )
    {
        int k = baseTint;
        if ( i < blend1 )
            k -= (int)( baseTint * (i-blend1)*(i-blend1) / (float)(blend1 * blend1) );
        if ( i > blend2 )
            k += (int)( (255-baseTint) * (i-blend2)*(i-blend2) / (float)(blend1 * blend1) );
        p.fillRect( 0, i, m_width, 1, QColor( k, k, k ) );
    }

    // draw okular logo in the four corners
    QPixmap logo = DesktopIcon( "okular", 64 );
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
                    Qt::AlignHCenter | Qt::AlignVCenter, m_metaStrings[i] );
        // text body
        p.setPen( 128 + (127 * i) / strNum );
        p.drawText( 0, m_height / 4 + strHeight * i, m_width, strHeight,
                    Qt::AlignHCenter | Qt::AlignVCenter, m_metaStrings[i] );
    }
}

void PresentationWidget::generateContentsPage( int pageNum, QPainter & p )
{
    PresentationFrame * frame = m_frames[ pageNum ];

    // translate painter and contents rect
    QRect geom( frame->geometry );
    p.translate( geom.left(), geom.top() );
    geom.translate( -geom.left(), -geom.top() );

    // draw the page using the shared PagePainter class
    int flags = PagePainter::Accessibility | PagePainter::Highlights | PagePainter::Annotations;
    PagePainter::paintPageOnPainter( &p, frame->page, PRESENTATION_ID, flags,
                                     geom.width(), geom.height(), geom );

    // restore painter
    p.translate( -frame->geometry.left(), -frame->geometry.top() );

    // fill unpainted areas with background color
    QRegion unpainted( QRect( 0, 0, m_width, m_height ) );
    QVector<QRect> rects = unpainted.subtract( frame->geometry ).rects();
    for ( int i = 0; i < rects.count(); i++ )
    {
        const QRect & r = rects[i];
        p.fillRect( r, Okular::Settings::slidesBackgroundColor() );
    }
}

// from Arthur - Qt4 - (is defined elsewhere as 'qt_div_255' to not break final compilation)
inline int qt_div255(int x) { return (x + (x>>8) + 0x80) >> 8; }
void PresentationWidget::generateOverlay()
{
#ifdef ENABLE_PROGRESS_OVERLAY
    // calculate overlay geometry and resize pixmap if needed
    int side = m_width / 16;
    m_overlayGeometry.setRect( m_width - side - 4, 4, side, side );

    // note: to get a sort of antialiasing, we render the pixmap double sized
    // and the resulting image is smoothly scaled down. So here we open a
    // painter on the double sized pixmap.
    side *= 2;
    QPixmap doublePixmap( side, side );
    doublePixmap.fill( Qt::black );
    QPainter pixmapPainter( &doublePixmap );
    pixmapPainter.setRenderHints( QPainter::Antialiasing );

    // draw PIE SLICES in blue levels (the levels will then be the alpha component)
    int pages = m_document->pages();
    if ( pages > 28 )
    {   // draw continuous slices
        int degrees = (int)( 360 * (float)(m_frameIndex + 1) / (float)pages );
        pixmapPainter.setPen( 0x05 );
        pixmapPainter.setBrush( QColor( 0x40 ) );
        pixmapPainter.drawPie( 2, 2, side - 4, side - 4, 90*16, (360-degrees)*16 );
        pixmapPainter.setPen( 0x40 );
        pixmapPainter.setBrush( QColor( 0xF0 ) );
        pixmapPainter.drawPie( 2, 2, side - 4, side - 4, 90*16, -degrees*16 );
    }
    else
    {   // draw discrete slices
        float oldCoord = -90;
        for ( int i = 0; i < pages; i++ )
        {
            float newCoord = -90 + 360 * (float)(i + 1) / (float)pages;
            pixmapPainter.setPen( i <= m_frameIndex ? 0x40 : 0x05 );
            pixmapPainter.setBrush( QColor( i <= m_frameIndex ? 0xF0 : 0x40 ) );
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
    QImage image( doublePixmap.toImage().scaled( side / 2, side / 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
    image = image.convertToFormat( QImage::Format_ARGB32 );

    // draw circular shadow using the same technique
    doublePixmap.fill( Qt::black );
    pixmapPainter.begin( &doublePixmap );
    pixmapPainter.setPen( 0x40 );
    pixmapPainter.setBrush( QColor( 0x80 ) );
    pixmapPainter.drawEllipse( 0, 0, side, side );
    pixmapPainter.end();
    QImage shadow( doublePixmap.toImage().scaled( side / 2, side / 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );

    // generate a 2 colors pixmap using mixing shadow (made with highlight color)
    // and image (made with highlightedText color)
    QPalette pal = palette();
    QColor color = pal.color( QPalette::Active, QPalette::HighlightedText );
    int red = color.red(), green = color.green(), blue = color.blue();
    color = pal.color( QPalette::Active, QPalette::Highlight );
    int sRed = color.red(), sGreen = color.green(), sBlue = color.blue();
    // pointers
    unsigned int * data = (unsigned int *)image.bits(),
                 * shadowData = (unsigned int *)shadow.bits(),
                 pixels = image.width() * image.height();
    // cache data (reduce computation time to 26%!)
    int c1 = -1, c2 = -1, cR = 0, cG = 0, cB = 0, cA = 0;
    // foreach pixel
    for( unsigned int i = 0; i < pixels; ++i )
    {
        // alpha for shadow and image
        int shadowAlpha = shadowData[i] & 0xFF,
            srcAlpha = data[i] & 0xFF;
        // cache values
        if ( srcAlpha != c1 || shadowAlpha != c2 )
        {
            c1 = srcAlpha;
            c2 = shadowAlpha;
            // fuse color components and alpha value of image over shadow
            data[i] = qRgba(
                cR = qt_div255( srcAlpha * red   + (255 - srcAlpha) * sRed ),
                cG = qt_div255( srcAlpha * green + (255 - srcAlpha) * sGreen ),
                cB = qt_div255( srcAlpha * blue  + (255 - srcAlpha) * sBlue ),
                cA = qt_div255( srcAlpha * srcAlpha + (255 - srcAlpha) * shadowAlpha )
            );
        }
        else
            data[i] = qRgba( cR, cG, cB, cA );
    }
    m_lastRenderedOverlay = QPixmap::fromImage( image );

    // start the autohide timer
    repaint( m_overlayGeometry ); // toggle with next line
    //update( m_overlayGeometry );
    m_overlayHideTimer->start( 2500 );
#endif
}


QRect PresentationWidget::routeMouseDrawingEvent( QMouseEvent * e )
{
    const QRect & geom = m_frames[ m_frameIndex ]->geometry;
    const Okular::Page * page = m_frames[ m_frameIndex ]->page;

    AnnotatorEngine::EventType eventType = AnnotatorEngine::Press;
    if ( e->type() == QEvent::MouseMove )
        eventType = AnnotatorEngine::Move;
    else if ( e->type() == QEvent::MouseButtonRelease )
        eventType = AnnotatorEngine::Release;

    // find out the pressed button
    AnnotatorEngine::Button button = AnnotatorEngine::None;
    Qt::MouseButtons buttonState = ( eventType == AnnotatorEngine::Move ) ? e->buttons() : e->button();
    if ( buttonState == Qt::LeftButton )
        button = AnnotatorEngine::Left;
    else if ( buttonState == Qt::RightButton )
        button = AnnotatorEngine::Right;
    static bool hasclicked = false;
    if ( eventType == AnnotatorEngine::Press )
        hasclicked = true;

    double nX = ( (double)e->x() - (double)geom.left() ) / (double)geom.width();
    double nY = ( (double)e->y() - (double)geom.top() ) / (double)geom.height();
    QRect ret;
    if ( hasclicked && nX >= 0 && nX < 1 && nY >= 0 && nY < 1 )
        ret = m_drawingEngine->event( eventType, button, nX, nY, geom.width(), geom.height(), page );

    if ( eventType == AnnotatorEngine::Release )
        hasclicked = false;

    return ret;
}

void PresentationWidget::startAutoChangeTimer()
{
    double pageDuration = m_frameIndex >= 0 && m_frameIndex < (int)m_frames.count() ? m_frames[ m_frameIndex ]->page->duration() : -1;
    if ( Okular::Settings::slidesAdvance() || pageDuration > 0.0 )
    {
        double secs = pageDuration <= 0.0
                   ? Okular::Settings::slidesAdvanceTime()
                   : qMin<double>( pageDuration, Okular::Settings::slidesAdvanceTime() );
        m_nextPageTimer->start( (int)( secs * 1000 ) );
    }
}



void PresentationWidget::slotNextPage()
{
    // loop when configured
    if ( m_frameIndex == (int)m_frames.count() - 1 && Okular::Settings::slidesLoop() )
        m_frameIndex = -1;

    if ( m_frameIndex < (int)m_frames.count() - 1 )
    {
        // go to next page
        changePage( m_frameIndex + 1 );
        // auto advance to the next page if set
        startAutoChangeTimer();
    }
    else
    {
#ifdef ENABLE_PROGRESS_OVERLAY
        if ( Okular::Settings::slidesShowProgress() )
            generateOverlay();
#endif
        if ( m_transitionTimer->isActive() )
        {
            m_transitionTimer->stop();
            update();
        }
    }
    // we need the setFocus() call here to let KCursor::autoHide() work correctly
    setFocus();
}

void PresentationWidget::slotPrevPage()
{
    if ( m_frameIndex > 0 )
    {
        // go to previous page
        changePage( m_frameIndex - 1 );

        // auto advance to the next page if set
        startAutoChangeTimer();
    }
    else
    {
#ifdef ENABLE_PROGRESS_OVERLAY
        if ( Okular::Settings::slidesShowProgress() )
            generateOverlay();
#endif
        if ( m_transitionTimer->isActive() )
        {
            m_transitionTimer->stop();
            update();
        }
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
    m_transitionTimer->start( m_transitionDelay );
}

void PresentationWidget::slotDelayedEvents()
{
  // inform user on how to exit from presentation mode
  KMessageBox::information( this, i18n("There are two ways of exiting presentation mode, you can press either ESC key or click with the quit button that appears when placing the mouse in the top-right corner. Of course you can cycle windows (Alt+TAB by default)"), QString::null, "presentationInfo" );
}

void PresentationWidget::slotPageChanged()
{
    bool ok = true;
    int p = m_pagesEdit->text().toInt( &ok );
    if ( !ok )
        return;

    changePage( p - 1 );
}

void PresentationWidget::togglePencilMode( bool on )
{
    if ( on )
    {
        QString colorstring = Okular::Settings::slidesPencilColor().name();
        // FIXME this should not be recreated every time
        QDomDocument doc( "engine" );
        QDomElement root = doc.createElement( "engine" );
        root.setAttribute( "color", colorstring );
        doc.appendChild( root );
        QDomElement annElem = doc.createElement( "annotation" );
        root.appendChild( annElem );
        annElem.setAttribute( "type", "Ink" );
        annElem.setAttribute( "color", colorstring );
        annElem.setAttribute( "width", "2" );
        m_drawingEngine = new SmoothPathEngine( root );
    }
    else
    {
        delete m_drawingEngine;
        m_drawingEngine = 0;
        m_drawingRect = QRect();
    }
}

void PresentationWidget::clearDrawings()
{
    m_document->removePageAnnotations( m_document->viewport().pageNumber, m_currentPageDrawings );
    m_currentPageDrawings.clear();
}

void PresentationWidget::slotFind()
{
    if ( !m_searchBar )
    {
        m_searchBar = new PresentationSearchBar( m_document, this, this );
        m_searchBar->forceSnap();
    }
    m_searchBar->show();
}


const Okular::PageTransition PresentationWidget::defaultTransition() const
{
    return defaultTransition( Okular::Settings::slidesTransition() );
}

const Okular::PageTransition PresentationWidget::defaultTransition( int type ) const
{
    switch ( type )
    {
        case Okular::Settings::EnumSlidesTransition::BlindsHorizontal:
        {
            Okular::PageTransition transition( Okular::PageTransition::Blinds );
            transition.setAlignment( Okular::PageTransition::Horizontal );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::BlindsVertical:
        {
            Okular::PageTransition transition( Okular::PageTransition::Blinds );
            transition.setAlignment( Okular::PageTransition::Vertical );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::BoxIn:
        {
            Okular::PageTransition transition( Okular::PageTransition::Box );
            transition.setDirection( Okular::PageTransition::Inward );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::BoxOut:
        {
            Okular::PageTransition transition( Okular::PageTransition::Box );
            transition.setDirection( Okular::PageTransition::Outward );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::Dissolve:
        {
            return Okular::PageTransition( Okular::PageTransition::Dissolve );
            break;
        }
        case Okular::Settings::EnumSlidesTransition::GlitterDown:
        {
            Okular::PageTransition transition( Okular::PageTransition::Glitter );
            transition.setAngle( 270 );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::GlitterRight:
        {
            Okular::PageTransition transition( Okular::PageTransition::Glitter );
            transition.setAngle( 0 );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::GlitterRightDown:
        {
            Okular::PageTransition transition( Okular::PageTransition::Glitter );
            transition.setAngle( 315 );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::Random:
        {
            return defaultTransition( KRandom::random() % 18 );
            break;
        }
        case Okular::Settings::EnumSlidesTransition::SplitHorizontalIn:
        {
            Okular::PageTransition transition( Okular::PageTransition::Split );
            transition.setAlignment( Okular::PageTransition::Horizontal );
            transition.setDirection( Okular::PageTransition::Inward );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::SplitHorizontalOut:
        {
            Okular::PageTransition transition( Okular::PageTransition::Split );
            transition.setAlignment( Okular::PageTransition::Horizontal );
            transition.setDirection( Okular::PageTransition::Outward );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::SplitVerticalIn:
        {
            Okular::PageTransition transition( Okular::PageTransition::Split );
            transition.setAlignment( Okular::PageTransition::Vertical );
            transition.setDirection( Okular::PageTransition::Inward );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::SplitVerticalOut:
        {
            Okular::PageTransition transition( Okular::PageTransition::Split );
            transition.setAlignment( Okular::PageTransition::Vertical );
            transition.setDirection( Okular::PageTransition::Outward );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::WipeDown:
        {
            Okular::PageTransition transition( Okular::PageTransition::Wipe );
            transition.setAngle( 270 );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::WipeRight:
        {
            Okular::PageTransition transition( Okular::PageTransition::Wipe );
            transition.setAngle( 0 );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::WipeLeft:
        {
            Okular::PageTransition transition( Okular::PageTransition::Wipe );
            transition.setAngle( 180 );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::WipeUp:
        {
            Okular::PageTransition transition( Okular::PageTransition::Wipe );
            transition.setAngle( 90 );
            return transition;
            break;
        }
        case Okular::Settings::EnumSlidesTransition::Replace:
        default:
            return Okular::PageTransition( Okular::PageTransition::Replace );
            break;
    }
    // should not happen, just make gcc happy
    return Okular::PageTransition();
}

/** ONLY the TRANSITIONS GENERATION function from here on **/
void PresentationWidget::initTransition( const Okular::PageTransition *transition )
{
    // if it's just a 'replace' transition, repaint the screen
    if ( transition->type() == Okular::PageTransition::Replace )
    {
        update();
        return;
    }

    const bool isInward = transition->direction() == Okular::PageTransition::Inward;
    const bool isHorizontal = transition->alignment() == Okular::PageTransition::Horizontal;
    const float totalTime = transition->duration();

    m_transitionRects.clear();

    switch( transition->type() )
    {
            // split: horizontal / vertical and inward / outward
        case Okular::PageTransition::Split:
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
        case Okular::PageTransition::Blinds:
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
        case Okular::PageTransition::Box:
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
        case Okular::PageTransition::Wipe:
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
        case Okular::PageTransition::Dissolve:
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
        case Okular::PageTransition::Glitter:
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
        case Okular::PageTransition::Fly:

        case Okular::PageTransition::Push:

        case Okular::PageTransition::Cover:

        case Okular::PageTransition::Uncover:

        case Okular::PageTransition::Fade:

        default:
            update();
            return;
    }

    // send the first start to the timer
    m_transitionTimer->start( 0 );
}


#include "presentationwidget.moc"
