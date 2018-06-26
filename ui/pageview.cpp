/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *   Copyright (C) 2004-2006 by Albert Astals Cid <aacid@kde.org>          *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   With portions of code from kpdf/kpdf_pagewidget.cc by:                *
 *     Copyright (C) 2002 by Wilco Greven <greven@kde.org>                 *
 *     Copyright (C) 2003 by Christophe Devriese                           *
 *                           <Christophe.Devriese@student.kuleuven.ac.be>  *
 *     Copyright (C) 2003 by Laurent Montel <montel@kde.org>               *
 *     Copyright (C) 2003 by Dirk Mueller <mueller@kde.org>                *
 *     Copyright (C) 2004 by James Ots <kde@jamesots.com>                  *
 *     Copyright (C) 2011 by Jiri Baum - NICTA <jiri@baum.com.au>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "pageview.h"

// qt/kde includes
#include <QtCore/qloggingcategory.h>
#include <qcursor.h>
#include <qevent.h>
#include <qimage.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qset.h>
#include <qscrollbar.h>
#include <qtooltip.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qmenu.h>
#include <QInputDialog>
#include <qdesktopwidget.h>
#include <QDesktopServices>
#include <QMimeDatabase>
#include <QMimeData>
#include <QGestureEvent>

#include <qaction.h>
#include <kactionmenu.h>
#include <kstandardaction.h>
#include <kactioncollection.h>
#include <KLocalizedString>
#include <kselectaction.h>
#include <ktoggleaction.h>
#include <QtCore/QDebug>
#include <kmessagebox.h>
#include <QIcon>
#include <kurifilter.h>
#include <kstringhandler.h>
#include <ktoolinvocation.h>
#include <krun.h>

// system includes
#include <math.h>
#include <stdlib.h>

// local includes
#include "debug_ui.h"
#include "formwidgets.h"
#include "pageviewutils.h"
#include "pagepainter.h"
#include "core/annotations.h"
#include "annotwindow.h"
#include "guiutils.h"
#include "annotationpopup.h"
#include "pageviewannotator.h"
#include "pageviewmouseannotation.h"
#include "priorities.h"
#include "toolaction.h"
#include "okmenutitle.h"
#ifdef HAVE_SPEECH
#include "tts.h"
#endif
#include "videowidget.h"
#include "core/action.h"
#include "core/area.h"
#include "core/document_p.h"
#include "core/form.h"
#include "core/page.h"
#include "core/page_p.h"
#include "core/misc.h"
#include "core/generator.h"
#include "core/movie.h"
#include "core/audioplayer.h"
#include "core/sourcereference.h"
#include "core/tile.h"
#include "settings.h"
#include "settings_core.h"
#include "url_utils.h"
#include "magnifierview.h"

static const int pageflags = PagePainter::Accessibility | PagePainter::EnhanceLinks |
                       PagePainter::EnhanceImages | PagePainter::Highlights |
                       PagePainter::TextSelection | PagePainter::Annotations;

static const float kZoomValues[] = { 0.12, 0.25, 0.33, 0.50, 0.66, 0.75, 1.00, 1.25, 1.50, 2.00, 4.00, 8.00, 16.00 };

static inline double normClamp( double value, double def )
{
    return ( value < 0.0 || value > 1.0 ) ? def : value;
}

struct TableSelectionPart {
    PageViewItem * item;
    Okular::NormalizedRect rectInItem;
    Okular::NormalizedRect rectInSelection;

    TableSelectionPart(PageViewItem * item_p, const Okular::NormalizedRect &rectInItem_p, const Okular::NormalizedRect &rectInSelection_p);
};

TableSelectionPart::TableSelectionPart(  PageViewItem * item_p, const Okular::NormalizedRect &rectInItem_p, const Okular::NormalizedRect &rectInSelection_p)
    : item ( item_p ), rectInItem (rectInItem_p), rectInSelection (rectInSelection_p)
{
}

// structure used internally by PageView for data storage
class PageViewPrivate
{
public:
    PageViewPrivate( PageView *qq );

    FormWidgetsController* formWidgetsController();
#ifdef HAVE_SPEECH
    OkularTTS* tts();
#endif
    QString selectedText() const;

    // the document, pageviewItems and the 'visible cache'
    PageView *q;
    Okular::Document * document;
    QVector< PageViewItem * > items;
    QLinkedList< PageViewItem * > visibleItems;
    MagnifierView *magnifierView;

    // view layout (columns and continuous in Settings), zoom and mouse
    PageView::ZoomMode zoomMode;
    float zoomFactor;
    QPoint mouseGrabPos;
    QPoint mousePressPos;
    QPoint mouseSelectPos;
    int mouseMidLastY;
    bool mouseSelecting;
    QRect mouseSelectionRect;
    QColor mouseSelectionColor;
    bool mouseTextSelecting;
    QSet< int > pagesWithTextSelection;
    bool mouseOnRect;
    int mouseMode;
    MouseAnnotation * mouseAnnotation;

    // table selection
    QList<double> tableSelectionCols;
    QList<double> tableSelectionRows;
    QList<TableSelectionPart> tableSelectionParts;
    bool tableDividersGuessed;

    // viewport move
    bool viewportMoveActive;
    QTime viewportMoveTime;
    QPoint viewportMoveDest;
    int lastSourceLocationViewportPageNumber;
    double lastSourceLocationViewportNormalizedX;
    double lastSourceLocationViewportNormalizedY;
    QTimer * viewportMoveTimer;
    int controlWheelAccumulatedDelta;
    // auto scroll
    int scrollIncrement;
    QTimer * autoScrollTimer;
    // annotations
    PageViewAnnotator * annotator;
    //text annotation dialogs list
    QSet< AnnotWindow * > m_annowindows;
    // other stuff
    QTimer * delayResizeEventTimer;
    bool dirtyLayout;
    bool blockViewport;                 // prevents changes to viewport
    bool blockPixmapsRequest;           // prevent pixmap requests
    PageViewMessage * messageWindow;    // in pageviewutils.h
    bool m_formsVisible;
    FormWidgetsController *formsWidgetController;
#ifdef HAVE_SPEECH
    OkularTTS * m_tts;
#endif
    QTimer * refreshTimer;
    QSet<int> refreshPages;

    // bbox state for Trim to Selection mode
    Okular::NormalizedRect trimBoundingBox;

    // infinite resizing loop prevention
    bool verticalScrollBarVisible;
    bool horizontalScrollBarVisible;

    // drag scroll
    QPoint dragScrollVector;
    QTimer dragScrollTimer;

    // left click depress
    QTimer leftClickTimer;

    // actions
    QAction * aRotateClockwise;
    QAction * aRotateCounterClockwise;
    QAction * aRotateOriginal;
    KSelectAction * aPageSizes;
    KActionMenu * aTrimMode;
    KToggleAction * aTrimMargins;
    QAction * aMouseNormal;
    QAction * aMouseSelect;
    QAction * aMouseTextSelect;
    QAction * aMouseTableSelect;
    QAction * aMouseMagnifier;
    KToggleAction * aTrimToSelection;
    KToggleAction * aToggleAnnotator;
    KSelectAction * aZoom;
    QAction * aZoomIn;
    QAction * aZoomOut;
    KToggleAction * aZoomFitWidth;
    KToggleAction * aZoomFitPage;
    KToggleAction * aZoomAutoFit;
    KActionMenu * aViewMode;
    KToggleAction * aViewContinuous;
    QAction * aPrevAction;
    QAction * aToggleForms;
    QAction * aSpeakDoc;
    QAction * aSpeakPage;
    QAction * aSpeakStop;
    KActionCollection * actionCollection;
    QActionGroup * mouseModeActionGroup;
    QAction * aFitWindowToPage;

    int setting_viewCols;
    bool rtl_Mode;
    // Keep track of whether tablet pen is currently pressed down
    bool penDown;

    // Keep track of mouse over link object
    const Okular::ObjectRect * mouseOverLinkObject;
};

PageViewPrivate::PageViewPrivate( PageView *qq )
    : q( qq )
#ifdef HAVE_SPEECH
    , m_tts( nullptr )
#endif
{
}

FormWidgetsController* PageViewPrivate::formWidgetsController()
{
    if ( !formsWidgetController )
    {
        formsWidgetController = new FormWidgetsController( document );
        QObject::connect( formsWidgetController, SIGNAL( changed( int ) ),
                          q, SLOT( slotFormChanged( int ) ) );
        QObject::connect( formsWidgetController, SIGNAL( action( Okular::Action* ) ),
                          q, SLOT( slotAction( Okular::Action* ) ) );
    }

    return formsWidgetController;
}

#ifdef HAVE_SPEECH
OkularTTS* PageViewPrivate::tts()
{
    if ( !m_tts )
    {
        m_tts = new OkularTTS( q );
        if ( aSpeakStop )
        {
            QObject::connect( m_tts, &OkularTTS::isSpeaking,
                              aSpeakStop, &QAction::setEnabled );
        }
    }

    return m_tts;
}
#endif


/* PageView. What's in this file? -> quick overview.
 * Code weight (in rows) and meaning:
 *  160 - constructor and creating actions plus their connected slots (empty stuff)
 *  70  - DocumentObserver inherited methodes (important)
 *  550 - events: mouse, keyboard, drag
 *  170 - slotRelayoutPages: set contents of the scrollview on continuous/single modes
 *  100 - zoom: zooming pages in different ways, keeping update the toolbar actions, etc..
 *  other misc functions: only slotRequestVisiblePixmaps and pickItemOnPoint noticeable,
 * and many insignificant stuff like this comment :-)
 */
PageView::PageView( QWidget *parent, Okular::Document *document )
    : QAbstractScrollArea( parent )
    , Okular::View( QLatin1String( "PageView" ) )
{
    // create and initialize private storage structure
    d = new PageViewPrivate( this );
    d->document = document;
    d->aRotateClockwise = nullptr;
    d->aRotateCounterClockwise = nullptr;
    d->aRotateOriginal = nullptr;
    d->aViewMode = nullptr;
    d->zoomMode = PageView::ZoomFitWidth;
    d->zoomFactor = 1.0;
    d->mouseSelecting = false;
    d->mouseTextSelecting = false;
    d->mouseOnRect = false;
    d->mouseMode = Okular::Settings::mouseMode();
    d->mouseAnnotation = new MouseAnnotation( this, document );
    d->tableDividersGuessed = false;
    d->viewportMoveActive = false;
    d->lastSourceLocationViewportPageNumber = -1;
    d->lastSourceLocationViewportNormalizedX = 0.0;
    d->lastSourceLocationViewportNormalizedY = 0.0;
    d->viewportMoveTimer = nullptr;
    d->controlWheelAccumulatedDelta = 0;
    d->scrollIncrement = 0;
    d->autoScrollTimer = nullptr;
    d->annotator = nullptr;
    d->dirtyLayout = false;
    d->blockViewport = false;
    d->blockPixmapsRequest = false;
    d->messageWindow = new PageViewMessage(this);
    d->m_formsVisible = false;
    d->formsWidgetController = nullptr;
#ifdef HAVE_SPEECH
    d->m_tts = nullptr;
#endif
    d->refreshTimer = nullptr;
    d->aRotateClockwise = nullptr;
    d->aRotateCounterClockwise = nullptr;
    d->aRotateOriginal = nullptr;
    d->aPageSizes = nullptr;
    d->aTrimMode = nullptr;
    d->aTrimMargins = nullptr;
    d->aTrimToSelection = nullptr;
    d->aMouseNormal = nullptr;
    d->aMouseSelect = nullptr;
    d->aMouseTextSelect = nullptr;
    d->aToggleAnnotator = nullptr;
    d->aZoomFitWidth = nullptr;
    d->aZoomFitPage = nullptr;
    d->aZoomAutoFit = nullptr;
    d->aViewMode = nullptr;
    d->aViewContinuous = nullptr;
    d->aPrevAction = nullptr;
    d->aToggleForms = nullptr;
    d->aSpeakDoc = nullptr;
    d->aSpeakPage = nullptr;
    d->aSpeakStop = nullptr;
    d->actionCollection = nullptr;
    d->aPageSizes=nullptr;
    d->setting_viewCols = Okular::Settings::viewColumns();
    d->rtl_Mode = Okular::Settings::rtlReadingDirection();
    d->mouseModeActionGroup = nullptr;
    d->penDown = false;
    d->aMouseMagnifier = nullptr;
    d->aFitWindowToPage = nullptr;
    d->trimBoundingBox = Okular::NormalizedRect(); // Null box

    switch( Okular::Settings::zoomMode() )
    {
        case 0:
        {
            d->zoomFactor = 1;
            d->zoomMode = PageView::ZoomFixed;
            break;
        }
        case 1:
        {
            d->zoomMode = PageView::ZoomFitWidth;
            break;
        }
        case 2:
        {
            d->zoomMode = PageView::ZoomFitPage;
            break;
        }
        case 3:
        {
            d->zoomMode = PageView::ZoomFitAuto;
            break;
        }
    }

    d->delayResizeEventTimer = new QTimer( this );
    d->delayResizeEventTimer->setSingleShot( true );
    connect( d->delayResizeEventTimer, &QTimer::timeout, this, &PageView::delayedResizeEvent );

    setFrameStyle(QFrame::NoFrame);

    setAttribute( Qt::WA_StaticContents );

    setObjectName( QStringLiteral( "okular::pageView" ) );

    // viewport setup: setup focus, and track mouse
    viewport()->setFocusProxy( this );
    viewport()->setFocusPolicy( Qt::StrongFocus );
    viewport()->setAttribute( Qt::WA_OpaquePaintEvent );
    viewport()->setAttribute( Qt::WA_NoSystemBackground );
    viewport()->setMouseTracking( true );
    viewport()->setAutoFillBackground( false );
    // the apparently "magic" value of 20 is the same used internally in QScrollArea
    verticalScrollBar()->setCursor( Qt::ArrowCursor );
    verticalScrollBar()->setSingleStep( 20 );
    horizontalScrollBar()->setCursor( Qt::ArrowCursor );
    horizontalScrollBar()->setSingleStep( 20 );

    // conntect the padding of the viewport to pixmaps requests
    connect(horizontalScrollBar(), &QAbstractSlider::valueChanged, this, &PageView::slotRequestVisiblePixmaps);
    connect(verticalScrollBar(), &QAbstractSlider::valueChanged, this, &PageView::slotRequestVisiblePixmaps);
    connect( &d->dragScrollTimer, &QTimer::timeout, this, &PageView::slotDragScroll );

    d->leftClickTimer.setSingleShot( true );
    connect( &d->leftClickTimer, &QTimer::timeout, this, &PageView::slotShowSizeAllCursor );

    // set a corner button to resize the view to the page size
//    QPushButton * resizeButton = new QPushButton( viewport() );
//    resizeButton->setPixmap( SmallIcon("crop") );
//    setCornerWidget( resizeButton );
//    resizeButton->setEnabled( false );
    // connect(...);
    setAttribute( Qt::WA_InputMethodEnabled, true );

    // Grab pinch gestures to zoom and rotate the view
    grabGesture(Qt::PinchGesture);

    d->magnifierView = new MagnifierView(document, this);
    d->magnifierView->hide();
    d->magnifierView->setGeometry(0, 0, 351, 201); // TODO: more dynamic?

    connect(document, &Okular::Document::processMovieAction, this, &PageView::slotProcessMovieAction);
    connect(document, &Okular::Document::processRenditionAction, this, &PageView::slotProcessRenditionAction);

    // schedule the welcome message
    QMetaObject::invokeMethod(this, "slotShowWelcome", Qt::QueuedConnection);
}

PageView::~PageView()
{
#ifdef HAVE_SPEECH
    if ( d->m_tts )
        d->m_tts->stopAllSpeechs();
#endif

    delete d->mouseAnnotation;

    // delete the local storage structure

    // We need to assign it to a different list otherwise slotAnnotationWindowDestroyed
    // will bite us and clear d->m_annowindows
    QSet< AnnotWindow * > annowindows = d->m_annowindows;
    d->m_annowindows.clear();
    qDeleteAll( annowindows );

    // delete all widgets
    QVector< PageViewItem * >::const_iterator dIt = d->items.constBegin(), dEnd = d->items.constEnd();
    for ( ; dIt != dEnd; ++dIt )
        delete *dIt;
    delete d->formsWidgetController;
    d->document->removeObserver( this );
    delete d;
}

void PageView::setupBaseActions( KActionCollection * ac )
{
    d->actionCollection = ac;

    // Zoom actions ( higher scales takes lots of memory! )
    d->aZoom  = new KSelectAction(QIcon::fromTheme( QStringLiteral("page-zoom") ), i18n("Zoom"), this);
    ac->addAction(QStringLiteral("zoom_to"), d->aZoom );
    d->aZoom->setEditable( true );
    d->aZoom->setMaxComboViewCount( 14 );
    connect( d->aZoom, SIGNAL(triggered(QAction*)), this, SLOT(slotZoom()) );
    updateZoomText();

    d->aZoomIn = KStandardAction::zoomIn( this, SLOT(slotZoomIn()), ac );

    d->aZoomOut = KStandardAction::zoomOut( this, SLOT(slotZoomOut()), ac );
}

void PageView::setupViewerActions( KActionCollection * ac )
{
    d->actionCollection = ac;

    ac->setDefaultShortcut(d->aZoomIn, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Plus));
    ac->setDefaultShortcut(d->aZoomOut, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Minus));

    // orientation menu actions
    d->aRotateClockwise = new QAction( QIcon::fromTheme( QStringLiteral("object-rotate-right") ), i18n( "Rotate &Right" ), this );
    d->aRotateClockwise->setIconText( i18nc( "Rotate right", "Right" ) );
    ac->addAction( QStringLiteral("view_orientation_rotate_cw"), d->aRotateClockwise );
    d->aRotateClockwise->setEnabled( false );
    connect( d->aRotateClockwise, &QAction::triggered, this, &PageView::slotRotateClockwise );
    d->aRotateCounterClockwise = new QAction( QIcon::fromTheme( QStringLiteral("object-rotate-left") ), i18n( "Rotate &Left" ), this );
    d->aRotateCounterClockwise->setIconText( i18nc( "Rotate left", "Left" ) );
    ac->addAction( QStringLiteral("view_orientation_rotate_ccw"), d->aRotateCounterClockwise );
    d->aRotateCounterClockwise->setEnabled( false );
    connect( d->aRotateCounterClockwise, &QAction::triggered, this, &PageView::slotRotateCounterClockwise );
    d->aRotateOriginal = new QAction( i18n( "Original Orientation" ), this );
    ac->addAction( QStringLiteral("view_orientation_original"), d->aRotateOriginal );
    d->aRotateOriginal->setEnabled( false );
    connect( d->aRotateOriginal, &QAction::triggered, this, &PageView::slotRotateOriginal );

    d->aPageSizes = new KSelectAction(i18n("&Page Size"), this);
    ac->addAction(QStringLiteral("view_pagesizes"), d->aPageSizes);
    d->aPageSizes->setEnabled( false );

    connect( d->aPageSizes , SIGNAL(triggered(int)),
         this, SLOT(slotPageSizes(int)) );

    // Trim View actions
    d->aTrimMode = new KActionMenu(i18n( "&Trim View" ), this );
    d->aTrimMode->setDelayed( false );
    ac->addAction(QStringLiteral("view_trim_mode"), d->aTrimMode );

    d->aTrimMargins  = new KToggleAction( i18n( "&Trim Margins" ), d->aTrimMode->menu() );
    d->aTrimMode->addAction( d->aTrimMargins  );
    ac->addAction( QStringLiteral("view_trim_margins"), d->aTrimMargins  );
    d->aTrimMargins->setData( qVariantFromValue( (int)Okular::Settings::EnumTrimMode::Margins ) );
    connect( d->aTrimMargins, &QAction::toggled, this, &PageView::slotTrimMarginsToggled );
    d->aTrimMargins->setChecked( Okular::Settings::trimMargins() );

    d->aTrimToSelection  = new KToggleAction( i18n( "Trim To &Selection" ), d->aTrimMode->menu() );
    d->aTrimMode->addAction( d->aTrimToSelection);
    ac->addAction( QStringLiteral("view_trim_selection"), d->aTrimToSelection);
    d->aTrimToSelection->setData( qVariantFromValue( (int)Okular::Settings::EnumTrimMode::Selection ) );
    connect( d->aTrimToSelection, &QAction::toggled, this, &PageView::slotTrimToSelectionToggled );

    d->aZoomFitWidth  = new KToggleAction(QIcon::fromTheme( QStringLiteral("zoom-fit-width") ), i18n("Fit &Width"), this);
    ac->addAction(QStringLiteral("view_fit_to_width"), d->aZoomFitWidth );
    connect( d->aZoomFitWidth, &QAction::toggled, this, &PageView::slotFitToWidthToggled );

    d->aZoomFitPage  = new KToggleAction(QIcon::fromTheme( QStringLiteral("zoom-fit-best") ), i18n("Fit &Page"), this);
    ac->addAction(QStringLiteral("view_fit_to_page"), d->aZoomFitPage );
    connect( d->aZoomFitPage, &QAction::toggled, this, &PageView::slotFitToPageToggled );

    d->aZoomAutoFit  = new KToggleAction(QIcon::fromTheme( QStringLiteral("zoom-fit-best") ), i18n("&Auto Fit"), this);
    ac->addAction(QStringLiteral("view_auto_fit"), d->aZoomAutoFit );
    connect( d->aZoomAutoFit, &QAction::toggled, this, &PageView::slotAutoFitToggled );

    d->aFitWindowToPage = new QAction(QIcon::fromTheme( QStringLiteral("zoom-fit-width") ), i18n("Fit Wi&ndow to Page"), this);
    d->aFitWindowToPage->setEnabled( Okular::Settings::viewMode() == (int)Okular::Settings::EnumViewMode::Single );
    ac->setDefaultShortcut(d->aFitWindowToPage, QKeySequence(Qt::CTRL + Qt::Key_J) );
    ac->addAction( QStringLiteral("fit_window_to_page"), d->aFitWindowToPage );
    connect( d->aFitWindowToPage, &QAction::triggered, this, &PageView::slotFitWindowToPage );

    // View-Layout actions
    d->aViewMode = new KActionMenu( QIcon::fromTheme( QStringLiteral("view-split-left-right") ), i18n( "&View Mode" ), this );
    d->aViewMode->setDelayed( false );
#define ADD_VIEWMODE_ACTION( text, name, id ) \
do { \
    QAction *vm = new QAction( text, this ); \
    vm->setCheckable( true ); \
    vm->setData( qVariantFromValue( id ) ); \
    d->aViewMode->addAction( vm ); \
    ac->addAction( QStringLiteral(name), vm ); \
    vmGroup->addAction( vm ); \
} while( 0 )
    ac->addAction(QStringLiteral("view_render_mode"), d->aViewMode );
    QActionGroup *vmGroup = new QActionGroup( this ); //d->aViewMode->menu() );
    ADD_VIEWMODE_ACTION( i18n( "Single Page" ), "view_render_mode_single", (int)Okular::Settings::EnumViewMode::Single );
    ADD_VIEWMODE_ACTION( i18n( "Facing Pages" ), "view_render_mode_facing", (int)Okular::Settings::EnumViewMode::Facing );
    ADD_VIEWMODE_ACTION( i18n( "Facing Pages (Center First Page)" ), "view_render_mode_facing_center_first", (int)Okular::Settings::EnumViewMode::FacingFirstCentered );
    ADD_VIEWMODE_ACTION( i18n( "Overview" ), "view_render_mode_overview", (int)Okular::Settings::EnumViewMode::Summary );
    const QList<QAction *> viewModeActions = d->aViewMode->menu()->actions();
    foreach(QAction *viewModeAction, viewModeActions)
    {
        if (viewModeAction->data().toInt() == Okular::Settings::viewMode())
        {
            viewModeAction->setChecked( true );
        }
    }
    connect( vmGroup, &QActionGroup::triggered, this, &PageView::slotViewMode );
#undef ADD_VIEWMODE_ACTION

    d->aViewContinuous  = new KToggleAction(QIcon::fromTheme( QStringLiteral("view-list-text") ), i18n("&Continuous"), this);
    ac->addAction(QStringLiteral("view_continuous"), d->aViewContinuous );
    connect( d->aViewContinuous, &QAction::toggled, this, &PageView::slotContinuousToggled );
    d->aViewContinuous->setChecked( Okular::Settings::viewContinuous() );

    // Mouse mode actions for viewer mode
    d->mouseModeActionGroup = new QActionGroup( this );
    d->mouseModeActionGroup->setExclusive( true );
    d->aMouseNormal  = new QAction( QIcon::fromTheme( QStringLiteral("input-mouse") ), i18n( "&Browse Tool" ), this );
    ac->addAction(QStringLiteral("mouse_drag"), d->aMouseNormal );
    connect( d->aMouseNormal, &QAction::triggered, this, &PageView::slotSetMouseNormal );
    d->aMouseNormal->setIconText( i18nc( "Browse Tool", "Browse" ) );
    d->aMouseNormal->setCheckable( true );
    ac->setDefaultShortcut(d->aMouseNormal, QKeySequence(Qt::CTRL + Qt::Key_1));
    d->aMouseNormal->setActionGroup( d->mouseModeActionGroup );
    d->aMouseNormal->setChecked( Okular::Settings::mouseMode() == Okular::Settings::EnumMouseMode::Browse );

    QAction * mz  = new QAction(QIcon::fromTheme( QStringLiteral("page-zoom") ), i18n("&Zoom Tool"), this);
    ac->addAction(QStringLiteral("mouse_zoom"), mz );
    connect( mz, &QAction::triggered, this, &PageView::slotSetMouseZoom );
    mz->setIconText( i18nc( "Zoom Tool", "Zoom" ) );
    mz->setCheckable( true );
    ac->setDefaultShortcut(mz, QKeySequence(Qt::CTRL + Qt::Key_2));
    mz->setActionGroup( d->mouseModeActionGroup );
    mz->setChecked( Okular::Settings::mouseMode() == Okular::Settings::EnumMouseMode::Zoom );

    QAction * aToggleChangeColors  = new QAction(i18n("&Toggle Change Colors"), this);
    ac->addAction(QStringLiteral("toggle_change_colors"), aToggleChangeColors );
    connect( aToggleChangeColors, &QAction::triggered, this, &PageView::slotToggleChangeColors );
}

// WARNING: 'setupViewerActions' must have been called before this method
void PageView::setupActions( KActionCollection * ac )
{
    d->actionCollection = ac;

    ac->setDefaultShortcuts(d->aZoomIn, KStandardShortcut::zoomIn());
    ac->setDefaultShortcuts(d->aZoomOut, KStandardShortcut::zoomOut());

    // Mouse-Mode actions
    d->aMouseSelect  = new QAction(QIcon::fromTheme( QStringLiteral("select-rectangular") ), i18n("&Selection Tool"), this);
    ac->addAction(QStringLiteral("mouse_select"), d->aMouseSelect );
    connect( d->aMouseSelect, &QAction::triggered, this, &PageView::slotSetMouseSelect );
    d->aMouseSelect->setIconText( i18nc( "Select Tool", "Selection" ) );
    d->aMouseSelect->setCheckable( true );
    ac->setDefaultShortcut(d->aMouseSelect, Qt::CTRL + Qt::Key_3);

    d->aMouseSelect->setActionGroup( d->mouseModeActionGroup );
    d->aMouseSelect->setChecked( Okular::Settings::mouseMode() == Okular::Settings::EnumMouseMode::RectSelect );

    d->aMouseTextSelect  = new QAction(QIcon::fromTheme( QStringLiteral("draw-text") ), i18n("&Text Selection Tool"), this);
    ac->addAction(QStringLiteral("mouse_textselect"), d->aMouseTextSelect );
    connect( d->aMouseTextSelect, &QAction::triggered, this, &PageView::slotSetMouseTextSelect );
    d->aMouseTextSelect->setIconText( i18nc( "Text Selection Tool", "Text Selection" ) );
    d->aMouseTextSelect->setCheckable( true );
    ac->setDefaultShortcut(d->aMouseTextSelect, Qt::CTRL + Qt::Key_4);
    d->aMouseTextSelect->setActionGroup( d->mouseModeActionGroup );
    d->aMouseTextSelect->setChecked( Okular::Settings::mouseMode() == Okular::Settings::EnumMouseMode::TextSelect );

    d->aMouseTableSelect  = new QAction(QIcon::fromTheme( QStringLiteral("table") ), i18n("T&able Selection Tool"), this);
    ac->addAction(QStringLiteral("mouse_tableselect"), d->aMouseTableSelect );
    connect( d->aMouseTableSelect, &QAction::triggered, this, &PageView::slotSetMouseTableSelect );
    d->aMouseTableSelect->setIconText( i18nc( "Table Selection Tool", "Table Selection" ) );
    d->aMouseTableSelect->setCheckable( true );
    ac->setDefaultShortcut(d->aMouseTableSelect, Qt::CTRL + Qt::Key_5);
    d->aMouseTableSelect->setActionGroup( d->mouseModeActionGroup );
    d->aMouseTableSelect->setChecked( Okular::Settings::mouseMode() == Okular::Settings::EnumMouseMode::TableSelect );

    d->aMouseMagnifier = new QAction(QIcon::fromTheme( QStringLiteral("document-preview") ), i18n("&Magnifier"), this);
    ac->addAction(QStringLiteral("mouse_magnifier"), d->aMouseMagnifier );
    connect( d->aMouseMagnifier, &QAction::triggered, this, &PageView::slotSetMouseMagnifier );
    d->aMouseMagnifier->setIconText( i18nc( "Magnifier Tool", "Magnifier" ) );
    d->aMouseMagnifier->setCheckable( true );
    ac->setDefaultShortcut(d->aMouseMagnifier, Qt::CTRL + Qt::Key_6);
    d->aMouseMagnifier->setActionGroup( d->mouseModeActionGroup );
    d->aMouseMagnifier->setChecked( Okular::Settings::mouseMode() == Okular::Settings::EnumMouseMode::Magnifier );

    d->aToggleAnnotator  = new KToggleAction(QIcon::fromTheme( QStringLiteral("draw-freehand") ), i18n("&Review"), this);
    ac->addAction(QStringLiteral("mouse_toggle_annotate"), d->aToggleAnnotator );
    d->aToggleAnnotator->setCheckable( true );
    connect( d->aToggleAnnotator, &QAction::toggled, this, &PageView::slotToggleAnnotator );
    ac->setDefaultShortcut(d->aToggleAnnotator, Qt::Key_F6);

    ToolAction *ta = new ToolAction( this );
    ac->addAction( QStringLiteral("mouse_selecttools"), ta );
    ta->addAction( d->aMouseSelect );
    ta->addAction( d->aMouseTextSelect );
    ta->addAction( d->aMouseTableSelect );

    // speak actions
#ifdef HAVE_SPEECH
    d->aSpeakDoc = new QAction( QIcon::fromTheme( QStringLiteral("text-speak") ), i18n( "Speak Whole Document" ), this );
    ac->addAction( QStringLiteral("speak_document"), d->aSpeakDoc );
    d->aSpeakDoc->setEnabled( false );
    connect( d->aSpeakDoc, &QAction::triggered, this, &PageView::slotSpeakDocument );

    d->aSpeakPage = new QAction( QIcon::fromTheme( QStringLiteral("text-speak") ), i18n( "Speak Current Page" ), this );
    ac->addAction( QStringLiteral("speak_current_page"), d->aSpeakPage );
    d->aSpeakPage->setEnabled( false );
    connect( d->aSpeakPage, &QAction::triggered, this, &PageView::slotSpeakCurrentPage );

    d->aSpeakStop = new QAction( QIcon::fromTheme( QStringLiteral("media-playback-stop") ), i18n( "Stop Speaking" ), this );
    ac->addAction( QStringLiteral("speak_stop_all"), d->aSpeakStop );
    d->aSpeakStop->setEnabled( false );
    connect( d->aSpeakStop, &QAction::triggered, this, &PageView::slotStopSpeaks );
#else
    d->aSpeakDoc = 0;
    d->aSpeakPage = 0;
    d->aSpeakStop = 0;
#endif

    // Other actions
    QAction * su  = new QAction(i18n("Scroll Up"), this);
    ac->addAction(QStringLiteral("view_scroll_up"), su );
    connect( su, &QAction::triggered, this, &PageView::slotAutoScrollUp );
    ac->setDefaultShortcut(su, QKeySequence(Qt::SHIFT + Qt::Key_Up));
    addAction(su);

    QAction * sd  = new QAction(i18n("Scroll Down"), this);
    ac->addAction(QStringLiteral("view_scroll_down"), sd );
    connect( sd, &QAction::triggered, this, &PageView::slotAutoScrollDown );
    ac->setDefaultShortcut(sd, QKeySequence(Qt::SHIFT + Qt::Key_Down));
    addAction(sd);

    QAction * spu = new QAction(i18n("Scroll Page Up"), this);
    ac->addAction( QStringLiteral("view_scroll_page_up"), spu );
    connect( spu, &QAction::triggered, this, &PageView::slotScrollUp );
    ac->setDefaultShortcut(spu, QKeySequence(Qt::SHIFT + Qt::Key_Space));
    addAction( spu );

    QAction * spd = new QAction(i18n("Scroll Page Down"), this);
    ac->addAction( QStringLiteral("view_scroll_page_down"), spd );
    connect( spd, &QAction::triggered, this, &PageView::slotScrollDown );
    ac->setDefaultShortcut(spd, QKeySequence(Qt::Key_Space));
    addAction( spd );

    d->aToggleForms = new QAction( this );
    ac->addAction( QStringLiteral("view_toggle_forms"), d->aToggleForms );
    connect( d->aToggleForms, &QAction::triggered, this, &PageView::slotToggleForms );
    d->aToggleForms->setEnabled( false );
    toggleFormWidgets( false );

    // Setup undo and redo actions
    QAction *kundo = KStandardAction::create( KStandardAction::Undo, d->document, SLOT(undo()), ac );
    QAction *kredo = KStandardAction::create( KStandardAction::Redo, d->document, SLOT(redo()), ac );
    connect(d->document, &Okular::Document::canUndoChanged, kundo, &QAction::setEnabled);
    connect(d->document, &Okular::Document::canRedoChanged, kredo, &QAction::setEnabled);
    kundo->setEnabled(false);
    kredo->setEnabled(false);
}

bool PageView::canFitPageWidth() const
{
    return Okular::Settings::viewMode() != Okular::Settings::EnumViewMode::Single || d->zoomMode != ZoomFitWidth;
}

void PageView::fitPageWidth( int page )
{
    // zoom: Fit Width, columns: 1. setActions + relayout + setPage + update
    d->zoomMode = ZoomFitWidth;
    Okular::Settings::setViewMode( 0 );
    d->aZoomFitWidth->setChecked( true );
    d->aZoomFitPage->setChecked( false );
    d->aZoomAutoFit->setChecked( false );
    d->aViewMode->menu()->actions().at( 0 )->setChecked( true );
    viewport()->setUpdatesEnabled( false );
    slotRelayoutPages();
    viewport()->setUpdatesEnabled( true );
    d->document->setViewportPage( page );
    updateZoomText();
    setFocus();
}

void PageView::openAnnotationWindow( Okular::Annotation * annotation, int pageNumber )
{
    if ( !annotation )
        return;

    // find the annot window
    AnnotWindow* existWindow = nullptr;
    foreach(AnnotWindow *aw, d->m_annowindows)
    {
        if ( aw->annotation() == annotation )
        {
            existWindow = aw;
            break;
        }
    }

    if ( existWindow == nullptr )
    {
        existWindow = new AnnotWindow( this, annotation, d->document, pageNumber );
        connect(existWindow, &QObject::destroyed, this, &PageView::slotAnnotationWindowDestroyed);

        d->m_annowindows << existWindow;
    } else {
        existWindow->raise();
        existWindow->findChild<KTextEdit *>()->setFocus();
    }

    existWindow->show();
}

void PageView::slotAnnotationWindowDestroyed( QObject * window )
{
    d->m_annowindows.remove( static_cast<AnnotWindow*>( window ) );
}

void PageView::displayMessage( const QString & message, const QString & details, PageViewMessage::Icon icon, int duration )
{
    if ( !Okular::Settings::showOSD() )
    {
        if (icon == PageViewMessage::Error)
        {
            if ( !details.isEmpty() )
                KMessageBox::detailedError( this, message, details );
            else
                KMessageBox::error( this, message );
        }
        return;
    }

    // hide messageWindow if string is empty
    if ( message.isEmpty() )
        return d->messageWindow->hide();

    // display message (duration is length dependant)
    if (duration==-1)
    {
        duration = 500 + 100 * message.length();
        if ( !details.isEmpty() )
            duration += 500 + 100 * details.length();
    }
    d->messageWindow->display( message, details, icon, duration );
}

void PageView::reparseConfig()
{
    // set the scroll bars policies
    Qt::ScrollBarPolicy scrollBarMode = Okular::Settings::showScrollBars() ?
        Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff;
    if ( horizontalScrollBarPolicy() != scrollBarMode )
    {
        setHorizontalScrollBarPolicy( scrollBarMode );
        setVerticalScrollBarPolicy( scrollBarMode );
    }

    if ( Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::Summary &&
         ( (int)Okular::Settings::viewColumns() != d->setting_viewCols ) )
    {
        d->setting_viewCols = Okular::Settings::viewColumns();

        slotRelayoutPages();
    }

    if (Okular::Settings::rtlReadingDirection() != d->rtl_Mode )
    {
        d->rtl_Mode = Okular::Settings::rtlReadingDirection();
        slotRelayoutPages();
    }

    updatePageStep();

    if ( d->annotator )
    {
        d->annotator->setEnabled( false );
        d->annotator->reparseConfig();
        if ( d->aToggleAnnotator->isChecked() )
            slotToggleAnnotator( true );
    }

    // Something like invert colors may have changed
    // As we don't have a way to find out the old value
    // We just update the viewport, this shouldn't be that bad
    // since it's just a repaint of pixmaps we already have
    viewport()->update();
}

KActionCollection *PageView::actionCollection() const
{
    return d->actionCollection;
}

QAction *PageView::toggleFormsAction() const
{
    return d->aToggleForms;
}

int PageView::contentAreaWidth() const
{
    return horizontalScrollBar()->maximum() + viewport()->width();
}

int PageView::contentAreaHeight() const
{
    return verticalScrollBar()->maximum() + viewport()->height();
}

QPoint PageView::contentAreaPosition() const
{
    return QPoint( horizontalScrollBar()->value(), verticalScrollBar()->value() );
}

QPoint PageView::contentAreaPoint( const QPoint & pos ) const
{
    return pos + contentAreaPosition();
}

QPointF PageView::contentAreaPoint( const QPointF & pos ) const
{
    return pos + contentAreaPosition();
}

QString PageViewPrivate::selectedText() const
{
    if ( pagesWithTextSelection.isEmpty() )
        return QString();

    QString text;
    QList< int > selpages = pagesWithTextSelection.toList();
    qSort( selpages );
    const Okular::Page * pg = nullptr;
    if ( selpages.count() == 1 )
    {
        pg = document->page( selpages.first() );
        text.append( pg->text( pg->textSelection(), Okular::TextPage::CentralPixelTextAreaInclusionBehaviour ) );
    }
    else
    {
        pg = document->page( selpages.first() );
        text.append( pg->text( pg->textSelection(), Okular::TextPage::CentralPixelTextAreaInclusionBehaviour ) );
        int end = selpages.count() - 1;
        for( int i = 1; i < end; ++i )
        {
            pg = document->page( selpages.at( i ) );
            text.append( pg->text( nullptr, Okular::TextPage::CentralPixelTextAreaInclusionBehaviour ) );
        }
        pg = document->page( selpages.last() );
        text.append( pg->text( pg->textSelection(), Okular::TextPage::CentralPixelTextAreaInclusionBehaviour ) );
    }
    return text;
}

void PageView::copyTextSelection() const
{
    const QString text = d->selectedText();
    if ( !text.isEmpty() )
    {
        QClipboard *cb = QApplication::clipboard();
        cb->setText( text, QClipboard::Clipboard );
    }
}

void PageView::selectAll()
{
    QVector< PageViewItem * >::const_iterator it = d->items.constBegin(), itEnd = d->items.constEnd();
    for ( ; it < itEnd; ++it )
    {
        Okular::RegularAreaRect * area = textSelectionForItem( *it );
        d->pagesWithTextSelection.insert( (*it)->pageNumber() );
        d->document->setPageTextSelection( (*it)->pageNumber(), area, palette().color( QPalette::Active, QPalette::Highlight ) );
    }
}

void PageView::createAnnotationsVideoWidgets(PageViewItem *item, const QLinkedList< Okular::Annotation * > &annotations)
{
    qDeleteAll( item->videoWidgets() );
    item->videoWidgets().clear();

    QLinkedList< Okular::Annotation * >::const_iterator aIt = annotations.constBegin(), aEnd = annotations.constEnd();
    for ( ; aIt != aEnd; ++aIt )
    {
        Okular::Annotation * a = *aIt;
        if ( a->subType() == Okular::Annotation::AMovie )
        {
            Okular::MovieAnnotation * movieAnn = static_cast< Okular::MovieAnnotation * >( a );
            VideoWidget * vw = new VideoWidget( movieAnn, movieAnn->movie(), d->document, viewport() );
            item->videoWidgets().insert( movieAnn->movie(), vw );
            vw->pageInitialized();
        }
        else if ( a->subType() == Okular::Annotation::ARichMedia )
        {
            Okular::RichMediaAnnotation * richMediaAnn = static_cast< Okular::RichMediaAnnotation * >( a );
            VideoWidget * vw = new VideoWidget( richMediaAnn, richMediaAnn->movie(), d->document, viewport() );
            item->videoWidgets().insert( richMediaAnn->movie(), vw );
            vw->pageInitialized();
        }
        else if ( a->subType() == Okular::Annotation::AScreen )
        {
            const Okular::ScreenAnnotation * screenAnn = static_cast< Okular::ScreenAnnotation * >( a );
            Okular::Movie *movie = GuiUtils::renditionMovieFromScreenAnnotation( screenAnn );
            if ( movie )
            {
                VideoWidget * vw = new VideoWidget( screenAnn, movie, d->document, viewport() );
                item->videoWidgets().insert( movie, vw );
                vw->pageInitialized();
            }
        }
    }
}

//BEGIN DocumentObserver inherited methods
void PageView::notifySetup( const QVector< Okular::Page * > & pageSet, int setupFlags )
{
    bool documentChanged = setupFlags & Okular::DocumentObserver::DocumentChanged;
    const bool allownotes = d->document->isAllowed( Okular::AllowNotes );
    const bool allowfillforms = d->document->isAllowed( Okular::AllowFillForms );

    // allownotes may have changed
    if ( d->aToggleAnnotator )
        d->aToggleAnnotator->setEnabled( allownotes );

    // reuse current pages if nothing new
    if ( ( pageSet.count() == d->items.count() ) && !documentChanged && !( setupFlags & Okular::DocumentObserver::NewLayoutForPages ) )
    {
        int count = pageSet.count();
        for ( int i = 0; (i < count) && !documentChanged; i++ )
        {
            if ( (int)pageSet[i]->number() != d->items[i]->pageNumber() )
            {
                documentChanged = true;
            }
            else
            {
                // even if the document has not changed, allowfillforms may have
                // changed, so update all fields' "canBeFilled" flag
                foreach ( FormWidgetIface * w, d->items[i]->formWidgets() )
                    w->setCanBeFilled( allowfillforms );
            }
        }

        if ( !documentChanged )
        {
            if ( setupFlags & Okular::DocumentObserver::UrlChanged )
            {
                // Here with UrlChanged and no document changed it means we
                // need to update all the Annotation* and Form* otherwise
                // they still point to the old document ones, luckily the old ones are still
                // around so we can look for the new ones using unique ids, etc
                d->mouseAnnotation->updateAnnotationPointers();

                foreach(AnnotWindow *aw, d->m_annowindows)
                {
                    Okular::Annotation *newA = d->document->page( aw->pageNumber() )->annotation( aw->annotation()->uniqueName() );
                    aw->updateAnnotation( newA );
                }

                const QRect viewportRect( horizontalScrollBar()->value(), verticalScrollBar()->value(),
                                          viewport()->width(), viewport()->height() );
                for ( int i = 0; i < count; i++ )
                {
                    PageViewItem *item = d->items[i];
                    const QSet<FormWidgetIface*> fws = item->formWidgets();
                    foreach ( FormWidgetIface * w, fws )
                    {
                        Okular::FormField *f = Okular::PagePrivate::findEquivalentForm( d->document->page( i ), w->formField() );
                        if (f)
                        {
                            w->setFormField( f );
                        }
                        else
                        {
                            qWarning() << "Lost form field on document save, something is wrong";
                            item->formWidgets().remove(w);
                            delete w;
                        }
                    }

                    // For the video widgets we don't really care about reusing them since they don't contain much info so just
                    // create them again
                    createAnnotationsVideoWidgets( item, pageSet[i]->annotations() );
                    Q_FOREACH ( VideoWidget *vw, item->videoWidgets() )
                    {
                        const Okular::NormalizedRect r = vw->normGeometry();
                        vw->setGeometry(
                            qRound( item->uncroppedGeometry().left() + item->uncroppedWidth() * r.left ) + 1 - viewportRect.left(),
                            qRound( item->uncroppedGeometry().top() + item->uncroppedHeight() * r.top ) + 1 - viewportRect.top(),
                            qRound( fabs( r.right - r.left ) * item->uncroppedGeometry().width() ),
                            qRound( fabs( r.bottom - r.top ) * item->uncroppedGeometry().height() ) );

                        // Workaround, otherwise the size somehow gets lost
                        vw->show();
                        vw->hide();
                    }

                }
            }

            return;
        }
    }

    // mouseAnnotation must not access our PageViewItem widgets any longer
    d->mouseAnnotation->reset();

    // delete all widgets (one for each page in pageSet)
    QVector< PageViewItem * >::const_iterator dIt = d->items.constBegin(), dEnd = d->items.constEnd();
    for ( ; dIt != dEnd; ++dIt )
        delete *dIt;
    d->items.clear();
    d->visibleItems.clear();
    d->pagesWithTextSelection.clear();
    toggleFormWidgets( false );
    if ( d->formsWidgetController )
        d->formsWidgetController->dropRadioButtons();

    bool haspages = !pageSet.isEmpty();
    bool hasformwidgets = false;
    // create children widgets
    QVector< Okular::Page * >::const_iterator setIt = pageSet.constBegin(), setEnd = pageSet.constEnd();
    for ( ; setIt != setEnd; ++setIt )
    {
        PageViewItem * item = new PageViewItem( *setIt );
        d->items.push_back( item );
#ifdef PAGEVIEW_DEBUG
        qCDebug(OkularUiDebug).nospace() << "cropped geom for " << d->items.last()->pageNumber() << " is " << d->items.last()->croppedGeometry();
#endif
        const QLinkedList< Okular::FormField * > pageFields = (*setIt)->formFields();
        QLinkedList< Okular::FormField * >::const_iterator ffIt = pageFields.constBegin(), ffEnd = pageFields.constEnd();
        for ( ; ffIt != ffEnd; ++ffIt )
        {
            Okular::FormField * ff = *ffIt;
            FormWidgetIface * w = FormWidgetFactory::createWidget( ff, viewport() );
            if ( w )
            {
                w->setPageItem( item );
                w->setFormWidgetsController( d->formWidgetsController() );
                w->setVisibility( false );
                w->setCanBeFilled( allowfillforms );
                item->formWidgets().insert( w );
                hasformwidgets = true;
            }
        }

        createAnnotationsVideoWidgets( item, (*setIt)->annotations() );
    }

    // invalidate layout so relayout/repaint will happen on next viewport change
    if ( haspages )
    {
        // We do a delayed call to slotRelayoutPages but also set the dirtyLayout
        // because we might end up in notifyViewportChanged while slotRelayoutPages
        // has not been done and we don't want that to happen
        d->dirtyLayout = true;
        QMetaObject::invokeMethod(this, "slotRelayoutPages", Qt::QueuedConnection);
    }
    else
    {
        // update the mouse cursor when closing because we may have close through a link and
        // want the cursor to come back to the normal cursor
        updateCursor();
        // then, make the message window and scrollbars disappear, and trigger a repaint
        d->messageWindow->hide();
        resizeContentArea( QSize( 0,0 ) );
        viewport()->update(); // when there is no change to the scrollbars, no repaint would
                              // be done and the old document would still be shown
    }

    // OSD to display pages
    if ( documentChanged && pageSet.count() > 0 && Okular::Settings::showOSD() )
        d->messageWindow->display(
            i18np(" Loaded a one-page document.",
                 " Loaded a %1-page document.",
                 pageSet.count() ),
            QString(),
            PageViewMessage::Info, 4000 );

    updateActionState( haspages, documentChanged, hasformwidgets );

    // We need to assign it to a different list otherwise slotAnnotationWindowDestroyed
    // will bite us and clear d->m_annowindows
    QSet< AnnotWindow * > annowindows = d->m_annowindows;
    d->m_annowindows.clear();
    qDeleteAll( annowindows );

    selectionClear();
}

void PageView::updateActionState( bool haspages, bool documentChanged, bool hasformwidgets )
{
    if ( d->aPageSizes )
    { // may be null if dummy mode is on
        bool pageSizes = d->document->supportsPageSizes();
        d->aPageSizes->setEnabled( pageSizes );
        // set the new page sizes:
        // - if the generator supports them
        // - if the document changed
        if ( pageSizes && documentChanged )
        {
            QStringList items;
            foreach ( const Okular::PageSize &p, d->document->pageSizes() )
                items.append( p.name() );
            d->aPageSizes->setItems( items );
        }
    }

    if ( d->aTrimMargins )
        d->aTrimMargins->setEnabled( haspages );

    if ( d->aTrimToSelection )
        d->aTrimToSelection->setEnabled( haspages );

    if ( d->aViewMode )
        d->aViewMode->setEnabled( haspages );

    if ( d->aViewContinuous )
        d->aViewContinuous->setEnabled( haspages );

    if ( d->aZoomFitWidth )
        d->aZoomFitWidth->setEnabled( haspages );
    if ( d->aZoomFitPage )
        d->aZoomFitPage->setEnabled( haspages );
    if ( d->aZoomAutoFit )
        d->aZoomAutoFit->setEnabled( haspages );

    if ( d->aZoom )
    {
        d->aZoom->selectableActionGroup()->setEnabled( haspages );
        d->aZoom->setEnabled( haspages );
    }
    if ( d->aZoomIn )
        d->aZoomIn->setEnabled( haspages );
    if ( d->aZoomOut )
        d->aZoomOut->setEnabled( haspages );

    if ( d->mouseModeActionGroup )
        d->mouseModeActionGroup->setEnabled( haspages );

    if ( d->aRotateClockwise )
        d->aRotateClockwise->setEnabled( haspages );
    if ( d->aRotateCounterClockwise )
        d->aRotateCounterClockwise->setEnabled( haspages );
    if ( d->aRotateOriginal )
        d->aRotateOriginal->setEnabled( haspages );
    if ( d->aToggleForms )
    { // may be null if dummy mode is on
        d->aToggleForms->setEnabled( haspages && hasformwidgets );
    }
    bool allowAnnotations = d->document->isAllowed( Okular::AllowNotes );
    if ( d->annotator )
    {
        bool allowTools = haspages && allowAnnotations;
        d->annotator->setToolsEnabled( allowTools );
        d->annotator->setTextToolsEnabled( allowTools && d->document->supportsSearching() );
    }
    if ( d->aToggleAnnotator )
    {
        if ( !allowAnnotations && d->aToggleAnnotator->isChecked() )
        {
            d->aToggleAnnotator->trigger();
        }
        d->aToggleAnnotator->setEnabled( allowAnnotations );
    }
#ifdef HAVE_SPEECH
    if ( d->aSpeakDoc )
    {
        const bool enablettsactions = haspages ? Okular::Settings::useTTS() : false;
        d->aSpeakDoc->setEnabled( enablettsactions );
        d->aSpeakPage->setEnabled( enablettsactions );
    }
#endif
    if (d->aMouseMagnifier)
        d->aMouseMagnifier->setEnabled(d->document->supportsTiles());
    if ( d->aFitWindowToPage )
        d->aFitWindowToPage->setEnabled( haspages && !Okular::Settings::viewContinuous() );
}

bool PageView::areSourceLocationsShownGraphically() const
{
    return Okular::Settings::showSourceLocationsGraphically();
}

void PageView::setShowSourceLocationsGraphically(bool show)
{
    if( show == Okular::Settings::showSourceLocationsGraphically() )
    {
        return;
    }
    Okular::Settings::setShowSourceLocationsGraphically( show );
    viewport()->update();
}

void PageView::setLastSourceLocationViewport( const Okular::DocumentViewport& vp )
{
    if( vp.rePos.enabled )
    {
        d->lastSourceLocationViewportNormalizedX = normClamp( vp.rePos.normalizedX, 0.5 );
        d->lastSourceLocationViewportNormalizedY = normClamp( vp.rePos.normalizedY, 0.0 );
    }
    else
    {
        d->lastSourceLocationViewportNormalizedX = 0.5;
        d->lastSourceLocationViewportNormalizedY = 0.0;
    }
    d->lastSourceLocationViewportPageNumber = vp.pageNumber;
    viewport()->update();
}

void PageView::clearLastSourceLocationViewport()
{
    d->lastSourceLocationViewportPageNumber = -1;
    d->lastSourceLocationViewportNormalizedX = 0.0;
    d->lastSourceLocationViewportNormalizedY = 0.0;
    viewport()->update();
}

void PageView::notifyViewportChanged( bool smoothMove )
{
    QMetaObject::invokeMethod(this, "slotRealNotifyViewportChanged", Qt::QueuedConnection, Q_ARG( bool, smoothMove ));
}

void PageView::slotRealNotifyViewportChanged( bool smoothMove )
{
    // if we are the one changing viewport, skip this nofity
    if ( d->blockViewport )
        return;

    // block setViewport outgoing calls
    d->blockViewport = true;

    // find PageViewItem matching the viewport description
    const Okular::DocumentViewport & vp = d->document->viewport();
    PageViewItem * item = nullptr;
    QVector< PageViewItem * >::const_iterator iIt = d->items.constBegin(), iEnd = d->items.constEnd();
    for ( ; iIt != iEnd; ++iIt )
        if ( (*iIt)->pageNumber() == vp.pageNumber )
        {
            item = *iIt;
            break;
        }
    if ( !item )
    {
        qCWarning(OkularUiDebug) << "viewport for page" << vp.pageNumber << "has no matching item!";
        d->blockViewport = false;
        return;
    }
#ifdef PAGEVIEW_DEBUG
    qCDebug(OkularUiDebug) << "document viewport changed";
#endif
    // relayout in "Single Pages" mode or if a relayout is pending
    d->blockPixmapsRequest = true;
    if ( !Okular::Settings::viewContinuous() || d->dirtyLayout )
        slotRelayoutPages();

    // restore viewport center or use default {x-center,v-top} alignment
    const QRect & r = item->croppedGeometry();
    int newCenterX = r.left(),
        newCenterY = r.top();
    if ( vp.rePos.enabled )
    {
        if ( vp.rePos.pos == Okular::DocumentViewport::Center )
        {
            newCenterX += (int)( normClamp( vp.rePos.normalizedX, 0.5 ) * (double)r.width() );
            newCenterY += (int)( normClamp( vp.rePos.normalizedY, 0.0 ) * (double)r.height() );
        }
        else
        {
            // TopLeft
            newCenterX += (int)( normClamp( vp.rePos.normalizedX, 0.0 ) * (double)r.width() + viewport()->width() / 2 );
            newCenterY += (int)( normClamp( vp.rePos.normalizedY, 0.0 ) * (double)r.height() + viewport()->height() / 2 );
        }
    }
    else
    {
        newCenterX += r.width() / 2;
        newCenterY += viewport()->height() / 2 - 10;
    }

    // if smooth movement requested, setup parameters and start it
    if ( smoothMove )
    {
        d->viewportMoveActive = true;
        d->viewportMoveTime.start();
        d->viewportMoveDest.setX( newCenterX );
        d->viewportMoveDest.setY( newCenterY );
        if ( !d->viewportMoveTimer )
        {
            d->viewportMoveTimer = new QTimer( this );
            connect( d->viewportMoveTimer, &QTimer::timeout,
                     this, &PageView::slotMoveViewport );
        }
        d->viewportMoveTimer->start( 25 );
        verticalScrollBar()->setEnabled( false );
        horizontalScrollBar()->setEnabled( false );
    }
    else
        center( newCenterX, newCenterY );
    d->blockPixmapsRequest = false;

    // request visible pixmaps in the current viewport and recompute it
    slotRequestVisiblePixmaps();

    // enable setViewport calls
    d->blockViewport = false;

    if( viewport() )
    {
        viewport()->update();
    }

    // since the page has moved below cursor, update it
    updateCursor();
}

void PageView::notifyPageChanged( int pageNumber, int changedFlags )
{
    // only handle pixmap / highlight changes notifies
    if ( changedFlags & DocumentObserver::Bookmark )
        return;

    if ( changedFlags & DocumentObserver::Annotations )
    {
        const QLinkedList< Okular::Annotation * > annots = d->document->page( pageNumber )->annotations();
        const QLinkedList< Okular::Annotation * >::ConstIterator annItEnd = annots.end();
        QSet< AnnotWindow * >::Iterator it = d->m_annowindows.begin();
        for ( ; it != d->m_annowindows.end(); )
        {
            QLinkedList< Okular::Annotation * >::ConstIterator annIt = qFind( annots, (*it)->annotation() );
            if ( annIt != annItEnd )
            {
                (*it)->reloadInfo();
                ++it;
            }
            else
            {
                AnnotWindow *w = *it;
                it = d->m_annowindows.erase( it );
                // Need to delete after removing from the list
                // otherwise deleting will call slotAnnotationWindowDestroyed which will mess
                // the list and the iterators
                delete w;
            }
        }

        d->mouseAnnotation->notifyAnnotationChanged( pageNumber );
    }

    if ( changedFlags & DocumentObserver::BoundingBox )
    {
#ifdef PAGEVIEW_DEBUG
        qCDebug(OkularUiDebug) << "BoundingBox change on page" << pageNumber;
#endif
        slotRelayoutPages();
        slotRequestVisiblePixmaps(); // TODO: slotRelayoutPages() may have done this already!
        // Repaint the whole widget since layout may have changed
        viewport()->update();
        return;
    }

    // iterate over visible items: if page(pageNumber) is one of them, repaint it
    QLinkedList< PageViewItem * >::const_iterator iIt = d->visibleItems.constBegin(), iEnd = d->visibleItems.constEnd();
    for ( ; iIt != iEnd; ++iIt )
        if ( (*iIt)->pageNumber() == pageNumber && (*iIt)->isVisible() )
        {
            // update item's rectangle plus the little outline
            QRect expandedRect = (*iIt)->croppedGeometry();
            // a PageViewItem is placed in the global page layout,
            // while we need to map its position in the viewport coordinates
            // (to get the correct area to repaint)
            expandedRect.translate( -contentAreaPosition() );
            expandedRect.adjust( -1, -1, 3, 3 );
            viewport()->update( expandedRect );

            // if we were "zoom-dragging" do not overwrite the "zoom-drag" cursor
            if ( cursor().shape() != Qt::SizeVerCursor )
            {
                // since the page has been regenerated below cursor, update it
                updateCursor();
            }
            break;
        }
}

void PageView::notifyContentsCleared( int changedFlags )
{
    // if pixmaps were cleared, re-ask them
    if ( changedFlags & DocumentObserver::Pixmap )
        QMetaObject::invokeMethod(this, "slotRequestVisiblePixmaps", Qt::QueuedConnection);
}

void PageView::notifyZoom( int factor )
{
    if ( factor > 0 )
        updateZoom( ZoomIn );
    else
        updateZoom( ZoomOut );
}

bool PageView::canUnloadPixmap( int pageNumber ) const
{
    if ( Okular::SettingsCore::memoryLevel() == Okular::SettingsCore::EnumMemoryLevel::Low ||
         Okular::SettingsCore::memoryLevel() == Okular::SettingsCore::EnumMemoryLevel::Normal )
    {
        // if the item is visible, forbid unloading
        QLinkedList< PageViewItem * >::const_iterator vIt = d->visibleItems.constBegin(), vEnd = d->visibleItems.constEnd();
        for ( ; vIt != vEnd; ++vIt )
            if ( (*vIt)->pageNumber() == pageNumber )
                return false;
    }
    else
    {
        // forbid unloading of the visible items, and of the previous and next
        QLinkedList< PageViewItem * >::const_iterator vIt = d->visibleItems.constBegin(), vEnd = d->visibleItems.constEnd();
        for ( ; vIt != vEnd; ++vIt )
            if ( abs( (*vIt)->pageNumber() - pageNumber ) <= 1 )
                return false;
    }
    // if hidden premit unloading
    return true;
}

void PageView::notifyCurrentPageChanged( int previous, int current )
{
    if ( previous != -1 )
    {
        PageViewItem * item = d->items.at( previous );
        if ( item )
        {
            Q_FOREACH ( VideoWidget *videoWidget, item->videoWidgets() )
                videoWidget->pageLeft();
        }
    }

    if ( current != -1 )
    {
        PageViewItem * item = d->items.at( current );
        if ( item )
        {
            Q_FOREACH ( VideoWidget *videoWidget, item->videoWidgets() )
                videoWidget->pageEntered();
        }

        // update zoom text and factor if in a ZoomFit/* zoom mode
        if ( d->zoomMode != ZoomFixed )
            updateZoomText();
    }
}

//END DocumentObserver inherited methods

//BEGIN View inherited methods
bool PageView::supportsCapability( ViewCapability capability ) const
{
    switch ( capability )
    {
        case Zoom:
        case ZoomModality:
            return true;
    }
    return false;
}

Okular::View::CapabilityFlags PageView::capabilityFlags( ViewCapability capability ) const
{
    switch ( capability )
    {
        case Zoom:
        case ZoomModality:
            return CapabilityRead | CapabilityWrite | CapabilitySerializable;
    }
    return nullptr;
}

QVariant PageView::capability( ViewCapability capability ) const
{
    switch ( capability )
    {
        case Zoom:
            return d->zoomFactor;
        case ZoomModality:
            return d->zoomMode;
    }
    return QVariant();
}

void PageView::setCapability( ViewCapability capability, const QVariant &option )
{
    switch ( capability )
    {
        case Zoom:
        {
            bool ok = true;
            double factor = option.toDouble( &ok );
            if ( ok && factor > 0.0 )
            {
                d->zoomFactor = static_cast< float >( factor );
                updateZoom( ZoomRefreshCurrent );
            }
            break;
        }
        case ZoomModality:
        {
            bool ok = true;
            int mode = option.toInt( &ok );
            if ( ok )
            {
                if ( mode >= 0 && mode < 3 )
                    updateZoom( (ZoomMode)mode );
            }
            break;
        }
    }
}

//END View inherited methods

//BEGIN widget events
bool PageView::event( QEvent * event )
{
    if ( event->type() == QEvent::Gesture )
    {
        return gestureEvent(static_cast<QGestureEvent*>( event ));
    }

    // do not stop the event
    return QAbstractScrollArea::event( event );
}


bool PageView::gestureEvent( QGestureEvent * event )
{
    QPinchGesture *pinch = static_cast<QPinchGesture*>(event->gesture(Qt::PinchGesture));

    if (pinch)
    {
        // Viewport zoom level at the moment where the pinch gesture starts.
        // The viewport zoom level _during_ the gesture will be this value
        // times the relative zoom reported by QGestureEvent.
        static qreal vanillaZoom = d->zoomFactor;

        if (pinch->state() == Qt::GestureStarted)
        {
            vanillaZoom = d->zoomFactor;
        }

        const QPinchGesture::ChangeFlags changeFlags = pinch->changeFlags();

        // Zoom
        if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged)
        {
            d->zoomFactor = vanillaZoom * pinch->totalScaleFactor();

            d->blockPixmapsRequest = true;
            updateZoom( ZoomRefreshCurrent );
            d->blockPixmapsRequest = false;
            viewport()->repaint();
        }

        // Count the number of 90-degree rotations we did since the start of the pinch gesture.
        // Otherwise a pinch turned to 90 degrees and held there will rotate the page again and again.
        static int rotations = 0;

        if (changeFlags & QPinchGesture::RotationAngleChanged)
        {
            // Rotation angle relative to the accumulated page rotations triggered by the current pinch
            // We actually turn at 80 degrees rather than at 90 degrees.  That's less strain on the hands.
            const qreal relativeAngle = pinch->rotationAngle() - rotations*90;
            if (relativeAngle > 80)
            {
                slotRotateClockwise();
                rotations++;
            }
            if (relativeAngle < -80)
            {
                slotRotateCounterClockwise();
                rotations--;
            }
        }

        if (pinch->state() == Qt::GestureFinished)
        {
            rotations = 0;
        }

        return true;
    }

    return false;
}

void PageView::paintEvent(QPaintEvent *pe)
{
        const QPoint areaPos = contentAreaPosition();
        // create the rect into contents from the clipped screen rect
        QRect viewportRect = viewport()->rect();
        viewportRect.translate( areaPos );
        QRect contentsRect = pe->rect().translated( areaPos ).intersected( viewportRect );
        if ( !contentsRect.isValid() )
            return;

#ifdef PAGEVIEW_DEBUG
        qCDebug(OkularUiDebug) << "paintevent" << contentsRect;
#endif

        // create the screen painter. a pixel painted at contentsX,contentsY
        // appears to the top-left corner of the scrollview.
        QPainter screenPainter( viewport() );
        // translate to simulate the scrolled content widget
        screenPainter.translate( -areaPos );

        // selectionRect is the normalized mouse selection rect
        QRect selectionRect = d->mouseSelectionRect;
        if ( !selectionRect.isNull() )
            selectionRect = selectionRect.normalized();
        // selectionRectInternal without the border
        QRect selectionRectInternal = selectionRect;
        selectionRectInternal.adjust( 1, 1, -1, -1 );
        // color for blending
        QColor selBlendColor = (selectionRect.width() > 8 || selectionRect.height() > 8) ?
                            d->mouseSelectionColor : Qt::red;

        // subdivide region into rects
        const QVector<QRect> &allRects = pe->region().rects();
        uint numRects = allRects.count();

        // preprocess rects area to see if it worths or not using subdivision
        uint summedArea = 0;
        for ( uint i = 0; i < numRects; i++ )
        {
            const QRect & r = allRects[i];
            summedArea += r.width() * r.height();
        }
        // very elementary check: SUMj(Region[j].area) is less than boundingRect.area
        bool useSubdivision = summedArea < (0.6 * contentsRect.width() * contentsRect.height());
        if ( !useSubdivision )
            numRects = 1;

        // iterate over the rects (only one loop if not using subdivision)
        for ( uint i = 0; i < numRects; i++ )
        {
            if ( useSubdivision )
            {
                // set 'contentsRect' to a part of the sub-divided region
                contentsRect = allRects[i].translated( areaPos ).intersected( viewportRect );
                if ( !contentsRect.isValid() )
                    continue;
            }
#ifdef PAGEVIEW_DEBUG
            qCDebug(OkularUiDebug) << contentsRect;
#endif

            // note: this check will take care of all things requiring alpha blending (not only selection)
            bool wantCompositing = !selectionRect.isNull() && contentsRect.intersects( selectionRect );
            // also alpha-blend when there is a table selection...
            wantCompositing |= !d->tableSelectionParts.isEmpty();

            if ( wantCompositing && Okular::Settings::enableCompositing() )
            {
                // create pixmap and open a painter over it (contents{left,top} becomes pixmap {0,0})
                QPixmap doubleBuffer( contentsRect.size() * devicePixelRatioF() );
                doubleBuffer.setDevicePixelRatio(devicePixelRatioF());
                QPainter pixmapPainter( &doubleBuffer );

                pixmapPainter.translate( -contentsRect.left(), -contentsRect.top() );

                // 1) Layer 0: paint items and clear bg on unpainted rects
                drawDocumentOnPainter( contentsRect, &pixmapPainter );
                // 2a) Layer 1a: paint (blend) transparent selection (rectangle)
                if ( !selectionRect.isNull() && selectionRect.intersects( contentsRect ) &&
                    !selectionRectInternal.contains( contentsRect ) )
                {
                    QRect blendRect = selectionRectInternal.intersected( contentsRect );
                    // skip rectangles covered by the selection's border
                    if ( blendRect.isValid() )
                    {
                        // grab current pixmap into a new one to colorize contents
                        QPixmap blendedPixmap( blendRect.width() * devicePixelRatioF(), blendRect.height() * devicePixelRatioF() );
                        blendedPixmap.setDevicePixelRatio(devicePixelRatioF());
                        QPainter p( &blendedPixmap );

                        p.drawPixmap( 0, 0, doubleBuffer,
                                    (blendRect.left() - contentsRect.left()) * devicePixelRatioF(), (blendRect.top() - contentsRect.top()) * devicePixelRatioF(),
                                    blendRect.width() * devicePixelRatioF(), blendRect.height() * devicePixelRatioF() );

                        QColor blCol = selBlendColor.dark( 140 );
                        blCol.setAlphaF( 0.2 );
                        p.fillRect( blendedPixmap.rect(), blCol );
                        p.end();
                        // copy the blended pixmap back to its place
                        pixmapPainter.drawPixmap( blendRect.left(), blendRect.top(), blendedPixmap );
                    }
                    // draw border (red if the selection is too small)
                    pixmapPainter.setPen( selBlendColor );
                    pixmapPainter.drawRect( selectionRect.adjusted( 0, 0, -1, -1 ) );
                }
                // 2b) Layer 1b: paint (blend) transparent selection (table)
                foreach (const TableSelectionPart &tsp, d->tableSelectionParts) {
                    QRect selectionPartRect = tsp.rectInItem.geometry(tsp.item->uncroppedWidth(), tsp.item->uncroppedHeight());
                    selectionPartRect.translate( tsp.item->uncroppedGeometry().topLeft () );
                    QRect selectionPartRectInternal = selectionPartRect;
                    selectionPartRectInternal.adjust( 1, 1, -1, -1 );
                    if ( !selectionPartRect.isNull() && selectionPartRect.intersects( contentsRect ) &&
                        !selectionPartRectInternal.contains( contentsRect ) )
                    {
                        QRect blendRect = selectionPartRectInternal.intersected( contentsRect );
                        // skip rectangles covered by the selection's border
                        if ( blendRect.isValid() )
                        {
                            // grab current pixmap into a new one to colorize contents
                            QPixmap blendedPixmap( blendRect.width()  * devicePixelRatioF(), blendRect.height()  * devicePixelRatioF() );
                            blendedPixmap.setDevicePixelRatio(devicePixelRatioF());
                            QPainter p( &blendedPixmap );
                            p.drawPixmap( 0, 0, doubleBuffer,
                                        (blendRect.left() - contentsRect.left()) * devicePixelRatioF(), (blendRect.top() - contentsRect.top()) * devicePixelRatioF(),
                                        blendRect.width() * devicePixelRatioF(), blendRect.height() * devicePixelRatioF() );

                            QColor blCol = d->mouseSelectionColor.dark( 140 );
                            blCol.setAlphaF( 0.2 );
                            p.fillRect( blendedPixmap.rect(), blCol );
                            p.end();
                            // copy the blended pixmap back to its place
                            pixmapPainter.drawPixmap( blendRect.left(), blendRect.top(), blendedPixmap );
                        }
                        // draw border (red if the selection is too small)
                        pixmapPainter.setPen( d->mouseSelectionColor );
                        pixmapPainter.drawRect( selectionPartRect.adjusted( 0, 0, -1, -1 ) );
                    }
                }
                drawTableDividers( &pixmapPainter );
                // 3a) Layer 1: give annotator painting control
                if ( d->annotator && d->annotator->routePaints( contentsRect ) )
                    d->annotator->routePaint( &pixmapPainter, contentsRect );
                // 3b) Layer 1: give mouseAnnotation painting control
                d->mouseAnnotation->routePaint( &pixmapPainter, contentsRect );

                // 4) Layer 2: overlays
                if ( Okular::Settings::debugDrawBoundaries() )
                {
                    pixmapPainter.setPen( Qt::blue );
                    pixmapPainter.drawRect( contentsRect );
                }

                // finish painting and draw contents
                pixmapPainter.end();
                screenPainter.drawPixmap( contentsRect.left(), contentsRect.top(), doubleBuffer );
            }
            else
            {
                // 1) Layer 0: paint items and clear bg on unpainted rects
                drawDocumentOnPainter( contentsRect, &screenPainter );
                // 2a) Layer 1a: paint opaque selection (rectangle)
                if ( !selectionRect.isNull() && selectionRect.intersects( contentsRect ) &&
                    !selectionRectInternal.contains( contentsRect ) )
                {
                    screenPainter.setPen( palette().color( QPalette::Active, QPalette::Highlight ).dark(110) );
                    screenPainter.drawRect( selectionRect );
                }
                // 2b) Layer 1b: paint opaque selection (table)
                foreach (const TableSelectionPart &tsp, d->tableSelectionParts) {
                    QRect selectionPartRect = tsp.rectInItem.geometry(tsp.item->uncroppedWidth(), tsp.item->uncroppedHeight());
                    selectionPartRect.translate( tsp.item->uncroppedGeometry().topLeft () );
                    QRect selectionPartRectInternal = selectionPartRect;
                    selectionPartRectInternal.adjust( 1, 1, -1, -1 );
                    if ( !selectionPartRect.isNull() && selectionPartRect.intersects( contentsRect ) &&
                        !selectionPartRectInternal.contains( contentsRect ) )
                    {
                        screenPainter.setPen( palette().color( QPalette::Active, QPalette::Highlight ).dark(110) );
                        screenPainter.drawRect( selectionPartRect );
                    }
                }
                drawTableDividers( &screenPainter );
                // 3a) Layer 1: give annotator painting control
                if ( d->annotator && d->annotator->routePaints( contentsRect ) )
                    d->annotator->routePaint( &screenPainter, contentsRect );
                // 3b) Layer 1: give mouseAnnotation painting control
                d->mouseAnnotation->routePaint( &screenPainter, contentsRect );

                // 4) Layer 2: overlays
                if ( Okular::Settings::debugDrawBoundaries() )
                {
                    screenPainter.setPen( Qt::red );
                    screenPainter.drawRect( contentsRect );
                }
            }
        }
}

void PageView::drawTableDividers(QPainter * screenPainter)
{
        if (!d->tableSelectionParts.isEmpty()) {
            screenPainter->setPen( d->mouseSelectionColor.dark() );
            if (d->tableDividersGuessed) {
                QPen p = screenPainter->pen();
                p.setStyle( Qt::DashLine );
                screenPainter->setPen( p );
            }
            foreach (const TableSelectionPart &tsp, d->tableSelectionParts) {
                QRect selectionPartRect = tsp.rectInItem.geometry(tsp.item->uncroppedWidth(), tsp.item->uncroppedHeight());
                selectionPartRect.translate( tsp.item->uncroppedGeometry().topLeft () );
                QRect selectionPartRectInternal = selectionPartRect;
                selectionPartRectInternal.adjust( 1, 1, -1, -1 );
                foreach(double col, d->tableSelectionCols) {
                    if (col >= tsp.rectInSelection.left && col <= tsp.rectInSelection.right) {
                        col = (col - tsp.rectInSelection.left) / (tsp.rectInSelection.right - tsp.rectInSelection.left);
                        const int x =  selectionPartRect.left() + col * selectionPartRect.width() + 0.5;
                        screenPainter->drawLine(
                            x, selectionPartRectInternal.top(),
                            x, selectionPartRectInternal.top() + selectionPartRectInternal.height()
                        );
                    }
                }
                foreach(double row, d->tableSelectionRows) {
                    if (row >= tsp.rectInSelection.top && row <= tsp.rectInSelection.bottom) {
                        row = (row - tsp.rectInSelection.top) / (tsp.rectInSelection.bottom - tsp.rectInSelection.top);
                        const int y =  selectionPartRect.top() + row * selectionPartRect.height() + 0.5;
                        screenPainter->drawLine(
                            selectionPartRectInternal.left(), y,
                            selectionPartRectInternal.left() + selectionPartRectInternal.width(), y
                        );
                    }
                }
            }
        }
}

void PageView::resizeEvent( QResizeEvent *e )
{
    if ( d->items.isEmpty() )
    {
        resizeContentArea( e->size() );
        return;
    }

    if ( ( d->zoomMode == ZoomFitWidth || d->zoomMode == ZoomFitAuto ) && !verticalScrollBar()->isVisible() && qAbs(e->oldSize().height() - e->size().height()) < verticalScrollBar()->width() && d->verticalScrollBarVisible )
    {
        // this saves us from infinite resizing loop because of scrollbars appearing and disappearing
        // see bug 160628 for more info
        // TODO looks are still a bit ugly because things are left uncentered
        // but better a bit ugly than unusable
        d->verticalScrollBarVisible = false;
        resizeContentArea( e->size() );
        return;
    }
    else if ( d->zoomMode == ZoomFitAuto && !horizontalScrollBar()->isVisible() && qAbs(e->oldSize().width() - e->size().width()) < horizontalScrollBar()->height() && d->horizontalScrollBarVisible )
    {
        // this saves us from infinite resizing loop because of scrollbars appearing and disappearing
        // TODO looks are still a bit ugly because things are left uncentered
        // but better a bit ugly than unusable
        d->horizontalScrollBarVisible = false;
        resizeContentArea( e->size() );
        return;
    }

    // start a timer that will refresh the pixmap after 0.2s
    d->delayResizeEventTimer->start( 200 );
    d->verticalScrollBarVisible = verticalScrollBar()->isVisible();
    d->horizontalScrollBarVisible = horizontalScrollBar()->isVisible();
}

void PageView::keyPressEvent( QKeyEvent * e )
{
    e->accept();

    // if performing a selection or dyn zooming, disable keys handling
    if ( ( d->mouseSelecting && e->key() != Qt::Key_Escape ) || ( QApplication::mouseButtons () & Qt::MidButton ) )
        return;

    // if viewport is moving, disable keys handling
    if ( d->viewportMoveActive )
        return;

    // move/scroll page by using keys
    switch ( e->key() )
    {
        case Qt::Key_J:
        case Qt::Key_K:
        case Qt::Key_Down:
        case Qt::Key_PageDown:
        case Qt::Key_Up:
        case Qt::Key_PageUp:
        case Qt::Key_Backspace:
            if ( e->key() == Qt::Key_Down
                 || e->key() == Qt::Key_PageDown
                 || e->key() == Qt::Key_J )
            {
                bool singleStep = e->key() == Qt::Key_Down || e->key() == Qt::Key_J;
                slotScrollDown( singleStep );
            }
            else
            {
                bool singleStep = e->key() == Qt::Key_Up || e->key() == Qt::Key_K;
                slotScrollUp( singleStep );
            }
            break;
        case Qt::Key_Left:
        case Qt::Key_H:
            if ( horizontalScrollBar()->maximum() == 0 )
            {
                //if we cannot scroll we go to the previous page vertically
                int next_page = d->document->currentPage() - viewColumns();
                d->document->setViewportPage(next_page);
            }
            else
                horizontalScrollBar()->triggerAction( QScrollBar::SliderSingleStepSub );
            break;
        case Qt::Key_Right:
        case Qt::Key_L:
            if ( horizontalScrollBar()->maximum() == 0 )
            {
                //if we cannot scroll we advance the page vertically
                int next_page = d->document->currentPage() + viewColumns();
                d->document->setViewportPage(next_page);
            }
            else
                horizontalScrollBar()->triggerAction( QScrollBar::SliderSingleStepAdd );
            break;
        case Qt::Key_Escape:
            emit escPressed();
            selectionClear( d->tableDividersGuessed ? ClearOnlyDividers : ClearAllSelection );
            d->mousePressPos = QPoint();
            if ( d->aPrevAction )
            {
                d->aPrevAction->trigger();
                d->aPrevAction = nullptr;
            }
            d->mouseAnnotation->routeKeyPressEvent( e );
            break;
        case Qt::Key_Delete:
            d->mouseAnnotation->routeKeyPressEvent( e );
            break;
        case Qt::Key_Shift:
        case Qt::Key_Control:
            if ( d->autoScrollTimer )
            {
                if ( d->autoScrollTimer->isActive() )
                    d->autoScrollTimer->stop();
                else
                    slotAutoScroll();
                return;
            }
            // fallthrough
        default:
            e->ignore();
            return;
    }
    // if a known key has been pressed, stop scrolling the page
    if ( d->autoScrollTimer )
    {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }
}

void PageView::keyReleaseEvent( QKeyEvent * e )
{
    e->accept();

    if ( d->annotator && d->annotator->active() )
    {
        if ( d->annotator->routeKeyEvent( e ) )
            return;
    }

    if ( e->key() == Qt::Key_Escape && d->autoScrollTimer )
    {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }
}

void PageView::inputMethodEvent( QInputMethodEvent * e )
{
    Q_UNUSED(e)
}

void PageView::tabletEvent( QTabletEvent * e )
{
    // Ignore tablet events that we don't care about
    if ( !( e->type() == QEvent::TabletPress ||
            e->type() == QEvent::TabletRelease ||
            e->type() == QEvent::TabletMove ) )
    {
        e->ignore();
        return;
    }

    // Determine pen state
    bool penReleased = false;
    if ( e->type() == QEvent::TabletPress )
    {
        d->penDown = true;
    }
    if ( e->type() == QEvent::TabletRelease )
    {
        d->penDown = false;
        penReleased = true;
    }

    // If we're editing an annotation and the tablet pen is either down or just released
    // then dispatch event to annotator
    if ( d->annotator && d->annotator->active() && ( d->penDown || penReleased ) )
    {
        const QPoint eventPos = contentAreaPoint( e->pos() );
        PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
        const QPoint localOriginInGlobal = mapToGlobal( QPoint(0,0) );

        // routeTabletEvent will accept or ignore event as appropriate
        d->annotator->routeTabletEvent( e, pageItem, localOriginInGlobal );
    } else {
        e->ignore();
    }
}

void PageView::mouseMoveEvent( QMouseEvent * e )
{
    d->controlWheelAccumulatedDelta = 0;

    // don't perform any mouse action when no document is shown
    if ( d->items.isEmpty() )
        return;

    // don't perform any mouse action when viewport is autoscrolling
    if ( d->viewportMoveActive )
        return;

    // if holding mouse mid button, perform zoom
    if ( e->buttons() & Qt::MidButton )
    {
        int mouseY = e->globalPos().y();
        int deltaY = d->mouseMidLastY - mouseY;

        // wrap mouse from top to bottom
        const QRect mouseContainer = QApplication::desktop()->screenGeometry( this );
        const int absDeltaY = abs(deltaY);
        if ( absDeltaY > mouseContainer.height() / 2 )
        {
            deltaY = mouseContainer.height() - absDeltaY;
        }

        const float upperZoomLimit = d->document->supportsTiles() ? 15.99 : 3.99;
        if ( mouseY <= mouseContainer.top() + 4 &&
             d->zoomFactor < upperZoomLimit )
        {
            mouseY = mouseContainer.bottom() - 5;
            QCursor::setPos( e->globalPos().x(), mouseY );
        }
        // wrap mouse from bottom to top
        else if ( mouseY >= mouseContainer.bottom() - 4 &&
                  d->zoomFactor > 0.101 )
        {
            mouseY = mouseContainer.top() + 5;
            QCursor::setPos( e->globalPos().x(), mouseY );
        }
        // remember last position
        d->mouseMidLastY = mouseY;

        // update zoom level, perform zoom and redraw
        if ( deltaY )
        {
            d->zoomFactor *= ( 1.0 + ( (double)deltaY / 500.0 ) );
            d->blockPixmapsRequest = true;
            updateZoom( ZoomRefreshCurrent );
            d->blockPixmapsRequest = false;
            viewport()->repaint();
        }
        return;
    }

    const QPoint eventPos = contentAreaPoint( e->pos() );

    // if we're editing an annotation, dispatch event to it
    if ( d->annotator && d->annotator->active() )
    {
        PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
        updateCursor( eventPos );
        d->annotator->routeMouseEvent( e, pageItem );
        return;
    }

    bool leftButton = (e->buttons() == Qt::LeftButton);
    bool rightButton = (e->buttons() == Qt::RightButton);

    switch ( d->mouseMode )
    {
        case Okular::Settings::EnumMouseMode::Browse:
        {
            PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
            if ( leftButton )
            {
                d->leftClickTimer.stop();
                if ( pageItem && d->mouseAnnotation->isActive() )
                {
                    // if left button pressed and annotation is focused, forward move event
                    d->mouseAnnotation->routeMouseMoveEvent( pageItem, eventPos, leftButton );
                }
                // drag page
                else if ( !d->mouseGrabPos.isNull() )
                {
                    setCursor( Qt::ClosedHandCursor );

                    QPoint mousePos = e->globalPos();
                    QPoint delta = d->mouseGrabPos - mousePos;

                    // wrap mouse from top to bottom
                    const QRect mouseContainer = QApplication::desktop()->screenGeometry( this );
                    // If the delta is huge it probably means we just wrapped in that direction
                    const QPoint absDelta(abs(delta.x()), abs(delta.y()));
                    if ( absDelta.y() > mouseContainer.height() / 2 )
                    {
                        delta.setY(mouseContainer.height() - absDelta.y());
                    }
                    if ( absDelta.x() > mouseContainer.width() / 2 )
                    {
                        delta.setX(mouseContainer.width() - absDelta.x());
                    }
                    if ( mousePos.y() <= mouseContainer.top() + 4 &&
                         verticalScrollBar()->value() < verticalScrollBar()->maximum() - 10 )
                    {
                        mousePos.setY( mouseContainer.bottom() - 5 );
                        QCursor::setPos( mousePos );
                    }
                    // wrap mouse from bottom to top
                    else if ( mousePos.y() >= mouseContainer.bottom() - 4 &&
                              verticalScrollBar()->value() > 10 )
                    {
                        mousePos.setY( mouseContainer.top() + 5 );
                        QCursor::setPos( mousePos );
                    }
                    // remember last position
                    d->mouseGrabPos = mousePos;

                    // scroll page by position increment
                    scrollTo( horizontalScrollBar()->value() + delta.x(), verticalScrollBar()->value() + delta.y() );
                }
            }
            else if ( rightButton && !d->mousePressPos.isNull() && d->aMouseSelect )
            {
                // if mouse moves 5 px away from the press point, switch to 'selection'
                int deltaX = d->mousePressPos.x() - e->globalPos().x(),
                    deltaY = d->mousePressPos.y() - e->globalPos().y();
                if ( deltaX > 5 || deltaX < -5 || deltaY > 5 || deltaY < -5 )
                {
                    d->aPrevAction = d->aMouseNormal;
                    d->aMouseSelect->trigger();
                    QPoint newPos = eventPos + QPoint( deltaX, deltaY );
                    selectionStart( newPos, palette().color( QPalette::Active, QPalette::Highlight ).light( 120 ), false );
                    updateSelection( eventPos );
                    break;
                }
            }
            else
            {
                /* Forward move events which are still not yet consumed by "mouse grab" or aMouseSelect */
                d->mouseAnnotation->routeMouseMoveEvent( pageItem, eventPos, leftButton );
                updateCursor();
            }
        }
        break;

        case Okular::Settings::EnumMouseMode::Zoom:
        case Okular::Settings::EnumMouseMode::RectSelect:
        case Okular::Settings::EnumMouseMode::TableSelect:
        case Okular::Settings::EnumMouseMode::TrimSelect:
            // set second corner of selection
            if ( d->mouseSelecting ) {
                updateSelection( eventPos );
                d->mouseOverLinkObject = nullptr;
            }
            updateCursor();
            break;

        case Okular::Settings::EnumMouseMode::Magnifier:
            if ( e->buttons() ) // if any button is pressed at all
            {
                moveMagnifier( e->pos() );
                updateMagnifier( eventPos );
            }
            break;

        case Okular::Settings::EnumMouseMode::TextSelect:
            // if mouse moves 5 px away from the press point and the document soupports text extraction, do 'textselection'
            if ( !d->mouseTextSelecting && !d->mousePressPos.isNull() && d->document->supportsSearching() && ( ( eventPos - d->mouseSelectPos ).manhattanLength() > 5 ) )
            {
                d->mouseTextSelecting = true;
            }
            updateSelection( eventPos );
            updateCursor();
            break;
    }
}

void PageView::mousePressEvent( QMouseEvent * e )
{
    d->controlWheelAccumulatedDelta = 0;

    // don't perform any mouse action when no document is shown
    if ( d->items.isEmpty() )
        return;

    // if performing a selection or dyn zooming, disable mouse press
    if ( d->mouseSelecting || ( e->button() != Qt::MidButton && ( e->buttons() & Qt::MidButton) ) || d->viewportMoveActive )
        return;

    // if the page is scrolling, stop it
    if ( d->autoScrollTimer )
    {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }

    // if pressing mid mouse button while not doing other things, begin 'continuous zoom' mode
    if ( e->button() == Qt::MidButton )
    {
        d->mouseMidLastY = e->globalPos().y();
        setCursor( Qt::SizeVerCursor );
        return;
    }

    const QPoint eventPos = contentAreaPoint( e->pos() );

    // if we're editing an annotation, dispatch event to it
    if ( d->annotator && d->annotator->active() )
    {
        PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
        d->annotator->routeMouseEvent( e, pageItem );
        return;
    }

    // trigger history navigation for additional mouse buttons
    if ( e->button() == Qt::XButton1 )
    {
        emit mouseBackButtonClick();
        return;
    }
    if ( e->button() == Qt::XButton2 )
    {
        emit mouseForwardButtonClick();
        return;
    }

    // update press / 'start drag' mouse position
    d->mousePressPos = e->globalPos();

    // handle mode dependant mouse press actions
    bool leftButton = e->button() == Qt::LeftButton,
         rightButton = e->button() == Qt::RightButton;

//   Not sure we should erase the selection when clicking with left.
     if ( d->mouseMode != Okular::Settings::EnumMouseMode::TextSelect )
       textSelectionClear();

    switch ( d->mouseMode )
    {
        case Okular::Settings::EnumMouseMode::Browse:   // drag start / click / link following
        {
            PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
            if ( leftButton )
            {
                if ( pageItem )
                {
                    d->mouseAnnotation->routeMousePressEvent( pageItem, eventPos );
                }
                d->mouseGrabPos = d->mouseOnRect ? QPoint() : d->mousePressPos;
                if ( !d->mouseOnRect )
                    d->leftClickTimer.start( QApplication::doubleClickInterval() + 10 );
            }
            else if ( rightButton )
            {
                if ( pageItem )
                {
                    // find out normalized mouse coords inside current item
                    const QRect & itemRect = pageItem->uncroppedGeometry();
                    double nX = pageItem->absToPageX(eventPos.x());
                    double nY = pageItem->absToPageY(eventPos.y());

                    const QLinkedList< const Okular::ObjectRect *> orects = pageItem->page()->objectRects( Okular::ObjectRect::OAnnotation, nX, nY, itemRect.width(), itemRect.height() );

                    if ( !orects.isEmpty() )
                    {
                        AnnotationPopup popup( d->document, AnnotationPopup::MultiAnnotationMode, this );

                        foreach ( const Okular::ObjectRect * orect, orects )
                        {
                            Okular::Annotation * ann = ( (Okular::AnnotationObjectRect *)orect )->annotation();
                            if ( ann && (ann->subType() != Okular::Annotation::AWidget) )
                                popup.addAnnotation( ann, pageItem->pageNumber() );

                        }

                        connect( &popup, &AnnotationPopup::openAnnotationWindow,
                                 this, &PageView::openAnnotationWindow );

                        popup.exec( e->globalPos() );
                        // Since ↑ spins its own event loop we won't get the mouse release event
                        // so reset mousePressPos here
                        d->mousePressPos = QPoint();
                    }
                }
            }
        }
        break;

        case Okular::Settings::EnumMouseMode::Zoom:     // set first corner of the zoom rect
            if ( leftButton )
                selectionStart( eventPos, palette().color( QPalette::Active, QPalette::Highlight ), false );
            else if ( rightButton )
                updateZoom( ZoomOut );
            break;

        case Okular::Settings::EnumMouseMode::Magnifier:
            moveMagnifier( e->pos() );
            d->magnifierView->show();
            updateMagnifier( eventPos );
            break;

        case Okular::Settings::EnumMouseMode::RectSelect:   // set first corner of the selection rect
        case Okular::Settings::EnumMouseMode::TrimSelect:
             if ( leftButton )
             {
                selectionStart( eventPos, palette().color( QPalette::Active, QPalette::Highlight ).light( 120 ), false );
             }
            break;
        case Okular::Settings::EnumMouseMode::TableSelect:
             if ( leftButton )
             {
                if (d->tableSelectionParts.isEmpty())
                {
                    selectionStart( eventPos, palette().color( QPalette::Active, QPalette::Highlight ).light( 120 ), false );
                } else {
                    QRect updatedRect;
                    foreach (const TableSelectionPart &tsp, d->tableSelectionParts) {
                        QRect selectionPartRect = tsp.rectInItem.geometry(tsp.item->uncroppedWidth(), tsp.item->uncroppedHeight());
                        selectionPartRect.translate( tsp.item->uncroppedGeometry().topLeft () );

                        // This will update the whole table rather than just the added/removed divider
                        // (which can span more than one part).
                        updatedRect = updatedRect.united(selectionPartRect);

                        if (!selectionPartRect.contains(eventPos))
                            continue;

                        // At this point it's clear we're either adding or removing a divider manually, so obviously the user is happy with the guess (if any).
                        d->tableDividersGuessed = false;

                        // There's probably a neat trick to finding which edge it's closest to,
                        // but this way has the advantage of simplicity.
                        const int fromLeft = abs(selectionPartRect.left() - eventPos.x());
                        const int fromRight = abs(selectionPartRect.left() + selectionPartRect.width() - eventPos.x());
                        const int fromTop = abs(selectionPartRect.top() - eventPos.y());
                        const int fromBottom = abs(selectionPartRect.top() + selectionPartRect.height() - eventPos.y());
                        const int colScore = fromTop<fromBottom ? fromTop : fromBottom;
                        const int rowScore = fromLeft<fromRight ? fromLeft : fromRight;

                        if (colScore < rowScore) {
                            bool deleted=false;
                            for(int i=0; i<d->tableSelectionCols.length(); i++) {
                                const double col = (d->tableSelectionCols[i] - tsp.rectInSelection.left) / (tsp.rectInSelection.right - tsp.rectInSelection.left);
                                const int colX =  selectionPartRect.left() + col * selectionPartRect.width() + 0.5;
                                if (abs(colX - eventPos.x())<=3) {
                                    d->tableSelectionCols.removeAt(i);
                                    deleted=true;

                                    break;
                                }
                            }
                            if (!deleted) {
                                double col = eventPos.x() - selectionPartRect.left();
                                col /= selectionPartRect.width(); // at this point, it's normalised within the part
                                col *= (tsp.rectInSelection.right - tsp.rectInSelection.left);
                                col += tsp.rectInSelection.left; // at this point, it's normalised within the whole table

                                d->tableSelectionCols.append(col);
                                qSort(d->tableSelectionCols);
                            }
                        } else {
                            bool deleted=false;
                            for(int i=0; i<d->tableSelectionRows.length(); i++) {
                                const double row = (d->tableSelectionRows[i] - tsp.rectInSelection.top) / (tsp.rectInSelection.bottom - tsp.rectInSelection.top);
                                const int rowY =  selectionPartRect.top() + row * selectionPartRect.height() + 0.5;
                                if (abs(rowY - eventPos.y())<=3) {
                                    d->tableSelectionRows.removeAt(i);
                                    deleted=true;

                                    break;
                                }
                            }
                            if (!deleted) {
                                double row = eventPos.y() - selectionPartRect.top();
                                row /= selectionPartRect.height(); // at this point, it's normalised within the part
                                row *= (tsp.rectInSelection.bottom - tsp.rectInSelection.top);
                                row += tsp.rectInSelection.top; // at this point, it's normalised within the whole table

                                d->tableSelectionRows.append(row);
                                qSort(d->tableSelectionRows);
                            }
                        }
                    }
                    updatedRect.translate( -contentAreaPosition() );
                    viewport()->update( updatedRect );
                }
             }
            break;
        case Okular::Settings::EnumMouseMode::TextSelect:
            d->mouseSelectPos = eventPos;
            if ( !rightButton )
            {
                textSelectionClear();
            }
            break;
    }
}

void PageView::mouseReleaseEvent( QMouseEvent * e )
{
    d->controlWheelAccumulatedDelta = 0;

    // stop the drag scrolling
    d->dragScrollTimer.stop();

    d->leftClickTimer.stop();

    const bool leftButton = e->button() == Qt::LeftButton;
    const bool rightButton = e->button() == Qt::RightButton;

    if ( d->mouseAnnotation->isActive() && leftButton )
    {
        // Just finished to move the annotation
        d->mouseAnnotation->routeMouseReleaseEvent();
    }

    // don't perform any mouse action when no document is shown..
    if ( d->items.isEmpty() )
    {
        // ..except for right Clicks (emitted even it viewport is empty)
        if ( e->button() == Qt::RightButton )
            emit rightClick( nullptr, e->globalPos() );
        return;
    }

    // don't perform any mouse action when viewport is autoscrolling
    if ( d->viewportMoveActive )
        return;

    const QPoint eventPos = contentAreaPoint( e->pos() );

    // handle mode indepent mid buttom zoom
    if ( e->button() == Qt::MidButton )
    {
        // request pixmaps since it was disabled during drag
        slotRequestVisiblePixmaps();
        // the cursor may now be over a link.. update it
        updateCursor( eventPos );
        return;
    }

    // if we're editing an annotation, dispatch event to it
    if ( d->annotator && d->annotator->active() )
    {
        PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
        d->annotator->routeMouseEvent( e, pageItem );
        return;
    }

    switch ( d->mouseMode )
    {
        case Okular::Settings::EnumMouseMode::Browse:{
            // return the cursor to its normal state after dragging
            if ( cursor().shape() == Qt::ClosedHandCursor )
                updateCursor( eventPos );

            PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
            const QPoint pressPos = contentAreaPoint( mapFromGlobal( d->mousePressPos ) );
            const PageViewItem * pageItemPressPos = pickItemOnPoint( pressPos.x(), pressPos.y() );

            // if the mouse has not moved since the press, that's a -click-
            if ( leftButton && pageItem && pageItem == pageItemPressPos &&
                 ( (d->mousePressPos - e->globalPos()).manhattanLength() < QApplication::startDragDistance() ) )
            {
                if ( !mouseReleaseOverLink( d->mouseOverLinkObject ) && ( e->modifiers() == Qt::ShiftModifier ) )
                {
                    const double nX = pageItem->absToPageX(eventPos.x());
                    const double nY = pageItem->absToPageY(eventPos.y());
                    const Okular::ObjectRect * rect;
                    // TODO: find a better way to activate the source reference "links"
                    // for the moment they are activated with Shift + left click
                    // Search the nearest source reference.
                    rect = pageItem->page()->objectRect( Okular::ObjectRect::SourceRef, nX, nY, pageItem->uncroppedWidth(), pageItem->uncroppedHeight() );
                    if ( !rect )
                    {
                        static const double s_minDistance = 0.025; // FIXME?: empirical value?
                        double distance = 0.0;
                        rect = pageItem->page()->nearestObjectRect( Okular::ObjectRect::SourceRef, nX, nY, pageItem->uncroppedWidth(), pageItem->uncroppedHeight(), &distance );
                        // distance is distanceSqr, adapt it to a normalized value
                        distance = distance / (pow( pageItem->uncroppedWidth(), 2 ) + pow( pageItem->uncroppedHeight(), 2 ));
                        if ( rect && ( distance > s_minDistance ) )
                            rect = nullptr;
                    }
                    if ( rect )
                    {
                        const Okular::SourceReference * ref = static_cast< const Okular::SourceReference * >( rect->object() );
                        d->document->processSourceReference( ref );
                    }
                    else
                    {
                        const Okular::SourceReference * ref = d->document->dynamicSourceReference( pageItem->  pageNumber(), nX * pageItem->page()->width(), nY * pageItem->page()->height() );
                        if ( ref )
                        {
                            d->document->processSourceReference( ref );
                            delete ref;
                        }
                    }
                }
#if 0
                else
                {
                    // a link can move us to another page or even to another document, there's no point in trying to
                    //  process the click on the image once we have processes the click on the link
                    rect = pageItem->page()->objectRect( Okular::ObjectRect::Image, nX, nY, pageItem->width(), pageItem->height() );
                    if ( rect )
                    {
                        // handle click over a image
                    }
/*		Enrico and me have decided this is not worth the trouble it generates
                    else
                    {
                        // if not on a rect, the click selects the page
                        // if ( pageItem->pageNumber() != (int)d->document->currentPage() )
                        d->document->setViewportPage( pageItem->pageNumber(), this );
                    }*/
                }
#endif
            }
            else if ( rightButton && !d->mouseAnnotation->isModified() )
            {
                if ( pageItem && pageItem == pageItemPressPos &&
                     ( (d->mousePressPos - e->globalPos()).manhattanLength() < QApplication::startDragDistance() ) )
                {
                    QMenu * menu = createProcessLinkMenu(pageItem, eventPos );
                    if ( menu )
                    {
                        menu->exec( e->globalPos() );
                        menu->deleteLater();
                    }
                    else
                    {
                        const double nX = pageItem->absToPageX(eventPos.x());
                        const double nY = pageItem->absToPageY(eventPos.y());
                        // a link can move us to another page or even to another document, there's no point in trying to
                        //  process the click on the image once we have processes the click on the link
                        const Okular::ObjectRect * rect = pageItem->page()->objectRect( Okular::ObjectRect::Image, nX, nY, pageItem->uncroppedWidth(), pageItem->uncroppedHeight() );
                        if ( rect )
                        {
                            // handle right click over a image
                        }
                        else
                        {
                            // right click (if not within 5 px of the press point, the mode
                            // had been already changed to 'Selection' instead of 'Normal')
                            emit rightClick( pageItem->page(), e->globalPos() );
                        }
                    }
                }
                else
                {
                    // right click (if not within 5 px of the press point, the mode
                    // had been already changed to 'Selection' instead of 'Normal')
                    emit rightClick( pageItem ? pageItem->page() : nullptr, e->globalPos() );
                }
            }
            }break;

        case Okular::Settings::EnumMouseMode::Zoom:
            // if a selection rect has been defined, zoom into it
            if ( leftButton && d->mouseSelecting )
            {
                QRect selRect = d->mouseSelectionRect.normalized();
                if ( selRect.width() <= 8 && selRect.height() <= 8 )
                {
                    selectionClear();
                    break;
                }

                // find out new zoom ratio and normalized view center (relative to the contentsRect)
                double zoom = qMin( (double)viewport()->width() / (double)selRect.width(), (double)viewport()->height() / (double)selRect.height() );
                double nX = (double)(selRect.left() + selRect.right()) / (2.0 * (double)contentAreaWidth());
                double nY = (double)(selRect.top() + selRect.bottom()) / (2.0 * (double)contentAreaHeight());

                const float upperZoomLimit = d->document->supportsTiles() ? 16.0 : 4.0;
                if ( d->zoomFactor <= upperZoomLimit || zoom <= 1.0 )
                {
                    d->zoomFactor *= zoom;
                    viewport()->setUpdatesEnabled( false );
                    updateZoom( ZoomRefreshCurrent );
                    viewport()->setUpdatesEnabled( true );
                }

                // recenter view and update the viewport
                center( (int)(nX * contentAreaWidth()), (int)(nY * contentAreaHeight()) );
                viewport()->update();

                // hide message box and delete overlay window
                selectionClear();
            }
            break;

        case Okular::Settings::EnumMouseMode::Magnifier:
            d->magnifierView->hide();
            break;

        case Okular::Settings::EnumMouseMode::TrimSelect:
        {
            // if it is a left release checks if is over a previous link press
            if ( leftButton && mouseReleaseOverLink ( d->mouseOverLinkObject ) ) {
                selectionClear();
                break;
            }

            // if mouse is released and selection is null this is a rightClick
            if ( rightButton && !d->mouseSelecting )
            {
                break;
            }
            PageViewItem * pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
            // ensure end point rests within a page, or ignore
            if (!pageItem) {
                break;
            }
            QRect selectionRect = d->mouseSelectionRect.normalized();

            double nLeft = pageItem->absToPageX(selectionRect.left());
            double nRight = pageItem->absToPageX(selectionRect.right());
            double nTop = pageItem->absToPageY(selectionRect.top());
            double nBottom = pageItem->absToPageY(selectionRect.bottom());
            if ( nLeft < 0 ) nLeft = 0;
            if ( nTop < 0 ) nTop = 0;
            if ( nRight > 1 ) nRight = 1;
            if ( nBottom > 1 ) nBottom = 1;
            d->trimBoundingBox = Okular::NormalizedRect(nLeft, nTop, nRight, nBottom);

            // Trim Selection successfully done, hide prompt
            d->messageWindow->hide();

            // clear widget selection and invalidate rect
            selectionClear();

            // When Trim selection bbox interaction is over, we should switch to another mousemode.
            if ( d->aPrevAction )
            {
                d->aPrevAction->trigger();
                d->aPrevAction = nullptr;
            } else {
                d->aMouseNormal->trigger();
            }

            // with d->trimBoundingBox defined, redraw for trim to take visual effect
            if ( d->document->pages() > 0 )
            {
                slotRelayoutPages();
                slotRequestVisiblePixmaps();  // TODO: slotRelayoutPages() may have done this already!
            }

            break;
        }
        case Okular::Settings::EnumMouseMode::RectSelect:
        {
            // if it is a left release checks if is over a previous link press
            if ( leftButton && mouseReleaseOverLink ( d->mouseOverLinkObject ) ) {
                selectionClear();
                break;
            }

            // if mouse is released and selection is null this is a rightClick
            if ( rightButton && !d->mouseSelecting )
            {
                PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
                emit rightClick( pageItem ? pageItem->page() : nullptr, e->globalPos() );
                break;
            }

            // if a selection is defined, display a popup
            if ( (!leftButton && !d->aPrevAction) || (!rightButton && d->aPrevAction) ||
                 !d->mouseSelecting )
                break;

            QRect selectionRect = d->mouseSelectionRect.normalized();
            if ( selectionRect.width() <= 8 && selectionRect.height() <= 8 )
            {
                selectionClear();
                if ( d->aPrevAction )
                {
                    d->aPrevAction->trigger();
                    d->aPrevAction = nullptr;
                }
                break;
            }

            // if we support text generation
            QString selectedText;
            if (d->document->supportsSearching())
            {
                // grab text in selection by extracting it from all intersected pages
                const Okular::Page * okularPage=nullptr;
                QVector< PageViewItem * >::const_iterator iIt = d->items.constBegin(), iEnd = d->items.constEnd();
                for ( ; iIt != iEnd; ++iIt )
                {
                    PageViewItem * item = *iIt;
                    if ( !item->isVisible() )
                        continue;

                    const QRect & itemRect = item->croppedGeometry();
                    if ( selectionRect.intersects( itemRect ) )
                    {
                        // request the textpage if there isn't one
                        okularPage= item->page();
                        qCDebug(OkularUiDebug) << "checking if page" << item->pageNumber() << "has text:" << okularPage->hasTextPage();
                        if ( !okularPage->hasTextPage() )
                            d->document->requestTextPage( okularPage->number() );
                        // grab text in the rect that intersects itemRect
                        QRect relativeRect = selectionRect.intersected( itemRect );
                        relativeRect.translate( -item->uncroppedGeometry().topLeft() );
                        Okular::RegularAreaRect rects;
                        rects.append( Okular::NormalizedRect( relativeRect, item->uncroppedWidth(), item->uncroppedHeight() ) );
                        selectedText += okularPage->text( &rects );
                    }
                }
            }

            // popup that ask to copy:text and copy/save:image
            QMenu menu( this );
            menu.setObjectName("PopupMenu");
            QAction *textToClipboard = nullptr;
#ifdef HAVE_SPEECH
            QAction *speakText = nullptr;
#endif
            QAction *imageToClipboard = nullptr;
            QAction *imageToFile = nullptr;
            if ( d->document->supportsSearching() && !selectedText.isEmpty() )
            {
                menu.addAction( new OKMenuTitle( &menu, i18np( "Text (1 character)", "Text (%1 characters)", selectedText.length() ) ) );
                textToClipboard = menu.addAction( QIcon::fromTheme(QStringLiteral("edit-copy")), i18n( "Copy to Clipboard" ) );
                textToClipboard->setObjectName("CopyTextToClipboard");
                bool copyAllowed = d->document->isAllowed( Okular::AllowCopy );
                if ( !copyAllowed )
                {
                    textToClipboard->setEnabled( false );
                    textToClipboard->setText( i18n("Copy forbidden by DRM") );
                }
#ifdef HAVE_SPEECH
                if ( Okular::Settings::useTTS() )
                    speakText = menu.addAction( QIcon::fromTheme(QStringLiteral("text-speak")), i18n( "Speak Text" ) );
#endif
                if ( copyAllowed )
                {
                    addWebShortcutsMenu( &menu, selectedText );
                }
            }
            menu.addAction( new OKMenuTitle( &menu, i18n( "Image (%1 by %2 pixels)", selectionRect.width(), selectionRect.height() ) ) );
            imageToClipboard = menu.addAction( QIcon::fromTheme(QStringLiteral("image-x-generic")), i18n( "Copy to Clipboard" ) );
            imageToFile = menu.addAction( QIcon::fromTheme(QStringLiteral("document-save")), i18n( "Save to File..." ) );
            QAction *choice = menu.exec( e->globalPos() );
            // check if the user really selected an action
            if ( choice )
            {
            // IMAGE operation chosen
            if ( choice == imageToClipboard || choice == imageToFile )
            {
                // renders page into a pixmap
                QPixmap copyPix( selectionRect.width(), selectionRect.height() );
                QPainter copyPainter( &copyPix );
                copyPainter.translate( -selectionRect.left(), -selectionRect.top() );
                drawDocumentOnPainter( selectionRect, &copyPainter );
                copyPainter.end();

                if ( choice == imageToClipboard )
                {
                    // [2] copy pixmap to clipboard
                    QClipboard *cb = QApplication::clipboard();
                    cb->setPixmap( copyPix, QClipboard::Clipboard );
                    if ( cb->supportsSelection() )
                        cb->setPixmap( copyPix, QClipboard::Selection );
                    d->messageWindow->display( i18n( "Image [%1x%2] copied to clipboard.", copyPix.width(), copyPix.height() ) );
                }
                else if ( choice == imageToFile )
                {
                    // [3] save pixmap to file
                    QString fileName = QFileDialog::getSaveFileName(this, i18n("Save file"), QString(), i18n("Images (*.png *.jpeg)"));
                    if ( fileName.isEmpty() )
                        d->messageWindow->display( i18n( "File not saved." ), QString(), PageViewMessage::Warning );
                    else
                    {
                        QMimeDatabase db;
                        QMimeType mime = db.mimeTypeForUrl( QUrl::fromLocalFile(fileName) );
                        QString type;
                        if ( !mime.isDefault() )
                            type = QStringLiteral("PNG");
                        else
                            type = mime.name().section( QLatin1Char('/'), -1 ).toUpper();
                        copyPix.save( fileName, qPrintable( type ) );
                        d->messageWindow->display( i18n( "Image [%1x%2] saved to %3 file.", copyPix.width(), copyPix.height(), type ) );
                    }
                }
            }
            // TEXT operation chosen
            else
            {
                if ( choice == textToClipboard )
                {
                    // [1] copy text to clipboard
                    QClipboard *cb = QApplication::clipboard();
                    cb->setText( selectedText, QClipboard::Clipboard );
                    if ( cb->supportsSelection() )
                        cb->setText( selectedText, QClipboard::Selection );
                }
#ifdef HAVE_SPEECH
                else if ( choice == speakText )
                {
                    // [2] speech selection using TTS
                    d->tts()->say( selectedText );
                }
#endif
            }
            }
            // clear widget selection and invalidate rect
            selectionClear();

            // restore previous action if came from it using right button
            if ( d->aPrevAction )
            {
                d->aPrevAction->trigger();
                d->aPrevAction = nullptr;
            }
            }break;

        case Okular::Settings::EnumMouseMode::TableSelect:
        {
            // if it is a left release checks if is over a previous link press
            if ( leftButton && mouseReleaseOverLink ( d->mouseOverLinkObject ) ) {
                selectionClear();
                break;
            }

            // if mouse is released and selection is null this is a rightClick
            if ( rightButton && !d->mouseSelecting )
            {
                PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
                emit rightClick( pageItem ? pageItem->page() : nullptr, e->globalPos() );
                break;
            }

            QRect selectionRect = d->mouseSelectionRect.normalized();
            if ( selectionRect.width() <= 8 && selectionRect.height() <= 8 && d->tableSelectionParts.isEmpty() )
            {
                selectionClear();
                if ( d->aPrevAction )
                {
                    d->aPrevAction->trigger();
                    d->aPrevAction = nullptr;
                }
                break;
            }

            if (d->mouseSelecting) {
                // break up the selection into page-relative pieces
                d->tableSelectionParts.clear();
                const Okular::Page * okularPage=nullptr;
                QVector< PageViewItem * >::const_iterator iIt = d->items.constBegin(), iEnd = d->items.constEnd();
                for ( ; iIt != iEnd; ++iIt )
                {
                    PageViewItem * item = *iIt;
                    if ( !item->isVisible() )
                        continue;

                    const QRect & itemRect = item->croppedGeometry();
                    if ( selectionRect.intersects( itemRect ) )
                    {
                        // request the textpage if there isn't one
                        okularPage= item->page();
                        qCDebug(OkularUiDebug) << "checking if page" << item->pageNumber() << "has text:" << okularPage->hasTextPage();
                        if ( !okularPage->hasTextPage() )
                            d->document->requestTextPage( okularPage->number() );
                        // grab text in the rect that intersects itemRect
                        QRect rectInItem = selectionRect.intersected( itemRect );
                        rectInItem.translate( -item->uncroppedGeometry().topLeft() );
                        QRect rectInSelection = selectionRect.intersected( itemRect );
                        rectInSelection.translate( -selectionRect.topLeft() );
                        d->tableSelectionParts.append(
                            TableSelectionPart(
                                item,
                                Okular::NormalizedRect( rectInItem, item->uncroppedWidth(), item->uncroppedHeight() ),
                                Okular::NormalizedRect( rectInSelection, selectionRect.width(), selectionRect.height() )
                            )
                        );
                    }
                }

                QRect updatedRect = d->mouseSelectionRect.normalized().adjusted( 0, 0, 1, 1 );
                updatedRect.translate( -contentAreaPosition() );
                d->mouseSelecting = false;
                d->mouseSelectionRect.setCoords( 0, 0, 0, 0 );
                d->tableSelectionCols.clear();
                d->tableSelectionRows.clear();
                guessTableDividers();
                viewport()->update( updatedRect );
            }

            if ( !d->document->isAllowed( Okular::AllowCopy ) ) {
                d->messageWindow->display( i18n("Copy forbidden by DRM"), QString(), PageViewMessage::Info, -1 );
                break;
            }

            QString selText;
            QString selHtml;
            QList<double> xs = d->tableSelectionCols;
            QList<double> ys = d->tableSelectionRows;
            xs.prepend(0.0);
            xs.append(1.0);
            ys.prepend(0.0);
            ys.append(1.0);
            selHtml = QString::fromLatin1("<html><head>"
                      "<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">"
                      "</head><body><table>");
            for (int r=0; r+1<ys.length(); r++) {
                selHtml += QLatin1String("<tr>");
                for (int c=0; c+1<xs.length(); c++) {
                    Okular::NormalizedRect cell(xs[c], ys[r], xs[c+1], ys[r+1]);
                    if (c) selText += QLatin1Char('\t');
                    QString txt;
                    foreach (const TableSelectionPart &tsp, d->tableSelectionParts) {
                        // first, crop the cell to this part
                        if (!tsp.rectInSelection.intersects(cell))
                            continue;
                        Okular::NormalizedRect cellPart = tsp.rectInSelection & cell; // intersection

                        // second, convert it from table coordinates to part coordinates
                        cellPart.left -= tsp.rectInSelection.left;
                        cellPart.left /= (tsp.rectInSelection.right - tsp.rectInSelection.left);
                        cellPart.right -= tsp.rectInSelection.left;
                        cellPart.right /= (tsp.rectInSelection.right - tsp.rectInSelection.left);
                        cellPart.top -= tsp.rectInSelection.top;
                        cellPart.top /= (tsp.rectInSelection.bottom - tsp.rectInSelection.top);
                        cellPart.bottom -= tsp.rectInSelection.top;
                        cellPart.bottom /= (tsp.rectInSelection.bottom - tsp.rectInSelection.top);

                        // third, convert from part coordinates to item coordinates
                        cellPart.left *= (tsp.rectInItem.right - tsp.rectInItem.left);
                        cellPart.left += tsp.rectInItem.left;
                        cellPart.right *= (tsp.rectInItem.right - tsp.rectInItem.left);
                        cellPart.right += tsp.rectInItem.left;
                        cellPart.top *= (tsp.rectInItem.bottom - tsp.rectInItem.top);
                        cellPart.top += tsp.rectInItem.top;
                        cellPart.bottom *= (tsp.rectInItem.bottom - tsp.rectInItem.top);
                        cellPart.bottom += tsp.rectInItem.top;

                        // now get the text
                        Okular::RegularAreaRect rects;
                        rects.append( cellPart );
                        txt += tsp.item->page()->text( &rects, Okular::TextPage::CentralPixelTextAreaInclusionBehaviour );
                    }
                    QString html = txt;
                    selText += txt.replace(QLatin1Char('\n'), QLatin1Char(' '));
                    html.replace(QLatin1Char('&'), QLatin1String("&amp;")).replace(QLatin1Char('<'), QLatin1String("&lt;")).replace(QLatin1Char('>'), QLatin1String("&gt;"));
                    // Remove newlines, do not turn them into <br>, because
                    // Excel interprets <br> within cell as new cell...
                    html.replace(QLatin1Char('\n'), QLatin1String(" "));
                    selHtml += QStringLiteral("<td>") + html + QStringLiteral("</td>");
                }
                selText += QLatin1Char('\n');
                selHtml += QLatin1String("</tr>\n");
            }
            selHtml += QLatin1String("</table></body></html>\n");


            QClipboard *cb = QApplication::clipboard();
            QMimeData *md = new QMimeData();
            md->setText(selText);
            md->setHtml(selHtml);
            cb->setMimeData( md, QClipboard::Clipboard );
            if ( cb->supportsSelection() )
                cb->setMimeData( md, QClipboard::Selection );

        }break;
            case Okular::Settings::EnumMouseMode::TextSelect:
                // if it is a left release checks if is over a previous link press
                if ( leftButton && mouseReleaseOverLink ( d->mouseOverLinkObject ) ) {
                    selectionClear();
                    break;
                }

                if ( d->mouseTextSelecting )
                {
                    d->mouseTextSelecting = false;
//                    textSelectionClear();
                    if ( d->document->isAllowed( Okular::AllowCopy ) )
                    {
                        const QString text = d->selectedText();
                        if ( !text.isEmpty() )
                        {
                            QClipboard *cb = QApplication::clipboard();
                            if ( cb->supportsSelection() )
                                cb->setText( text, QClipboard::Selection );
                        }
                    }
                }
                else if ( !d->mousePressPos.isNull() && rightButton )
                {
                    PageViewItem* item = pickItemOnPoint(eventPos.x(),eventPos.y());
                    const Okular::Page *page;
                    //if there is text selected in the page
                    if (item)
                    {
                        QAction * httpLink = nullptr;
                        QAction * textToClipboard = nullptr;
                        QString url;

                        QMenu * menu = createProcessLinkMenu( item, eventPos );
                        const bool mouseClickOverLink = (menu != nullptr);
#ifdef HAVE_SPEECH
                        QAction *speakText = nullptr;
#endif
                        if ( (page = item->page())->textSelection() )
                        {
                            if ( !menu )
                            {
                                menu = new QMenu(this);
                            }
                            textToClipboard = menu->addAction( QIcon::fromTheme( QStringLiteral("edit-copy") ), i18n( "Copy Text" ) );

#ifdef HAVE_SPEECH
                            if ( Okular::Settings::useTTS() )
                                speakText = menu->addAction( QIcon::fromTheme( QStringLiteral("text-speak") ), i18n( "Speak Text" ) );
#endif
                            if ( !d->document->isAllowed( Okular::AllowCopy ) )
                            {
                                textToClipboard->setEnabled( false );
                                textToClipboard->setText( i18n("Copy forbidden by DRM") );
                            }
                            else
                            {
                                addWebShortcutsMenu( menu, d->selectedText() );
                            }

                            // if the right-click was over a link add "Follow This link" instead of "Go to"
                            if (!mouseClickOverLink)
                            {
                                url = UrlUtils::getUrl( d->selectedText() );
                                if ( !url.isEmpty() )
                                {
                                    const QString squeezedText = KStringHandler::rsqueeze( url, 30 );
                                    httpLink = menu->addAction( i18n( "Go to '%1'", squeezedText ) );
                                    httpLink->setObjectName("GoToAction");
                                }
                            }
                        }

                        if ( menu ) {
                            menu->setObjectName("PopupMenu");

                            QAction *choice = menu->exec( e->globalPos() );
                            // check if the user really selected an action
                            if ( choice )
                            {
                                if ( choice == textToClipboard )
                                    copyTextSelection();
#ifdef HAVE_SPEECH
                                else if ( choice == speakText )
                                {
                                    const QString text = d->selectedText();
                                    d->tts()->say( text );
                                }
#endif
                                else if ( choice == httpLink ) {
                                    new KRun( QUrl( url ), this );
                                }
                            }

                            menu->deleteLater();
                        }
                    }
                }
            break;
    }

    // reset mouse press / 'drag start' position
    d->mousePressPos = QPoint();
}

void PageView::guessTableDividers()
{
    QList< QPair<double, int> > colTicks, rowTicks, colSelectionTicks, rowSelectionTicks;

    foreach ( const TableSelectionPart& tsp, d->tableSelectionParts )
    {
        // add ticks for the edges of this area...
        colSelectionTicks.append( qMakePair( tsp.rectInSelection.left,   +1 ) );
        colSelectionTicks.append( qMakePair( tsp.rectInSelection.right,  -1 ) );
        rowSelectionTicks.append( qMakePair( tsp.rectInSelection.top,    +1 ) );
        rowSelectionTicks.append( qMakePair( tsp.rectInSelection.bottom, -1 ) );

        // get the words in this part
        Okular::RegularAreaRect rects;
        rects.append( tsp.rectInItem );
        const Okular::TextEntity::List words = tsp.item->page()->words( &rects, Okular::TextPage::CentralPixelTextAreaInclusionBehaviour );

        foreach (Okular::TextEntity *te, words)
        {
            if (te->text().isEmpty()) {
                delete te;
                continue;
            }

            Okular::NormalizedRect wordArea = *te->area();

            // convert it from item coordinates to part coordinates
            wordArea.left -= tsp.rectInItem.left;
            wordArea.left /= (tsp.rectInItem.right - tsp.rectInItem.left);
            wordArea.right -= tsp.rectInItem.left;
            wordArea.right /= (tsp.rectInItem.right - tsp.rectInItem.left);
            wordArea.top -= tsp.rectInItem.top;
            wordArea.top /= (tsp.rectInItem.bottom - tsp.rectInItem.top);
            wordArea.bottom -= tsp.rectInItem.top;
            wordArea.bottom /= (tsp.rectInItem.bottom - tsp.rectInItem.top);

            // convert from part coordinates to table coordinates
            wordArea.left *= (tsp.rectInSelection.right - tsp.rectInSelection.left);
            wordArea.left += tsp.rectInSelection.left;
            wordArea.right *= (tsp.rectInSelection.right - tsp.rectInSelection.left);
            wordArea.right += tsp.rectInSelection.left;
            wordArea.top *= (tsp.rectInSelection.bottom - tsp.rectInSelection.top);
            wordArea.top += tsp.rectInSelection.top;
            wordArea.bottom *= (tsp.rectInSelection.bottom - tsp.rectInSelection.top);
            wordArea.bottom += tsp.rectInSelection.top;

            // add to the ticks arrays...
            colTicks.append( qMakePair( wordArea.left,   +1) );
            colTicks.append( qMakePair( wordArea.right,  -1) );
            rowTicks.append( qMakePair( wordArea.top,    +1) );
            rowTicks.append( qMakePair( wordArea.bottom, -1) );

            delete te;
        }
    }

    int tally = 0;

    qSort( colSelectionTicks );
    qSort( rowSelectionTicks );

    for (int i = 0; i < colSelectionTicks.length(); ++i)
    {
        tally += colSelectionTicks[i].second;
        if ( tally == 0 && i + 1 < colSelectionTicks.length() && colSelectionTicks[i+1].first != colSelectionTicks[i].first)
        {
            colTicks.append( qMakePair( colSelectionTicks[i].first,   +1 ) );
            colTicks.append( qMakePair( colSelectionTicks[i+1].first, -1 ) );
        }
    }
    Q_ASSERT( tally == 0 );

    for (int i = 0; i < rowSelectionTicks.length(); ++i)
    {
        tally += rowSelectionTicks[i].second;
        if ( tally == 0 && i + 1 < rowSelectionTicks.length() && rowSelectionTicks[i+1].first != rowSelectionTicks[i].first) {
            rowTicks.append( qMakePair( rowSelectionTicks[i].first,   +1 ) );
            rowTicks.append( qMakePair( rowSelectionTicks[i+1].first, -1 ) );
        }
    }
    Q_ASSERT( tally == 0 );

    qSort( colTicks );
    qSort( rowTicks );

    for (int i = 0; i < colTicks.length(); ++i)
    {
        tally += colTicks[i].second;
        if ( tally == 0 && i + 1 < colTicks.length() && colTicks[i+1].first != colTicks[i].first)
        {
            d->tableSelectionCols.append( (colTicks[i].first+colTicks[i+1].first) / 2 );
            d->tableDividersGuessed = true;
        }
    }
    Q_ASSERT( tally == 0 );

    for (int i = 0; i < rowTicks.length(); ++i)
    {
        tally += rowTicks[i].second;
        if ( tally == 0 && i + 1 < rowTicks.length() && rowTicks[i+1].first != rowTicks[i].first)
        {
            d->tableSelectionRows.append( (rowTicks[i].first+rowTicks[i+1].first) / 2 );
            d->tableDividersGuessed = true;
        }
    }
    Q_ASSERT( tally == 0 );
}

void PageView::mouseDoubleClickEvent( QMouseEvent * e )
{
    d->controlWheelAccumulatedDelta = 0;

    if ( e->button() == Qt::LeftButton )
    {
        const QPoint eventPos = contentAreaPoint( e->pos() );
        PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
        if ( pageItem )
        {
            // find out normalized mouse coords inside current item
            double nX = pageItem->absToPageX(eventPos.x());
            double nY = pageItem->absToPageY(eventPos.y());

            if ( d->mouseMode == Okular::Settings::EnumMouseMode::TextSelect ) {
                textSelectionClear();

                Okular::RegularAreaRect *wordRect = pageItem->page()->wordAt( Okular::NormalizedPoint( nX, nY ) );
                if ( wordRect )
                {
                    // TODO words with hyphens across pages
                    d->document->setPageTextSelection( pageItem->pageNumber(), wordRect, palette().color( QPalette::Active, QPalette::Highlight ) );
                    d->pagesWithTextSelection << pageItem->pageNumber();
                    if ( d->document->isAllowed( Okular::AllowCopy ) )
                    {
                        const QString text = d->selectedText();
                        if ( !text.isEmpty() )
                        {
                            QClipboard *cb = QApplication::clipboard();
                            if ( cb->supportsSelection() )
                                cb->setText( text, QClipboard::Selection );
                        }
                    }
                    return;
                }
            }

            const QRect & itemRect = pageItem->uncroppedGeometry();
            Okular::Annotation * ann = nullptr;
            const Okular::ObjectRect * orect = pageItem->page()->objectRect( Okular::ObjectRect::OAnnotation, nX, nY, itemRect.width(), itemRect.height() );
            if ( orect )
                ann = ( (Okular::AnnotationObjectRect *)orect )->annotation();
            if ( ann && ann->subType() != Okular::Annotation::AWidget )
            {
                openAnnotationWindow( ann, pageItem->pageNumber() );
            }
        }
    }
}

void PageView::wheelEvent( QWheelEvent *e )
{
    // don't perform any mouse action when viewport is autoscrolling
    if ( d->viewportMoveActive )
        return;

    if ( !d->document->isOpened() )
    {
        QAbstractScrollArea::wheelEvent( e );
        return;
    }

    int delta = e->delta(),
        vScroll = verticalScrollBar()->value();
    e->accept();
    if ( (e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier ) {
        d->controlWheelAccumulatedDelta += delta;
        if ( d->controlWheelAccumulatedDelta <= -QWheelEvent::DefaultDeltasPerStep )
        {
            slotZoomOut();
            d->controlWheelAccumulatedDelta = 0;
        }
        else if ( d->controlWheelAccumulatedDelta >= QWheelEvent::DefaultDeltasPerStep )
        {
            slotZoomIn();
            d->controlWheelAccumulatedDelta = 0;
        }
    }
    else
    {
        d->controlWheelAccumulatedDelta = 0;

        if ( delta <= -QWheelEvent::DefaultDeltasPerStep && !Okular::Settings::viewContinuous() && vScroll == verticalScrollBar()->maximum() )
        {
            // go to next page
            if ( (int)d->document->currentPage() < d->items.count() - 1 )
            {
                // more optimized than document->setNextPage and then move view to top
                Okular::DocumentViewport newViewport = d->document->viewport();
                newViewport.pageNumber += viewColumns();
                if ( newViewport.pageNumber >= (int)d->items.count() )
                    newViewport.pageNumber = d->items.count() - 1;
                newViewport.rePos.enabled = true;
                newViewport.rePos.normalizedY = 0.0;
                d->document->setViewport( newViewport );
            }
        }
        else if ( delta >= QWheelEvent::DefaultDeltasPerStep && !Okular::Settings::viewContinuous() && vScroll == verticalScrollBar()->minimum() )
        {
            // go to prev page
            if ( d->document->currentPage() > 0 )
            {
                // more optimized than document->setPrevPage and then move view to bottom
                Okular::DocumentViewport newViewport = d->document->viewport();
                newViewport.pageNumber -= viewColumns();
                if ( newViewport.pageNumber < 0 )
                    newViewport.pageNumber = 0;
                newViewport.rePos.enabled = true;
                newViewport.rePos.normalizedY = 1.0;
                d->document->setViewport( newViewport );
            }
        }
        else
            QAbstractScrollArea::wheelEvent( e );
    }

    updateCursor();
}

bool PageView::viewportEvent( QEvent * e )
{
    if ( e->type() == QEvent::ToolTip && d->mouseMode == Okular::Settings::EnumMouseMode::Browse )
    {
        QHelpEvent * he = static_cast< QHelpEvent* >( e );
        if ( d->mouseAnnotation->isMouseOver() )
        {
            d->mouseAnnotation->routeTooltipEvent( he );
        }
        else
        {
            const QPoint eventPos = contentAreaPoint( he->pos() );
            PageViewItem * pageItem = pickItemOnPoint( eventPos.x(), eventPos.y() );
            const Okular::ObjectRect * rect = nullptr;
            const Okular::Action * link = nullptr;
            if ( pageItem )
            {
                double nX = pageItem->absToPageX( eventPos.x() );
                double nY = pageItem->absToPageY( eventPos.y() );
                rect = pageItem->page()->objectRect( Okular::ObjectRect::Action, nX, nY, pageItem->uncroppedWidth(), pageItem->uncroppedHeight() );
                if ( rect )
                    link = static_cast< const Okular::Action * >( rect->object() );
            }

            if ( link )
            {
                QRect r = rect->boundingRect( pageItem->uncroppedWidth(), pageItem->uncroppedHeight() );
                r.translate( pageItem->uncroppedGeometry().topLeft() );
                r.translate( -contentAreaPosition() );
                QString tip = link->actionTip();
                if ( !tip.isEmpty() )
                    QToolTip::showText( he->globalPos(), tip, viewport(), r );
            }
        }
        e->accept();
        return true;
    }
    else
        // do not stop the event
        return QAbstractScrollArea::viewportEvent( e );
}

void PageView::scrollContentsBy( int dx, int dy )
{
    const QRect r = viewport()->rect();
    viewport()->scroll( dx, dy, r );
    // HACK manually repaint the damaged regions, as it seems some updates are missed
    // thus leaving artifacts around
    QRegion rgn( r );
    rgn -= rgn & r.translated( dx, dy );
    foreach ( const QRect &rect, rgn.rects() )
        viewport()->repaint( rect );
}
//END widget events

QList< Okular::RegularAreaRect * > PageView::textSelections( const QPoint& start, const QPoint& end, int& firstpage )
{
    firstpage = -1;
    QList< Okular::RegularAreaRect * > ret;
    QSet< int > affectedItemsSet;
    QRect selectionRect = QRect( start, end ).normalized();
    foreach( PageViewItem * item, d->items )
    {
        if ( item->isVisible() && selectionRect.intersects( item->croppedGeometry() ) )
            affectedItemsSet.insert( item->pageNumber() );
    }
#ifdef PAGEVIEW_DEBUG
    qCDebug(OkularUiDebug) << ">>>> item selected by mouse:" << affectedItemsSet.count();
#endif

    if ( !affectedItemsSet.isEmpty() )
    {
        // is the mouse drag line the ne-sw diagonal of the selection rect?
        bool direction_ne_sw = start == selectionRect.topRight() || start == selectionRect.bottomLeft();

        int tmpmin = d->document->pages();
        int tmpmax = 0;
        foreach( int p, affectedItemsSet )
        {
            if ( p < tmpmin ) tmpmin = p;
            if ( p > tmpmax ) tmpmax = p;
        }

        PageViewItem * a = pickItemOnPoint( (int)( direction_ne_sw ? selectionRect.right() : selectionRect.left() ), (int)selectionRect.top() );
        int min = a && ( a->pageNumber() != tmpmax ) ? a->pageNumber() : tmpmin;
        PageViewItem * b = pickItemOnPoint( (int)( direction_ne_sw ? selectionRect.left() : selectionRect.right() ), (int)selectionRect.bottom() );
        int max = b && ( b->pageNumber() != tmpmin ) ? b->pageNumber() : tmpmax;

        QList< int > affectedItemsIds;
        for ( int i = min; i <= max; ++i )
            affectedItemsIds.append( i );
#ifdef PAGEVIEW_DEBUG
        qCDebug(OkularUiDebug) << ">>>> pages:" << affectedItemsIds;
#endif
        firstpage = affectedItemsIds.first();

        if ( affectedItemsIds.count() == 1 )
        {
            PageViewItem * item = d->items[ affectedItemsIds.first() ];
            selectionRect.translate( -item->uncroppedGeometry().topLeft() );
            ret.append( textSelectionForItem( item,
                direction_ne_sw ? selectionRect.topRight() : selectionRect.topLeft(),
                direction_ne_sw ? selectionRect.bottomLeft() : selectionRect.bottomRight() ) );
        }
        else if ( affectedItemsIds.count() > 1 )
        {
            // first item
            PageViewItem * first = d->items[ affectedItemsIds.first() ];
            QRect geom = first->croppedGeometry().intersected( selectionRect ).translated( -first->uncroppedGeometry().topLeft() );
            ret.append( textSelectionForItem( first,
                selectionRect.bottom() > geom.height() ? ( direction_ne_sw ? geom.topRight() : geom.topLeft() ) : ( direction_ne_sw ? geom.bottomRight() : geom.bottomLeft() ),
                QPoint() ) );
            // last item
            PageViewItem * last = d->items[ affectedItemsIds.last() ];
            geom = last->croppedGeometry().intersected( selectionRect ).translated( -last->uncroppedGeometry().topLeft() );
            // the last item needs to appended at last...
            Okular::RegularAreaRect * lastArea = textSelectionForItem( last,
                QPoint(),
                selectionRect.bottom() > geom.height() ? ( direction_ne_sw ? geom.bottomLeft() : geom.bottomRight() ) : ( direction_ne_sw ? geom.topLeft() : geom.topRight() ) );
            affectedItemsIds.removeFirst();
            affectedItemsIds.removeLast();
            // item between the two above
            foreach( int page, affectedItemsIds )
            {
                ret.append( textSelectionForItem( d->items[ page ] ) );
            }
            ret.append( lastArea );
        }
    }
    return ret;
}


void PageView::drawDocumentOnPainter( const QRect & contentsRect, QPainter * p )
{
    QColor backColor;

    if ( Okular::Settings::useCustomBackgroundColor() )
        backColor = Okular::Settings::backgroundColor();
    else
        backColor = viewport()->palette().color( QPalette::Dark );

    // when checking if an Item is contained in contentsRect, instead of
    // growing PageViewItems rects (for keeping outline into account), we
    // grow the contentsRect
    QRect checkRect = contentsRect;
    checkRect.adjust( -3, -3, 1, 1 );

    // create a region from which we'll subtract painted rects
    QRegion remainingArea( contentsRect );

    // iterate over all items painting the ones intersecting contentsRect
    QVector< PageViewItem * >::const_iterator iIt = d->items.constBegin(), iEnd = d->items.constEnd();
    for ( ; iIt != iEnd; ++iIt )
    {
        // check if a piece of the page intersects the contents rect
        if ( !(*iIt)->isVisible() || !(*iIt)->croppedGeometry().intersects( checkRect ) )
            continue;

        // get item and item's outline geometries
        PageViewItem * item = *iIt;
        QRect itemGeometry = item->croppedGeometry(),
              outlineGeometry = itemGeometry;
        outlineGeometry.adjust( -1, -1, 3, 3 );

        // move the painter to the top-left corner of the real page
        p->save();
        p->translate( itemGeometry.left(), itemGeometry.top() );

        // draw the page outline (black border and 2px bottom-right shadow)
        if ( !itemGeometry.contains( contentsRect ) )
        {
            int itemWidth = itemGeometry.width(),
                itemHeight = itemGeometry.height();
            // draw simple outline
            p->setPen( Qt::black );
            p->drawRect( -1, -1, itemWidth + 1, itemHeight + 1 );
            // draw bottom/right gradient
            static const int levels = 2;
            int r = backColor.red() / (levels + 2) + 6,
                g = backColor.green() / (levels + 2) + 6,
                b = backColor.blue() / (levels + 2) + 6;
            for ( int i = 0; i < levels; i++ )
            {
                p->setPen( QColor( r * (i+2), g * (i+2), b * (i+2) ) );
                p->drawLine( i, i + itemHeight + 1, i + itemWidth + 1, i + itemHeight + 1 );
                p->drawLine( i + itemWidth + 1, i, i + itemWidth + 1, i + itemHeight );
                p->setPen( backColor );
                p->drawLine( -1, i + itemHeight + 1, i - 1, i + itemHeight + 1 );
                p->drawLine( i + itemWidth + 1, -1, i + itemWidth + 1, i - 1 );
            }
        }

        // draw the page using the PagePainter with all flags active
        if ( contentsRect.intersects( itemGeometry ) )
        {
            Okular::NormalizedPoint *viewPortPoint = nullptr;
            Okular::NormalizedPoint point( d->lastSourceLocationViewportNormalizedX, d->lastSourceLocationViewportNormalizedY );
            if( Okular::Settings::showSourceLocationsGraphically()
                && item->pageNumber() ==  d->lastSourceLocationViewportPageNumber )
            {
                viewPortPoint = &point;
            }
            QRect pixmapRect = contentsRect.intersected( itemGeometry );
            pixmapRect.translate( -item->croppedGeometry().topLeft() );
            PagePainter::paintCroppedPageOnPainter( p, item->page(), this, pageflags,
                item->uncroppedWidth(), item->uncroppedHeight(), pixmapRect,
                item->crop(), viewPortPoint );
        }

        // remove painted area from 'remainingArea' and restore painter
        remainingArea -= outlineGeometry.intersected( contentsRect );
        p->restore();
    }

    // fill with background color the unpainted area
    const QVector<QRect> &backRects = remainingArea.rects();
    int backRectsNumber = backRects.count();
    for ( int jr = 0; jr < backRectsNumber; jr++ )
        p->fillRect( backRects[ jr ], backColor );
}

void PageView::updateItemSize( PageViewItem * item, int colWidth, int rowHeight )
{
    const Okular::Page * okularPage = item->page();
    double width = okularPage->width(),
           height = okularPage->height(),
           zoom = d->zoomFactor;
    Okular::NormalizedRect crop( 0., 0., 1., 1. );

    // Handle cropping, due to either "Trim Margin" or "Trim to Selection" cases
    if (( Okular::Settings::trimMargins() && okularPage->isBoundingBoxKnown()
         && !okularPage->boundingBox().isNull() ) ||
        ( d->aTrimToSelection && d->aTrimToSelection->isChecked() &&  !d->trimBoundingBox.isNull()))
    {

        crop = Okular::Settings::trimMargins() ? okularPage->boundingBox() : d->trimBoundingBox;

        // Rotate the bounding box
        for ( int i = okularPage->rotation(); i > 0; --i )
        {
            Okular::NormalizedRect rot = crop;
            crop.left   = 1 - rot.bottom;
            crop.top    = rot.left;
            crop.right  = 1 - rot.top;
            crop.bottom = rot.right;
        }

        // Expand the crop slightly beyond the bounding box (for Trim Margins only)
        if (Okular::Settings::trimMargins()) {
            static const double cropExpandRatio = 0.04;
            const double cropExpand = cropExpandRatio * ( (crop.right-crop.left) + (crop.bottom-crop.top) ) / 2;
            crop = Okular::NormalizedRect(
                crop.left - cropExpand,
                crop.top - cropExpand,
                crop.right + cropExpand,
                crop.bottom + cropExpand ) & Okular::NormalizedRect( 0, 0, 1, 1 );
        }

        // We currently generate a larger image and then crop it, so if the
        // crop rect is very small the generated image is huge. Hence, we shouldn't
        // let the crop rect become too small.
        static double minCropRatio;
        if (Okular::Settings::trimMargins()) {
            // Make sure we crop by at most 50% in either dimension:
            minCropRatio = 0.5;
        } else {
            // Looser Constraint for "Trim Selection"
            minCropRatio = 0.20;
        }
        if ( ( crop.right - crop.left ) < minCropRatio )
        {
            const double newLeft = ( crop.left + crop.right ) / 2 - minCropRatio/2;
            crop.left = qMax( 0.0, qMin( 1.0 - minCropRatio, newLeft ) );
            crop.right = crop.left + minCropRatio;
        }
        if ( ( crop.bottom - crop.top ) < minCropRatio )
        {
            const double newTop = ( crop.top + crop.bottom ) / 2 - minCropRatio/2;
            crop.top = qMax( 0.0, qMin( 1.0 - minCropRatio, newTop ) );
            crop.bottom = crop.top + minCropRatio;
        }

        width *= ( crop.right - crop.left );
        height *= ( crop.bottom - crop.top );
#ifdef PAGEVIEW_DEBUG
        qCDebug(OkularUiDebug) << "Cropped page" << okularPage->number() << "to" << crop
                 << "width" << width << "height" << height << "by bbox" << okularPage->boundingBox();
#endif
    }

    if ( d->zoomMode == ZoomFixed )
    {
        width *= zoom;
        height *= zoom;
        item->setWHZC( (int)width, (int)height, d->zoomFactor, crop );
    }
    else if ( d->zoomMode == ZoomFitWidth )
    {
        height = ( height / width ) * colWidth;
        zoom = (double)colWidth / width;
        item->setWHZC( colWidth, (int)height, zoom, crop );
        if ((uint)item->pageNumber() == d->document->currentPage())
            d->zoomFactor = zoom;
    }
    else if ( d->zoomMode == ZoomFitPage )
    {
        const double scaleW = (double)colWidth / (double)width;
        const double scaleH = (double)rowHeight / (double)height;
        zoom = qMin( scaleW, scaleH );
        item->setWHZC( (int)(zoom * width), (int)(zoom * height), zoom, crop );
        if ((uint)item->pageNumber() == d->document->currentPage())
            d->zoomFactor = zoom;
    }
    else if ( d->zoomMode == ZoomFitAuto )
    {
        const double aspectRatioRelation = 1.25; // relation between aspect ratios for "auto fit"
        const double uiAspect = (double)rowHeight / (double)colWidth;
        const double pageAspect = (double)height / (double)width;
        const double rel = uiAspect / pageAspect;

        const bool isContinuous = Okular::Settings::viewContinuous();
        if ( !isContinuous && rel > aspectRatioRelation )
        {
            // UI space is relatively much higher than the page
            zoom = (double)rowHeight / (double)height;
        }
        else if ( rel < 1.0 / aspectRatioRelation )
        {
            // UI space is relatively much wider than the page in relation
            zoom = (double)colWidth / (double)width;
        }
        else
        {
            // aspect ratios of page and UI space are very similar
            const double scaleW = (double)colWidth / (double)width;
            const double scaleH = (double)rowHeight / (double)height;
            zoom = qMin( scaleW, scaleH );
        }
        item->setWHZC( (int)(zoom * width), (int)(zoom * height), zoom, crop );
        if ((uint)item->pageNumber() == d->document->currentPage())
            d->zoomFactor = zoom;
    }
#ifndef NDEBUG
    else
        qCDebug(OkularUiDebug) << "calling updateItemSize with unrecognized d->zoomMode!";
#endif
}

PageViewItem * PageView::pickItemOnPoint( int x, int y )
{
    PageViewItem * item = nullptr;
    QLinkedList< PageViewItem * >::const_iterator iIt = d->visibleItems.constBegin(), iEnd = d->visibleItems.constEnd();
    for ( ; iIt != iEnd; ++iIt )
    {
        PageViewItem * i = *iIt;
        const QRect & r = i->croppedGeometry();
        if ( x < r.right() && x > r.left() && y < r.bottom() )
        {
            if ( y > r.top() )
                item = i;
            break;
        }
    }
    return item;
}

void PageView::textSelectionClear()
{
   // something to clear
    if ( !d->pagesWithTextSelection.isEmpty() )
    {
        QSet< int >::ConstIterator it = d->pagesWithTextSelection.constBegin(), itEnd = d->pagesWithTextSelection.constEnd();
        for ( ; it != itEnd; ++it )
            d->document->setPageTextSelection( *it, nullptr, QColor() );
        d->pagesWithTextSelection.clear();
    }
}

void PageView::selectionStart( const QPoint & pos, const QColor & color, bool /*aboveAll*/ )
{
    selectionClear();
    d->mouseSelecting = true;
    d->mouseSelectionRect.setRect( pos.x(), pos.y(), 1, 1 );
    d->mouseSelectionColor = color;
    // ensures page doesn't scroll
    if ( d->autoScrollTimer )
    {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }
}

void PageView::scrollPosIntoView( const QPoint & pos )
{
    if (pos.x() < horizontalScrollBar()->value()) d->dragScrollVector.setX(pos.x() - horizontalScrollBar()->value());
    else if (horizontalScrollBar()->value() + viewport()->width() < pos.x()) d->dragScrollVector.setX(pos.x() - horizontalScrollBar()->value() - viewport()->width());
    else d->dragScrollVector.setX(0);

    if (pos.y() < verticalScrollBar()->value()) d->dragScrollVector.setY(pos.y() - verticalScrollBar()->value());
    else if (verticalScrollBar()->value() + viewport()->height() < pos.y()) d->dragScrollVector.setY(pos.y() - verticalScrollBar()->value() - viewport()->height());
    else d->dragScrollVector.setY(0);

    if (d->dragScrollVector != QPoint(0, 0))
    {
        if (!d->dragScrollTimer.isActive()) d->dragScrollTimer.start(100);
    }
    else d->dragScrollTimer.stop();
}

void PageView::updateSelection( const QPoint & pos )
{
    if ( d->mouseSelecting )
    {
        scrollPosIntoView( pos );
        // update the selection rect
        QRect updateRect = d->mouseSelectionRect;
        d->mouseSelectionRect.setBottomLeft( pos );
        updateRect |= d->mouseSelectionRect;
        updateRect.translate( -contentAreaPosition() );
        viewport()->update( updateRect.adjusted( -1, -2, 2, 1 ) );
    }
    else if ( d->mouseTextSelecting)
    {
        scrollPosIntoView( pos );
        int first = -1;
        const QList< Okular::RegularAreaRect * > selections = textSelections( pos, d->mouseSelectPos, first );
        QSet< int > pagesWithSelectionSet;
        for ( int i = 0; i < selections.count(); ++i )
            pagesWithSelectionSet.insert( i + first );

        const QSet< int > noMoreSelectedPages = d->pagesWithTextSelection - pagesWithSelectionSet;
        // clear the selection from pages not selected anymore
        foreach( int p, noMoreSelectedPages )
        {
            d->document->setPageTextSelection( p, nullptr, QColor() );
        }
        // set the new selection for the selected pages
        foreach( int p, pagesWithSelectionSet )
        {
            d->document->setPageTextSelection( p, selections[ p - first ], palette().color( QPalette::Active, QPalette::Highlight ) );
        }
        d->pagesWithTextSelection = pagesWithSelectionSet;
    }
}

static Okular::NormalizedPoint rotateInNormRect( const QPoint &rotated, const QRect &rect, Okular::Rotation rotation )
{
    Okular::NormalizedPoint ret;

    switch ( rotation )
    {
        case Okular::Rotation0:
            ret = Okular::NormalizedPoint( rotated.x(), rotated.y(), rect.width(), rect.height() );
            break;
        case Okular::Rotation90:
            ret = Okular::NormalizedPoint( rotated.y(), rect.width() - rotated.x(), rect.height(), rect.width() );
            break;
        case Okular::Rotation180:
            ret = Okular::NormalizedPoint( rect.width() - rotated.x(), rect.height() - rotated.y(), rect.width(), rect.height() );
            break;
        case Okular::Rotation270:
            ret = Okular::NormalizedPoint( rect.height() - rotated.y(), rotated.x(), rect.height(), rect.width() );
            break;
    }

    return ret;
}

Okular::RegularAreaRect * PageView::textSelectionForItem( PageViewItem * item, const QPoint & startPoint, const QPoint & endPoint )
{
    const QRect & geometry = item->uncroppedGeometry();
    Okular::NormalizedPoint startCursor( 0.0, 0.0 );
    if ( !startPoint.isNull() )
    {
        startCursor = rotateInNormRect( startPoint, geometry, item->page()->rotation() );
    }
    Okular::NormalizedPoint endCursor( 1.0, 1.0 );
    if ( !endPoint.isNull() )
    {
        endCursor = rotateInNormRect( endPoint, geometry, item->page()->rotation() );
    }
    Okular::TextSelection mouseTextSelectionInfo( startCursor, endCursor );

    const Okular::Page * okularPage = item->page();

    if ( !okularPage->hasTextPage() )
        d->document->requestTextPage( okularPage->number() );

    Okular::RegularAreaRect * selectionArea = okularPage->textArea( &mouseTextSelectionInfo );
#ifdef PAGEVIEW_DEBUG
    qCDebug(OkularUiDebug).nospace() << "text areas (" << okularPage->number() << "): " << ( selectionArea ? QString::number( selectionArea->count() ) : "(none)" );
#endif
    return selectionArea;
}

void PageView::selectionClear(const ClearMode mode)
{
    QRect updatedRect = d->mouseSelectionRect.normalized().adjusted( -2, -2, 2, 2 );
    d->mouseSelecting = false;
    d->mouseSelectionRect.setCoords( 0, 0, 0, 0 );
    d->tableSelectionCols.clear();
    d->tableSelectionRows.clear();
    d->tableDividersGuessed = false;
    foreach (const TableSelectionPart &tsp, d->tableSelectionParts) {
        QRect selectionPartRect = tsp.rectInItem.geometry(tsp.item->uncroppedWidth(), tsp.item->uncroppedHeight());
        selectionPartRect.translate( tsp.item->uncroppedGeometry().topLeft () );
        // should check whether this is on-screen here?
        updatedRect = updatedRect.united(selectionPartRect);
    }
    if ( mode != ClearOnlyDividers ) {
        d->tableSelectionParts.clear();
    }
    d->tableSelectionParts.clear();
    updatedRect.translate( -contentAreaPosition() );
    viewport()->update( updatedRect );
}

// const to be used for both zoomFactorFitMode function and slotRelayoutPages.
static const int kcolWidthMargin = 6;
static const int krowHeightMargin = 12;

double PageView::zoomFactorFitMode( ZoomMode mode )
{
    const int pageCount = d->items.count();
    if ( pageCount == 0 )
        return 0;
    const bool facingCentered = Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::FacingFirstCentered || (Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::Facing && pageCount == 1);
    const bool overrideCentering = facingCentered && pageCount < 3;
    const int nCols = overrideCentering ? 1 : viewColumns();
    const double colWidth = viewport()->width() / nCols - kcolWidthMargin;
    const double rowHeight = viewport()->height() - krowHeightMargin;
    const PageViewItem * currentItem = d->items[ qMax( 0, (int)d->document->currentPage()) ];
    // prevent segmentation fault when openning a new document;
    if ( !currentItem )
        return 0;
    const Okular::Page * okularPage = currentItem->page();
    const double width = okularPage->width(), height = okularPage->height();
    if ( mode == ZoomFitWidth )
        return (double) colWidth / width;
    if ( mode == ZoomFitPage )
    {
        const double scaleW = (double) colWidth / (double)width;
        const double scaleH = (double) rowHeight / (double)height;
        return qMin(scaleW, scaleH);
    }
    return 0;
}

void PageView::updateZoom( ZoomMode newZoomMode )
{
    if ( newZoomMode == ZoomFixed )
    {
        if ( d->aZoom->currentItem() == 0 )
            newZoomMode = ZoomFitWidth;
        else if ( d->aZoom->currentItem() == 1 )
            newZoomMode = ZoomFitPage;
        else if ( d->aZoom->currentItem() == 2 )
            newZoomMode = ZoomFitAuto;
    }

    float newFactor = d->zoomFactor;
    QAction * checkedZoomAction = nullptr;
    switch ( newZoomMode )
    {
        case ZoomFixed:{ //ZoomFixed case
            QString z = d->aZoom->currentText();
            // kdelibs4 sometimes adds accelerators to actions' text directly :(
            z.remove (QLatin1Char('&'));
            z.remove (QLatin1Char('%'));
            newFactor = QLocale().toDouble( z ) / 100.0;
            }break;
        case ZoomIn:
        case ZoomOut:{
            const float zoomFactorFitWidth = zoomFactorFitMode(ZoomFitWidth);
            const float zoomFactorFitPage = zoomFactorFitMode(ZoomFitPage);
            QVector<float> zoomValue(15);
            qCopy(kZoomValues, kZoomValues + 13, zoomValue.begin());
            zoomValue[13] = zoomFactorFitWidth;
            zoomValue[14] = zoomFactorFitPage;
            qSort(zoomValue.begin(), zoomValue.end());
            QVector<float>::iterator i;
            if ( newZoomMode == ZoomOut )
            {
                if (newFactor <= zoomValue.first())
                    return;
                i = qLowerBound(zoomValue.begin(), zoomValue.end(), newFactor) - 1;
            }
            else
            {
                if (newFactor >= zoomValue.last())
                    return;
                i = qUpperBound(zoomValue.begin(), zoomValue.end(), newFactor);
            }
            const float tmpFactor = *i;
            if ( tmpFactor == zoomFactorFitWidth )
            {
                newZoomMode = ZoomFitWidth;
                checkedZoomAction = d->aZoomFitWidth;
            }
            else if ( tmpFactor == zoomFactorFitPage )
            {
                newZoomMode = ZoomFitPage;
                checkedZoomAction = d->aZoomFitPage;
            }
            else
            {
                newFactor = tmpFactor;
                newZoomMode = ZoomFixed;
            }
            }
            break;
        case ZoomFitWidth:
            checkedZoomAction = d->aZoomFitWidth;
            break;
        case ZoomFitPage:
            checkedZoomAction = d->aZoomFitPage;
            break;
        case ZoomFitAuto:
            checkedZoomAction = d->aZoomAutoFit;
            break;
        case ZoomRefreshCurrent:
            newZoomMode = ZoomFixed;
            d->zoomFactor = -1;
            break;
    }
    const float upperZoomLimit = d->document->supportsTiles() ? 16.0 : 4.0;
    if ( newFactor > upperZoomLimit )
        newFactor = upperZoomLimit;
    if ( newFactor < 0.1 )
        newFactor = 0.1;

    if ( newZoomMode != d->zoomMode || (newZoomMode == ZoomFixed && newFactor != d->zoomFactor ) )
    {
        // rebuild layout and update the whole viewport
        d->zoomMode = newZoomMode;
        d->zoomFactor = newFactor;
        // be sure to block updates to document's viewport
        bool prevState = d->blockViewport;
        d->blockViewport = true;
        slotRelayoutPages();
        d->blockViewport = prevState;
        // request pixmaps
        slotRequestVisiblePixmaps();
        // update zoom text
        updateZoomText();
        // update actions checked state
        if ( d->aZoomFitWidth )
        {
            d->aZoomFitWidth->setChecked( checkedZoomAction == d->aZoomFitWidth );
            d->aZoomFitPage->setChecked( checkedZoomAction == d->aZoomFitPage );
            d->aZoomAutoFit->setChecked( checkedZoomAction == d->aZoomAutoFit );
        }
    }
    else if ( newZoomMode == ZoomFixed && newFactor == d->zoomFactor )
        updateZoomText();

    d->aZoomIn->setEnabled( d->zoomFactor < upperZoomLimit-0.001 );
    d->aZoomOut->setEnabled( d->zoomFactor > 0.101 );
}

void PageView::updateZoomText()
{
    // use current page zoom as zoomFactor if in ZoomFit/* mode
    if ( d->zoomMode != ZoomFixed && d->items.count() > 0 )
        d->zoomFactor = d->items[ qMax( 0, (int)d->document->currentPage() ) ]->zoomFactor();
    float newFactor = d->zoomFactor;
    d->aZoom->removeAllActions();

    // add items that describe fit actions
    QStringList translated;
    translated << i18n("Fit Width") << i18n("Fit Page") << i18n("Auto Fit");

    // add percent items
    int idx = 0, selIdx = 3;
    bool inserted = false; //use: "d->zoomMode != ZoomFixed" to hide Fit/* zoom ratio
    int zoomValueCount = 11;
    if ( d->document->supportsTiles() )
        zoomValueCount = 13;
    while ( idx < zoomValueCount || !inserted )
    {
        float value = idx < zoomValueCount ? kZoomValues[ idx ] : newFactor;
        if ( !inserted && newFactor < (value - 0.0001) )
            value = newFactor;
        else
            idx ++;
        if ( value > (newFactor - 0.0001) && value < (newFactor + 0.0001) )
            inserted = true;
        if ( !inserted )
            selIdx++;
        // we do not need to display 2-digit precision
        QString localValue( QLocale().toString( value * 100.0, 'f', 1 ) );
        localValue.remove( QLocale().decimalPoint() + QLatin1Char('0') );
        // remove a trailing zero in numbers like 66.70
        if ( localValue.right( 1 ) == QLatin1String( "0" ) && localValue.indexOf( QLocale().decimalPoint() ) > -1 )
            localValue.chop( 1 );
        translated << QStringLiteral( "%1%" ).arg( localValue );
    }
    d->aZoom->setItems( translated );

    // select current item in list
    if ( d->zoomMode == ZoomFitWidth )
        selIdx = 0;
    else if ( d->zoomMode == ZoomFitPage )
        selIdx = 1;
    else if ( d->zoomMode == ZoomFitAuto )
        selIdx = 2;
    // we have to temporarily enable the actions as otherwise we can't set a new current item
    d->aZoom->setEnabled( true );
    d->aZoom->selectableActionGroup()->setEnabled( true );
    d->aZoom->setCurrentItem( selIdx );
    d->aZoom->setEnabled( d->items.size() > 0 );
    d->aZoom->selectableActionGroup()->setEnabled( d->items.size() > 0 );
}

void PageView::updateCursor()
{
    const QPoint p = contentAreaPosition() + viewport()->mapFromGlobal( QCursor::pos() );
    updateCursor( p );
}

void PageView::updateCursor( const QPoint &p )
{
    // reset mouse over link it will be re-set if that still valid
    d->mouseOverLinkObject = nullptr;

    // detect the underlaying page (if present)
    PageViewItem * pageItem = pickItemOnPoint( p.x(), p.y() );

    if ( d->annotator && d->annotator->active() )
    {
        if ( pageItem || d->annotator->annotating() )
            setCursor( d->annotator->cursor() );
        else
            setCursor( Qt::ForbiddenCursor );
    }
    else if ( pageItem )
    {
        double nX = pageItem->absToPageX(p.x());
        double nY = pageItem->absToPageY(p.y());
        Qt::CursorShape cursorShapeFallback;

        // if over a ObjectRect (of type Link) change cursor to hand
        switch ( d->mouseMode )
        {
        case Okular::Settings::EnumMouseMode::TextSelect:
            if (d->mouseTextSelecting)
            {
                setCursor( Qt::IBeamCursor );
                return;
            }
            cursorShapeFallback = Qt::IBeamCursor;
            break;
        case Okular::Settings::EnumMouseMode::Magnifier:
            setCursor( Qt::CrossCursor );
            return;
        case Okular::Settings::EnumMouseMode::RectSelect:
        case Okular::Settings::EnumMouseMode::TrimSelect:
            if (d->mouseSelecting)
            {
                setCursor( Qt::CrossCursor );
                return;
            }
            cursorShapeFallback = Qt::CrossCursor;
            break;
        case Okular::Settings::EnumMouseMode::Browse:
            d->mouseOnRect = false;
            if ( d->mouseAnnotation->isMouseOver() )
            {
                d->mouseOnRect = true;
                setCursor( d->mouseAnnotation->cursor() );
                return;
            }
            else
            {
                cursorShapeFallback = Qt::OpenHandCursor;
            }
            break;
        default:
            setCursor( Qt::ArrowCursor );
            return;
        }

        const Okular::ObjectRect * linkobj = pageItem->page()->objectRect( Okular::ObjectRect::Action, nX, nY, pageItem->uncroppedWidth(), pageItem->uncroppedHeight() );
        if ( linkobj )
        {
            d->mouseOverLinkObject = linkobj;
            d->mouseOnRect = true;
            setCursor( Qt::PointingHandCursor );
        }
        else
        {
            setCursor(cursorShapeFallback);
        }
    }
    else
    {
        // if there's no page over the cursor and we were showing the pointingHandCursor
        // go back to the normal one
        d->mouseOnRect = false;
        setCursor( Qt::ArrowCursor );
    }
}

void PageView::reloadForms()
{
    QLinkedList< PageViewItem * >::const_iterator iIt = d->visibleItems.constBegin(), iEnd = d->visibleItems.constEnd();
    if( d->m_formsVisible )
    {
        for ( ; iIt != iEnd; ++iIt )
        {
            (*iIt)->reloadFormWidgetsState();
        }
    }

}

void PageView::moveMagnifier( const QPoint& p ) // non scaled point
{
    const int w = d->magnifierView->width() * 0.5;
    const int h = d->magnifierView->height() * 0.5;

    int x = p.x() - w;
    int y = p.y() - h;

    const int max_x = viewport()->width();
    const int max_y = viewport()->height();

    QPoint scroll(0,0);

    if (x < 0)
    {
        if (horizontalScrollBar()->value() > 0) scroll.setX(x - w);
        x = 0;
    }

    if (y < 0)
    {
        if (verticalScrollBar()->value() > 0) scroll.setY(y - h);
        y = 0;
    }

    if (p.x() + w > max_x)
    {
        if (horizontalScrollBar()->value() < horizontalScrollBar()->maximum()) scroll.setX(p.x() + 2 * w - max_x);
        x = max_x - d->magnifierView->width() - 1;
    }

    if (p.y() + h > max_y)
    {
        if (verticalScrollBar()->value() < verticalScrollBar()->maximum()) scroll.setY(p.y() + 2 * h - max_y);
        y = max_y - d->magnifierView->height() - 1;
    }

    if (!scroll.isNull())
        scrollPosIntoView(contentAreaPoint(p + scroll));

    d->magnifierView->move(x, y);
}

void PageView::updateMagnifier( const QPoint& p ) // scaled point
{
    /* translate mouse coordinates to page coordinates and inform the magnifier of the situation */
    PageViewItem *item = pickItemOnPoint(p.x(), p.y());
    if (item)
    {
        Okular::NormalizedPoint np(item->absToPageX(p.x()), item->absToPageY(p.y()));
        d->magnifierView->updateView( np, item->page() );
    }
}

int PageView::viewColumns() const
{
    int vm = Okular::Settings::viewMode();
    if (vm == Okular::Settings::EnumViewMode::Single) return 1;
    else if (vm == Okular::Settings::EnumViewMode::Facing ||
             vm == Okular::Settings::EnumViewMode::FacingFirstCentered) return 2;
    else if (vm == Okular::Settings::EnumViewMode::Summary
            && d->document->pages() < Okular::Settings::viewColumns() )
            return d->document->pages();
    else return Okular::Settings::viewColumns();
}

void PageView::center(int cx, int cy)
{
    scrollTo( cx - viewport()->width() / 2, cy - viewport()->height() / 2 );
}

void PageView::scrollTo( int x, int y )
{
    bool prevState = d->blockPixmapsRequest;

    int newValue = -1;
    if ( x != horizontalScrollBar()->value() || y != verticalScrollBar()->value() )
        newValue = 1; // Pretend this call is the result of a scrollbar event

    d->blockPixmapsRequest = true;
    horizontalScrollBar()->setValue( x );
    verticalScrollBar()->setValue( y );

    d->blockPixmapsRequest = prevState;

    slotRequestVisiblePixmaps( newValue );
}

void PageView::toggleFormWidgets( bool on )
{
    bool somehadfocus = false;
    QVector< PageViewItem * >::const_iterator dIt = d->items.constBegin(), dEnd = d->items.constEnd();
    for ( ; dIt != dEnd; ++dIt )
    {
        bool hadfocus = (*dIt)->setFormWidgetsVisible( on );
        somehadfocus = somehadfocus || hadfocus;
    }
    if ( somehadfocus )
        setFocus();
    d->m_formsVisible = on;
    if ( d->aToggleForms ) // it may not exist if we are on dummy mode
    {
        if ( d->m_formsVisible )
        {
            d->aToggleForms->setText( i18n( "Hide Forms" ) );
        }
        else
        {
            d->aToggleForms->setText( i18n( "Show Forms" ) );
        }
    }
}

void PageView::resizeContentArea( const QSize & newSize )
{
    const QSize vs = viewport()->size();
    int hRange = newSize.width() - vs.width();
    int vRange = newSize.height() - vs.height();
    if ( horizontalScrollBar()->isVisible() && hRange == verticalScrollBar()->width() && verticalScrollBar()->isVisible() && vRange == horizontalScrollBar()->height() && Okular::Settings::showScrollBars() )
    {
        hRange = 0;
        vRange = 0;
    }
    horizontalScrollBar()->setRange( 0, hRange );
    verticalScrollBar()->setRange( 0, vRange );
    updatePageStep();
}

void PageView::updatePageStep() {
    const QSize vs = viewport()->size();
    horizontalScrollBar()->setPageStep( vs.width() );
    verticalScrollBar()->setPageStep( vs.height() * (100 - Okular::Settings::scrollOverlap()) / 100 );
}

void PageView::addWebShortcutsMenu( QMenu * menu, const QString & text )
{
    if ( text.isEmpty() )
    {
        return;
    }

    QString searchText = text;
    searchText = searchText.replace( QLatin1Char('\n'), QLatin1Char(' ') ).replace(QLatin1Char( '\r'), QLatin1Char(' ') ).simplified();

    if ( searchText.isEmpty() )
    {
        return;
    }

    KUriFilterData filterData( searchText );

    filterData.setSearchFilteringOptions( KUriFilterData::RetrievePreferredSearchProvidersOnly );

    if ( KUriFilter::self()->filterSearchUri( filterData, KUriFilter::NormalTextFilter ) )
    {
        const QStringList searchProviders = filterData.preferredSearchProviders();

        if ( !searchProviders.isEmpty() )
        {
            QMenu *webShortcutsMenu = new QMenu( menu );
            webShortcutsMenu->setIcon( QIcon::fromTheme( QStringLiteral("preferences-web-browser-shortcuts") ) );

            const QString squeezedText = KStringHandler::rsqueeze( searchText, 21 );
            webShortcutsMenu->setTitle( i18n( "Search for '%1' with", squeezedText ) );

            QAction *action = nullptr;

            foreach( const QString &searchProvider, searchProviders )
            {
                action = new QAction( searchProvider, webShortcutsMenu );
                action->setIcon( QIcon::fromTheme( filterData.iconNameForPreferredSearchProvider( searchProvider ) ) );
                action->setData( filterData.queryForPreferredSearchProvider( searchProvider ) );
                connect( action, &QAction::triggered, this, &PageView::slotHandleWebShortcutAction );
                webShortcutsMenu->addAction( action );
            }

            webShortcutsMenu->addSeparator();

            action = new QAction( i18n( "Configure Web Shortcuts..." ), webShortcutsMenu );
            action->setIcon( QIcon::fromTheme( QStringLiteral("configure") ) );
            connect( action, &QAction::triggered, this, &PageView::slotConfigureWebShortcuts );
            webShortcutsMenu->addAction( action );

            menu->addMenu(webShortcutsMenu);
        }
    }
}

QMenu* PageView::createProcessLinkMenu(PageViewItem *item, const QPoint &eventPos)
{
    // check if the right-click was over a link
    const double nX = item->absToPageX(eventPos.x());
    const double nY = item->absToPageY(eventPos.y());
    const Okular::ObjectRect * rect = item->page()->objectRect( Okular::ObjectRect::Action, nX, nY, item->uncroppedWidth(), item->uncroppedHeight() );
    if ( rect )
    {
        QMenu *menu = new QMenu(this);
        const Okular::Action * link = static_cast< const Okular::Action * >( rect->object() );
        // creating the menu and its actions
        QAction * processLink = menu->addAction( i18n( "Follow This Link" ) );
        processLink->setObjectName("ProcessLinkAction");
        if ( link->actionType() == Okular::Action::Sound ) {
            processLink->setText( i18n( "Play this Sound" ) );
            if ( Okular::AudioPlayer::instance()->state() == Okular::AudioPlayer::PlayingState ) {
                QAction * actStopSound = menu->addAction( i18n( "Stop Sound" ) );
                connect( actStopSound, &QAction::triggered, []() {
                    Okular::AudioPlayer::instance()->stopPlaybacks();
                });
            }
        }


        if ( dynamic_cast< const Okular::BrowseAction * >( link ) )
        {
            QAction * actCopyLinkLocation = menu->addAction( QIcon::fromTheme( QStringLiteral("edit-copy") ), i18n( "Copy Link Address" ) );
            actCopyLinkLocation->setObjectName("CopyLinkLocationAction");
            connect( actCopyLinkLocation, &QAction::triggered, [ link ]() {
                const Okular::BrowseAction * browseLink = static_cast< const Okular::BrowseAction * >( link );
                QClipboard *cb = QApplication::clipboard();
                cb->setText( browseLink->url().toDisplayString(), QClipboard::Clipboard );
                if ( cb->supportsSelection() )
                    cb->setText( browseLink->url().toDisplayString(), QClipboard::Selection );
            } );
        }

        connect( processLink, &QAction::triggered, [this, link]() {
            d->document->processAction( link );
        });
        return menu;
    }
    return nullptr;
}

//BEGIN private SLOTS
void PageView::slotRelayoutPages()
// called by: notifySetup, viewportResizeEvent, slotViewMode, slotContinuousToggled, updateZoom
{
    // set an empty container if we have no pages
    const int pageCount = d->items.count();
    if ( pageCount < 1 )
    {
        return;
    }

    // if viewport was auto-moving, stop it
    if ( d->viewportMoveActive )
    {
        center( d->viewportMoveDest.x(), d->viewportMoveDest.y() );
        d->viewportMoveActive = false;
        d->viewportMoveTimer->stop();
        verticalScrollBar()->setEnabled( true );
        horizontalScrollBar()->setEnabled( true );
    }

    // common iterator used in this method and viewport parameters
    QVector< PageViewItem * >::const_iterator iIt, iEnd = d->items.constEnd();
    int viewportWidth = viewport()->width(),
        viewportHeight = viewport()->height(),
        fullWidth = 0,
        fullHeight = 0;
    QRect viewportRect( horizontalScrollBar()->value(), verticalScrollBar()->value(), viewportWidth, viewportHeight );

    // handle the 'center first page in row' stuff
    const bool facing = Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::Facing && pageCount > 1;
    const bool facingCentered = Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::FacingFirstCentered ||
                               (Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::Facing && pageCount == 1);
    const bool overrideCentering = facingCentered && pageCount < 3;
    const bool centerFirstPage = facingCentered && !overrideCentering;
    const bool facingPages = facing || centerFirstPage;
    const bool centerLastPage = centerFirstPage && pageCount % 2 == 0;
    const bool continuousView = Okular::Settings::viewContinuous();
    const int nCols = overrideCentering ? 1 : viewColumns();
    const bool singlePageViewMode = Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::Single;

    if ( d->aFitWindowToPage )
        d->aFitWindowToPage->setEnabled( !continuousView && singlePageViewMode );

    // set all items geometry and resize contents. handle 'continuous' and 'single' modes separately

    PageViewItem * currentItem = d->items[ qMax( 0, (int)d->document->currentPage() ) ];

        // Here we find out column's width and row's height to compute a table
        // so we can place widgets 'centered in virtual cells'.
        const int nRows = (int)ceil( (float)(centerFirstPage ? (pageCount + nCols - 1) : pageCount) / (float)nCols );

        int * colWidth = new int[ nCols ],
            * rowHeight = new int[ nRows ],
            cIdx = 0,
            rIdx = 0;
        for ( int i = 0; i < nCols; i++ )
            colWidth[ i ] = viewportWidth / nCols;
        for ( int i = 0; i < nRows; i++ )
            rowHeight[ i ] = 0;
        // handle the 'centering on first row' stuff
        if ( centerFirstPage )
            cIdx += nCols - 1;

        // 1) find the maximum columns width and rows height for a grid in
        // which each page must well-fit inside a cell
        for ( iIt = d->items.constBegin(); iIt != iEnd; ++iIt )
        {
            PageViewItem * item = *iIt;
            // update internal page size (leaving a little margin in case of Fit* modes)
            updateItemSize( item, colWidth[ cIdx ] - kcolWidthMargin, viewportHeight - krowHeightMargin );
            // find row's maximum height and column's max width
            if ( item->croppedWidth() + kcolWidthMargin > colWidth[ cIdx ] )
                colWidth[ cIdx ] = item->croppedWidth() + kcolWidthMargin;
            if ( item->croppedHeight() + krowHeightMargin > rowHeight[ rIdx ] )
                rowHeight[ rIdx ] = item->croppedHeight() + krowHeightMargin;
            // handle the 'centering on first row' stuff
            // update col/row indices
            if ( ++cIdx == nCols )
            {
                cIdx = 0;
                rIdx++;
            }
        }

        const int pageRowIdx = ( ( centerFirstPage ? nCols - 1 : 0 ) + currentItem->pageNumber() ) / nCols;

        // 2) compute full size
        for ( int i = 0; i < nCols; i++ )
            fullWidth += colWidth[ i ];
        if ( continuousView )
        {
            for ( int i = 0; i < nRows; i++ )
                fullHeight += rowHeight[ i ];
        }
        else
            fullHeight = rowHeight[ pageRowIdx ];

        // 3) arrange widgets inside cells (and refine fullHeight if needed)
        int insertX = 0,
            insertY = fullHeight < viewportHeight ? ( viewportHeight - fullHeight ) / 2 : 0;
        const int origInsertY = insertY;
        cIdx = 0;
        rIdx = 0;
        if ( centerFirstPage )
        {
            cIdx += nCols - 1;
            for ( int i = 0; i < cIdx; ++i )
                insertX += colWidth[ i ];
        }
        for ( iIt = d->items.constBegin(); iIt != iEnd; ++iIt )
        {
            PageViewItem * item = *iIt;
            int cWidth = colWidth[ cIdx ],
                rHeight = rowHeight[ rIdx ];
            if ( continuousView || rIdx == pageRowIdx )
            {
                const bool reallyDoCenterFirst = item->pageNumber() == 0 && centerFirstPage;
                const bool reallyDoCenterLast = item->pageNumber() == pageCount - 1 && centerLastPage;
                int actualX = 0;
                if ( reallyDoCenterFirst || reallyDoCenterLast )
                {
                    // page is centered across entire viewport
                    actualX = (fullWidth - item->croppedWidth()) / 2;
                }
                else if ( facingPages )
                {
                    if (Okular::Settings::rtlReadingDirection()){
                        // RTL reading mode
                        actualX = ( (centerFirstPage && item->pageNumber() % 2 == 0) ||
                                    (!centerFirstPage && item->pageNumber() % 2 == 1) ) ?
                                    (fullWidth / 2) - item->croppedWidth() - 1 : (fullWidth / 2) + 1;
                    } else {
                        // page edges 'touch' the center of the viewport
                        actualX = ( (centerFirstPage && item->pageNumber() % 2 == 1) ||
                                    (!centerFirstPage && item->pageNumber() % 2 == 0) ) ?
                                    (fullWidth / 2) - item->croppedWidth() - 1 : (fullWidth / 2) + 1;
                    }
                }
                else
                {
                    // page is centered within its virtual column
                    //actualX = insertX + (cWidth - item->croppedWidth()) / 2;
                    if (Okular::Settings::rtlReadingDirection()){
                        actualX = fullWidth - insertX - cWidth +( (cWidth - item->croppedWidth()) / 2);
                    } else {
                        actualX = insertX + (cWidth - item->croppedWidth()) / 2;
                    }
                }
                item->moveTo( actualX,
                              (continuousView ? insertY : origInsertY) + (rHeight - item->croppedHeight()) / 2 );
                item->setVisible( true );
            }
            else
            {
                item->moveTo( 0, 0 );
                item->setVisible( false );
            }
            item->setFormWidgetsVisible( d->m_formsVisible );
            // advance col/row index
            insertX += cWidth;
            if ( ++cIdx == nCols )
            {
                cIdx = 0;
                rIdx++;
                insertX = 0;
                insertY += rHeight;
            }
#ifdef PAGEVIEW_DEBUG
            kWarning() << "updating size for pageno" << item->pageNumber() << "cropped" << item->croppedGeometry() << "uncropped" << item->uncroppedGeometry();
#endif
        }

        delete [] colWidth;
        delete [] rowHeight;

    // 3) reset dirty state
    d->dirtyLayout = false;

    // 4) update scrollview's contents size and recenter view
    bool wasUpdatesEnabled = viewport()->updatesEnabled();
    if ( fullWidth != contentAreaWidth() || fullHeight != contentAreaHeight() )
    {
        const Okular::DocumentViewport vp = d->document->viewport();
        // disable updates and resize the viewportContents
        if ( wasUpdatesEnabled )
            viewport()->setUpdatesEnabled( false );
        resizeContentArea( QSize( fullWidth, fullHeight ) );
        // restore previous viewport if defined and updates enabled
        if ( wasUpdatesEnabled )
        {
            if ( vp.pageNumber >= 0 )
            {
                int prevX = horizontalScrollBar()->value(),
                    prevY = verticalScrollBar()->value();
                const QRect & geometry = d->items[ vp.pageNumber ]->croppedGeometry();
                double nX = vp.rePos.enabled ? normClamp( vp.rePos.normalizedX, 0.5 ) : 0.5,
                       nY = vp.rePos.enabled ? normClamp( vp.rePos.normalizedY, 0.0 ) : 0.0;
                center( geometry.left() + qRound( nX * (double)geometry.width() ),
                        geometry.top() + qRound( nY * (double)geometry.height() ) );
                // center() usually moves the viewport, that requests pixmaps too.
                // if that doesn't happen we have to request them by hand
                if ( prevX == horizontalScrollBar()->value() && prevY == verticalScrollBar()->value() )
                    slotRequestVisiblePixmaps();
            }
            // or else go to center page
            else
                center( fullWidth / 2, 0 );
            viewport()->setUpdatesEnabled( true );
        }
    }

    // 5) update the whole viewport if updated enabled
    if ( wasUpdatesEnabled )
        viewport()->update();
}

void PageView::delayedResizeEvent()
{
    // If we already got here we don't need to execute the timer slot again
    d->delayResizeEventTimer->stop();
    slotRelayoutPages();
    slotRequestVisiblePixmaps();
}

static void slotRequestPreloadPixmap( Okular::DocumentObserver * observer, const PageViewItem * i, const QRect &expandedViewportRect, QLinkedList< Okular::PixmapRequest * > *requestedPixmaps )
{
    Okular::NormalizedRect preRenderRegion;
    const QRect intersectionRect = expandedViewportRect.intersected( i->croppedGeometry() );
    if ( !intersectionRect.isEmpty() )
        preRenderRegion = Okular::NormalizedRect( intersectionRect.translated( -i->uncroppedGeometry().topLeft() ), i->uncroppedWidth(), i->uncroppedHeight() );

    // request the pixmap if not already present
    if ( !i->page()->hasPixmap( observer, i->uncroppedWidth(), i->uncroppedHeight(), preRenderRegion ) && i->uncroppedWidth() > 0 )
    {
        Okular::PixmapRequest::PixmapRequestFeatures requestFeatures = Okular::PixmapRequest::Preload;
        requestFeatures |= Okular::PixmapRequest::Asynchronous;
        const bool pageHasTilesManager = i->page()->hasTilesManager( observer );
        if ( pageHasTilesManager && !preRenderRegion.isNull() )
        {
            Okular::PixmapRequest * p = new Okular::PixmapRequest( observer, i->pageNumber(), i->uncroppedWidth(), i->uncroppedHeight(), PAGEVIEW_PRELOAD_PRIO, requestFeatures );
            requestedPixmaps->push_back( p );

            p->setNormalizedRect( preRenderRegion );
            p->setTile( true );
        }
        else if ( !pageHasTilesManager )
        {
            Okular::PixmapRequest * p = new Okular::PixmapRequest( observer, i->pageNumber(), i->uncroppedWidth(), i->uncroppedHeight(), PAGEVIEW_PRELOAD_PRIO, requestFeatures );
            requestedPixmaps->push_back( p );
            p->setNormalizedRect( preRenderRegion );
        }
    }
}

void PageView::slotRequestVisiblePixmaps( int newValue )
{
    // if requests are blocked (because raised by an unwanted event), exit
    if ( d->blockPixmapsRequest || d->viewportMoveActive )
        return;

    // precalc view limits for intersecting with page coords inside the loop
    const bool isEvent = newValue != -1 && !d->blockViewport;
    const QRect viewportRect( horizontalScrollBar()->value(),
                              verticalScrollBar()->value(),
                              viewport()->width(), viewport()->height() );
    const QRect viewportRectAtZeroZero( 0, 0, viewport()->width(), viewport()->height() );

    // some variables used to determine the viewport
    int nearPageNumber = -1;
    const double viewportCenterX = (viewportRect.left() + viewportRect.right()) / 2.0;
    const double viewportCenterY = (viewportRect.top() + viewportRect.bottom()) / 2.0;
    double focusedX = 0.5,
           focusedY = 0.0,
           minDistance = -1.0;
    // Margin (in pixels) around the viewport to preload
    const int pixelsToExpand = 512;

    // iterate over all items
    d->visibleItems.clear();
    QLinkedList< Okular::PixmapRequest * > requestedPixmaps;
    QVector< Okular::VisiblePageRect * > visibleRects;
    QVector< PageViewItem * >::const_iterator iIt = d->items.constBegin(), iEnd = d->items.constEnd();
    for ( ; iIt != iEnd; ++iIt )
    {
        PageViewItem * i = *iIt;
        foreach( FormWidgetIface *fwi, i->formWidgets() )
        {
            Okular::NormalizedRect r = fwi->rect();
            fwi->moveTo(
                qRound( i->uncroppedGeometry().left() + i->uncroppedWidth() * r.left ) + 1 - viewportRect.left(),
                qRound( i->uncroppedGeometry().top() + i->uncroppedHeight() * r.top ) + 1 - viewportRect.top() );
        }
        Q_FOREACH ( VideoWidget *vw, i->videoWidgets() )
        {
            const Okular::NormalizedRect r = vw->normGeometry();
            vw->move(
                qRound( i->uncroppedGeometry().left() + i->uncroppedWidth() * r.left ) + 1 - viewportRect.left(),
                qRound( i->uncroppedGeometry().top() + i->uncroppedHeight() * r.top ) + 1 - viewportRect.top() );

            if ( vw->isPlaying() && viewportRectAtZeroZero.intersected( vw->geometry() ).isEmpty() ) {
                vw->stop();
                vw->pageLeft();
            }
        }

        if ( !i->isVisible() )
            continue;
#ifdef PAGEVIEW_DEBUG
        kWarning() << "checking page" << i->pageNumber();
        kWarning().nospace() << "viewportRect is " << viewportRect << ", page item is " << i->croppedGeometry() << " intersect : " << viewportRect.intersects( i->croppedGeometry() );
#endif
        // if the item doesn't intersect the viewport, skip it
        QRect intersectionRect = viewportRect.intersected( i->croppedGeometry() );
        if ( intersectionRect.isEmpty() )
        {
            continue;
        }

        // add the item to the 'visible list'
        d->visibleItems.push_back( i );
        Okular::VisiblePageRect * vItem = new Okular::VisiblePageRect( i->pageNumber(), Okular::NormalizedRect( intersectionRect.translated( -i->uncroppedGeometry().topLeft() ), i->uncroppedWidth(), i->uncroppedHeight() ) );
        visibleRects.push_back( vItem );
#ifdef PAGEVIEW_DEBUG
        kWarning() << "checking for pixmap for page" << i->pageNumber() << "=" << i->page()->hasPixmap( this, i->uncroppedWidth(), i->uncroppedHeight() );
        kWarning() << "checking for text for page" << i->pageNumber() << "=" << i->page()->hasTextPage();
#endif

        Okular::NormalizedRect expandedVisibleRect = vItem->rect;
        if ( i->page()->hasTilesManager( this ) && Okular::Settings::memoryLevel() != Okular::Settings::EnumMemoryLevel::Low )
        {
            double rectMargin = pixelsToExpand/(double)i->uncroppedHeight();
            expandedVisibleRect.left = qMax( 0.0, vItem->rect.left - rectMargin );
            expandedVisibleRect.top = qMax( 0.0, vItem->rect.top - rectMargin );
            expandedVisibleRect.right = qMin( 1.0, vItem->rect.right + rectMargin );
            expandedVisibleRect.bottom = qMin( 1.0, vItem->rect.bottom + rectMargin );
        }

        // if the item has not the right pixmap, add a request for it
        if ( !i->page()->hasPixmap( this, i->uncroppedWidth(), i->uncroppedHeight(), expandedVisibleRect ) )
        {
#ifdef PAGEVIEW_DEBUG
            kWarning() << "rerequesting visible pixmaps for page" << i->pageNumber() << "!";
#endif
            Okular::PixmapRequest * p = new Okular::PixmapRequest( this, i->pageNumber(), i->uncroppedWidth(), i->uncroppedHeight(), PAGEVIEW_PRIO, Okular::PixmapRequest::Asynchronous );
            requestedPixmaps.push_back( p );

            if ( i->page()->hasTilesManager( this ) )
            {
                p->setNormalizedRect( expandedVisibleRect );
                p->setTile( true );
            }
            else
                p->setNormalizedRect( vItem->rect );
        }

        // look for the item closest to viewport center and the relative
        // position between the item and the viewport center
        if ( isEvent )
        {
            const QRect & geometry = i->croppedGeometry();
            // compute distance between item center and viewport center (slightly moved left)
            double distance = hypot( (geometry.left() + geometry.right()) / 2 - (viewportCenterX - 4),
                                     (geometry.top() + geometry.bottom()) / 2 - viewportCenterY );
            if ( distance >= minDistance && nearPageNumber != -1 )
                continue;
            nearPageNumber = i->pageNumber();
            minDistance = distance;
            if ( geometry.height() > 0 && geometry.width() > 0 )
            {
                focusedX = ( viewportCenterX - (double)geometry.left() ) / (double)geometry.width();
                focusedY = ( viewportCenterY - (double)geometry.top() ) / (double)geometry.height();
            }
        }
    }

    // if preloading is enabled, add the pages before and after in preloading
    if ( !d->visibleItems.isEmpty() &&
         Okular::SettingsCore::memoryLevel() != Okular::SettingsCore::EnumMemoryLevel::Low )
    {
        // as the requests are done in the order as they appear in the list,
        // request first the next page and then the previous

        int pagesToPreload = viewColumns();

        // if the greedy option is set, preload all pages
        if (Okular::SettingsCore::memoryLevel() == Okular::SettingsCore::EnumMemoryLevel::Greedy)
            pagesToPreload = d->items.count();

        const QRect expandedViewportRect = viewportRect.adjusted( 0, -pixelsToExpand, 0, pixelsToExpand );

        for( int j = 1; j <= pagesToPreload; j++ )
        {
            // add the page after the 'visible series' in preload
            const int tailRequest = d->visibleItems.last()->pageNumber() + j;
            if ( tailRequest < (int)d->items.count() )
            {
                slotRequestPreloadPixmap( this, d->items[ tailRequest ], expandedViewportRect, &requestedPixmaps );
            }

            // add the page before the 'visible series' in preload
            const int headRequest = d->visibleItems.first()->pageNumber() - j;
            if ( headRequest >= 0 )
            {
                slotRequestPreloadPixmap( this, d->items[ headRequest ], expandedViewportRect, &requestedPixmaps );
            }

            // stop if we've already reached both ends of the document
            if ( headRequest < 0 && tailRequest >= (int)d->items.count() )
                break;
        }
    }

    // send requests to the document
    if ( !requestedPixmaps.isEmpty() )
    {
        d->document->requestPixmaps( requestedPixmaps );
    }
    // if this functions was invoked by viewport events, send update to document
    if ( isEvent && nearPageNumber != -1 )
    {
        // determine the document viewport
        Okular::DocumentViewport newViewport( nearPageNumber );
        newViewport.rePos.enabled = true;
        newViewport.rePos.normalizedX = focusedX;
        newViewport.rePos.normalizedY = focusedY;
        // set the viewport to other observers
        d->document->setViewport( newViewport , this );
    }
    d->document->setVisiblePageRects( visibleRects, this );
}

void PageView::slotMoveViewport()
{
    // converge to viewportMoveDest in 1 second
    int diffTime = d->viewportMoveTime.elapsed();
    if ( diffTime >= 667 || !d->viewportMoveActive )
    {
        center( d->viewportMoveDest.x(), d->viewportMoveDest.y() );
        d->viewportMoveTimer->stop();
        d->viewportMoveActive = false;
        slotRequestVisiblePixmaps();
        verticalScrollBar()->setEnabled( true );
        horizontalScrollBar()->setEnabled( true );
        return;
    }

    // move the viewport smoothly (kmplot: p(x)=1+0.47*(x-1)^3-0.25*(x-1)^4)
    float convergeSpeed = (float)diffTime / 667.0,
          x = ((float)viewport()->width() / 2.0) + horizontalScrollBar()->value(),
          y = ((float)viewport()->height() / 2.0) + verticalScrollBar()->value(),
          diffX = (float)d->viewportMoveDest.x() - x,
          diffY = (float)d->viewportMoveDest.y() - y;
    convergeSpeed *= convergeSpeed * (1.4 - convergeSpeed);
    center( (int)(x + diffX * convergeSpeed),
            (int)(y + diffY * convergeSpeed ) );
}

void PageView::slotAutoScroll()
{
    // the first time create the timer
    if ( !d->autoScrollTimer )
    {
        d->autoScrollTimer = new QTimer( this );
        d->autoScrollTimer->setSingleShot( true );
        connect( d->autoScrollTimer, &QTimer::timeout, this, &PageView::slotAutoScroll );
    }

    // if scrollIncrement is zero, stop the timer
    if ( !d->scrollIncrement )
    {
        d->autoScrollTimer->stop();
        return;
    }

    // compute delay between timer ticks and scroll amount per tick
    int index = abs( d->scrollIncrement ) - 1;  // 0..9
    const int scrollDelay[10] =  { 200, 100, 50, 30, 20, 30, 25, 20, 30, 20 };
    const int scrollOffset[10] = {   1,   1,  1,  1,  1,  2,  2,  2,  4,  4 };
    d->autoScrollTimer->start( scrollDelay[ index ] );
    int delta = d->scrollIncrement > 0 ? scrollOffset[ index ] : -scrollOffset[ index ];
    verticalScrollBar()->setValue(verticalScrollBar()->value() + delta);
}

void PageView::slotDragScroll()
{
    scrollTo( horizontalScrollBar()->value() + d->dragScrollVector.x(), verticalScrollBar()->value() + d->dragScrollVector.y() );
    QPoint p = contentAreaPosition() + viewport()->mapFromGlobal( QCursor::pos() );
    updateSelection( p );
}

void PageView::slotShowWelcome()
{
    // show initial welcome text
    d->messageWindow->display( i18n( "Welcome" ), QString(), PageViewMessage::Info, 2000 );
}

void PageView::slotShowSizeAllCursor()
{
    setCursor( Qt::SizeAllCursor );
}

void PageView::slotHandleWebShortcutAction()
{
    QAction *action = qobject_cast<QAction*>( sender() );

    if (action) {
        KUriFilterData filterData( action->data().toString() );

        if ( KUriFilter::self()->filterSearchUri( filterData, KUriFilter::WebShortcutFilter ) ) {
            QDesktopServices::openUrl( filterData.uri() );
        }
    }
}

void PageView::slotConfigureWebShortcuts()
{
    KToolInvocation::kdeinitExec( QStringLiteral("kcmshell5"), QStringList() << QStringLiteral("webshortcuts") );
}

void PageView::slotZoom()
{
    if ( !d->aZoom->selectableActionGroup()->isEnabled() )
        return;

    setFocus();
    updateZoom( ZoomFixed );
}

void PageView::slotZoomIn()
{
    updateZoom( ZoomIn );
}

void PageView::slotZoomOut()
{
    updateZoom( ZoomOut );
}

void PageView::slotFitToWidthToggled( bool on )
{
    if ( on ) updateZoom( ZoomFitWidth );
}

void PageView::slotFitToPageToggled( bool on )
{
    if ( on ) updateZoom( ZoomFitPage );
}

void PageView::slotAutoFitToggled( bool on )
{
    if ( on ) updateZoom( ZoomFitAuto );
}

void PageView::slotViewMode( QAction *action )
{
    const int nr = action->data().toInt();
    if ( (int)Okular::Settings::viewMode() != nr )
    {
        Okular::Settings::setViewMode( nr );
        Okular::Settings::self()->save();
        if ( d->document->pages() > 0 )
            slotRelayoutPages();
    }
}

void PageView::slotContinuousToggled( bool on )
{
    if ( Okular::Settings::viewContinuous() != on )
    {
        Okular::Settings::setViewContinuous( on );
        Okular::Settings::self()->save();
        if ( d->document->pages() > 0 )
            slotRelayoutPages();
    }
}

void PageView::slotSetMouseNormal()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::Browse;
    Okular::Settings::setMouseMode( d->mouseMode );
    // hide the messageWindow
    d->messageWindow->hide();
    // reshow the annotator toolbar if hiding was forced (and if it is not already visible)
    if ( d->annotator && d->annotator->hidingWasForced() && d->aToggleAnnotator && !d->aToggleAnnotator->isChecked() )
        d->aToggleAnnotator->trigger();
    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
}

void PageView::slotSetMouseZoom()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::Zoom;
    Okular::Settings::setMouseMode( d->mouseMode );
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display( i18n( "Select zooming area. Right-click to zoom out." ), QString(), PageViewMessage::Info, -1 );
    // force hiding of annotator toolbar
    if ( d->aToggleAnnotator && d->aToggleAnnotator->isChecked() )
    {
        d->aToggleAnnotator->trigger();
        d->annotator->setHidingForced( true );
    }
    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
}

void PageView::slotSetMouseMagnifier()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::Magnifier;
    Okular::Settings::setMouseMode( d->mouseMode );
    d->messageWindow->display( i18n( "Click to see the magnified view." ), QString() );

    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
}

void PageView::slotSetMouseSelect()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::RectSelect;
    Okular::Settings::setMouseMode( d->mouseMode );
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display( i18n( "Draw a rectangle around the text/graphics to copy." ), QString(), PageViewMessage::Info, -1 );
    // force hiding of annotator toolbar
    if ( d->aToggleAnnotator && d->aToggleAnnotator->isChecked() )
    {
        d->aToggleAnnotator->trigger();
        d->annotator->setHidingForced( true );
    }
    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
}

void PageView::slotSetMouseTextSelect()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::TextSelect;
    Okular::Settings::setMouseMode( d->mouseMode );
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display( i18n( "Select text" ), QString(), PageViewMessage::Info, -1 );
    // force hiding of annotator toolbar
    if ( d->aToggleAnnotator && d->aToggleAnnotator->isChecked() )
    {
        d->aToggleAnnotator->trigger();
        d->annotator->setHidingForced( true );
    }
    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
}

void PageView::slotSetMouseTableSelect()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::TableSelect;
    Okular::Settings::setMouseMode( d->mouseMode );
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display( i18n(
        "Draw a rectangle around the table, then click near edges to divide up; press Esc to clear."
        ), QString(), PageViewMessage::Info, -1 );
    // force hiding of annotator toolbar
    if ( d->aToggleAnnotator && d->aToggleAnnotator->isChecked() )
    {
        d->aToggleAnnotator->trigger();
        d->annotator->setHidingForced( true );
    }
    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
}

void PageView::slotToggleAnnotator( bool on )
{
    // the 'inHere' trick is needed as the slotSetMouseZoom() calls this
    static bool inHere = false;
    if ( inHere )
        return;
    inHere = true;

    // the annotator can be used in normal mouse mode only, so if asked for it,
    // switch to normal mode
    if ( on && d->mouseMode != Okular::Settings::EnumMouseMode::Browse )
        d->aMouseNormal->trigger();

    // ask for Author's name if not already set
    if ( Okular::Settings::identityAuthor().isEmpty() )
    {
        // get default username from the kdelibs/kdecore/KUser
        KUser currentUser;
        QString userName = currentUser.property( KUser::FullName ).toString();
        // ask the user for confirmation/change
        if ( userName.isEmpty() )
        {
            bool ok = false;
            userName = QInputDialog::getText(nullptr,
                           i18n( "Annotations author" ),
                           i18n( "Please insert your name or initials:" ),
                           QLineEdit::Normal,
                           QString(),
                           &ok );

            if ( !ok )
            {
                d->aToggleAnnotator->trigger();
                inHere = false;
                return;
            }
        }
        // save the name
        Okular::Settings::setIdentityAuthor( userName );
        Okular::Settings::self()->save();
    }

    // create the annotator object if not present
    if ( !d->annotator )
    {
        d->annotator = new PageViewAnnotator( this, d->document );
        bool allowTools = d->document->pages() > 0 && d->document->isAllowed( Okular::AllowNotes );
        d->annotator->setToolsEnabled( allowTools );
        d->annotator->setTextToolsEnabled( allowTools && d->document->supportsSearching() );
    }

    // initialize/reset annotator (and show/hide toolbar)
    d->annotator->setEnabled( on );
    d->annotator->setHidingForced( false );

    inHere = false;
}

void PageView::slotAutoScrollUp()
{
    if ( d->scrollIncrement < -9 )
        return;
    d->scrollIncrement--;
    slotAutoScroll();
    setFocus();
}

void PageView::slotAutoScrollDown()
{
    if ( d->scrollIncrement > 9 )
        return;
    d->scrollIncrement++;
    slotAutoScroll();
    setFocus();
}

void PageView::slotScrollUp( bool singleStep )
{
    // if in single page mode and at the top of the screen, go to \ page
    if ( Okular::Settings::viewContinuous() || verticalScrollBar()->value() > verticalScrollBar()->minimum() )
    {
        if ( singleStep )
            verticalScrollBar()->triggerAction( QScrollBar::SliderSingleStepSub );
        else
            verticalScrollBar()->triggerAction( QScrollBar::SliderPageStepSub );
    }
    else if ( d->document->currentPage() > 0 )
    {
        // more optimized than document->setPrevPage and then move view to bottom
        Okular::DocumentViewport newViewport = d->document->viewport();
        newViewport.pageNumber -= viewColumns();
        if ( newViewport.pageNumber < 0 )
            newViewport.pageNumber = 0;
        newViewport.rePos.enabled = true;
        newViewport.rePos.normalizedY = 1.0;
        d->document->setViewport( newViewport );
    }
}

void PageView::slotScrollDown( bool singleStep )
{
    // if in single page mode and at the bottom of the screen, go to next page
    if ( Okular::Settings::viewContinuous() || verticalScrollBar()->value() < verticalScrollBar()->maximum() )
    {
        if ( singleStep )
            verticalScrollBar()->triggerAction( QScrollBar::SliderSingleStepAdd );
        else
            verticalScrollBar()->triggerAction( QScrollBar::SliderPageStepAdd );
    }
    else if ( (int)d->document->currentPage() < d->items.count() - 1 )
    {
        // more optimized than document->setNextPage and then move view to top
        Okular::DocumentViewport newViewport = d->document->viewport();
        newViewport.pageNumber += viewColumns();
        if ( newViewport.pageNumber >= (int)d->items.count() )
            newViewport.pageNumber = d->items.count() - 1;
        newViewport.rePos.enabled = true;
        newViewport.rePos.normalizedY = 0.0;
        d->document->setViewport( newViewport );
    }
}

void PageView::slotRotateClockwise()
{
    int id = ( (int)d->document->rotation() + 1 ) % 4;
    d->document->setRotation( id );
}

void PageView::slotRotateCounterClockwise()
{
    int id = ( (int)d->document->rotation() + 3 ) % 4;
    d->document->setRotation( id );
}

void PageView::slotRotateOriginal()
{
    d->document->setRotation( 0 );
}

void PageView::slotPageSizes( int newsize )
{
    if ( newsize < 0 || newsize >= d->document->pageSizes().count() )
        return;

    d->document->setPageSize( d->document->pageSizes().at( newsize ) );
}

// Enforce mutual-exclusion between trim modes
// Each mode is uniquely identified by a single value
// From Okular::Settings::EnumTrimMode
void PageView::updateTrimMode( int except_id ) {
    const QList<QAction *> trimModeActions = d->aTrimMode->menu()->actions();
    foreach(QAction *trimModeAction, trimModeActions)
    {
        if (trimModeAction->data().toInt() != except_id)
            trimModeAction->setChecked( false );
    }
}

bool PageView::mouseReleaseOverLink( const Okular::ObjectRect * rect ) const
{
    if ( rect )
    {
        // handle click over a link
        const Okular::Action * action = static_cast< const Okular::Action * >( rect->object() );
        d->document->processAction( action );
        return true;
    }
    return false;
}

void PageView::slotTrimMarginsToggled( bool on )
{
    if (on) { // Turn off any other Trim modes
       updateTrimMode(d->aTrimMargins->data().toInt());
    }

    if ( Okular::Settings::trimMargins() != on )
    {
        Okular::Settings::setTrimMargins( on );
        Okular::Settings::self()->save();
        if ( d->document->pages() > 0 )
        {
            slotRelayoutPages();
            slotRequestVisiblePixmaps(); // TODO: slotRelayoutPages() may have done this already!
        }
    }
}

void PageView::slotTrimToSelectionToggled( bool on )
{
    if ( on ) { // Turn off any other Trim modes
        updateTrimMode(d->aTrimToSelection->data().toInt());

        d->mouseMode = Okular::Settings::EnumMouseMode::TrimSelect;
        // change the text in messageWindow (and show it if hidden)
        d->messageWindow->display( i18n( "Draw a rectangle around the page area you wish to keep visible" ), QString(), PageViewMessage::Info, -1 );
        // force hiding of annotator toolbar
        if ( d->aToggleAnnotator && d->aToggleAnnotator->isChecked() )
        {
           d->aToggleAnnotator->trigger();
           d->annotator->setHidingForced( true );
        }
        // force an update of the cursor
        updateCursor();
    } else {

        // toggled off while making selection
        if ( Okular::Settings::EnumMouseMode::TrimSelect == d->mouseMode ) {
            // clear widget selection and invalidate rect
            selectionClear();

            // When Trim selection bbox interaction is over, we should switch to another mousemode.
            if ( d->aPrevAction )
            {
                d->aPrevAction->trigger();
                d->aPrevAction = nullptr;
            } else {
                d->aMouseNormal->trigger();
            }
        }

        d->trimBoundingBox = Okular::NormalizedRect(); // invalidate box
        if ( d->document->pages() > 0 )
        {
            slotRelayoutPages();
            slotRequestVisiblePixmaps(); // TODO: slotRelayoutPages() may have done this already!
        }
    }

}

void PageView::slotToggleForms()
{
    toggleFormWidgets( !d->m_formsVisible );
}

void PageView::slotFormChanged( int pageNumber )
{
    if ( !d->refreshTimer )
    {
        d->refreshTimer = new QTimer( this );
        d->refreshTimer->setSingleShot( true );
        connect( d->refreshTimer, &QTimer::timeout,
                 this, &PageView::slotRefreshPage );
    }
    d->refreshPages << pageNumber;
    int delay = 0;
    if ( d->m_formsVisible )
    {
        delay = 1000;
    }
    d->refreshTimer->start( delay );
}

void PageView::slotRefreshPage()
{
    foreach(int req, d->refreshPages)
    {
        QMetaObject::invokeMethod( d->document, "refreshPixmaps", Qt::QueuedConnection,
                                   Q_ARG( int, req ) );
    }
    d->refreshPages.clear();
}

#ifdef HAVE_SPEECH
void PageView::slotSpeakDocument()
{
    QString text;
    QVector< PageViewItem * >::const_iterator it = d->items.constBegin(), itEnd = d->items.constEnd();
    for ( ; it < itEnd; ++it )
    {
        Okular::RegularAreaRect * area = textSelectionForItem( *it );
        text.append( (*it)->page()->text( area ) );
        text.append( '\n' );
        delete area;
    }

    d->tts()->say( text );
}

void PageView::slotSpeakCurrentPage()
{
    const int currentPage = d->document->viewport().pageNumber;

    PageViewItem *item = d->items.at( currentPage );
    Okular::RegularAreaRect * area = textSelectionForItem( item );
    const QString text = item->page()->text( area );
    delete area;

    d->tts()->say( text );
}

void PageView::slotStopSpeaks()
{
    if ( !d->m_tts )
        return;

    d->m_tts->stopAllSpeechs();
}
#endif

void PageView::slotAction( Okular::Action *action )
{
    d->document->processAction( action );
}

void PageView::externalKeyPressEvent( QKeyEvent *e )
{
    keyPressEvent( e );
}

void PageView::slotProcessMovieAction( const Okular::MovieAction *action )
{
    const Okular::MovieAnnotation *movieAnnotation = action->annotation();
    if ( !movieAnnotation )
        return;

    Okular::Movie *movie = movieAnnotation->movie();
    if ( !movie )
        return;

    const int currentPage = d->document->viewport().pageNumber;

    PageViewItem *item = d->items.at( currentPage );
    if ( !item )
        return;

    VideoWidget *vw = item->videoWidgets().value( movie );
    if ( !vw )
        return;

    vw->show();

    switch ( action->operation() )
    {
        case Okular::MovieAction::Play:
            vw->stop();
            vw->play();
            break;
        case Okular::MovieAction::Stop:
            vw->stop();
            break;
        case Okular::MovieAction::Pause:
            vw->pause();
            break;
        case Okular::MovieAction::Resume:
            vw->play();
            break;
    };
}

void PageView::slotProcessRenditionAction( const Okular::RenditionAction *action )
{
    Okular::Movie *movie = action->movie();
    if ( !movie )
        return;

    const int currentPage = d->document->viewport().pageNumber;

    PageViewItem *item = d->items.at( currentPage );
    if ( !item )
        return;

    VideoWidget *vw = item->videoWidgets().value( movie );
    if ( !vw )
        return;

    if ( action->operation() == Okular::RenditionAction::None )
        return;

    vw->show();

    switch ( action->operation() )
    {
        case Okular::RenditionAction::Play:
            vw->stop();
            vw->play();
            break;
        case Okular::RenditionAction::Stop:
            vw->stop();
            break;
        case Okular::RenditionAction::Pause:
            vw->pause();
            break;
        case Okular::RenditionAction::Resume:
            vw->play();
            break;
        default:
            return;
    };
}

void PageView::slotSetChangeColors(bool active)
{
    Okular::SettingsCore::setChangeColors(active);
    Okular::Settings::self()->save();
    viewport()->update();
}

void PageView::slotToggleChangeColors()
{
    slotSetChangeColors( !Okular::SettingsCore::changeColors() );
}

void PageView::slotFitWindowToPage()
{
    PageViewItem currentPageItem = nullptr;
    QSize viewportSize = viewport()->size();
    foreach ( const PageViewItem * pageItem, d->items )
    {
        if ( pageItem->isVisible() )
        {
            currentPageItem = *pageItem;
            break;
        }
    }
    const QSize pageSize = QSize( currentPageItem.uncroppedWidth() + kcolWidthMargin, currentPageItem.uncroppedHeight() + krowHeightMargin );
    if ( verticalScrollBar()->isVisible() )
        viewportSize.setWidth( viewportSize.width() + verticalScrollBar()->width() );
    if ( horizontalScrollBar()->isVisible() )
        viewportSize.setHeight( viewportSize.height() + horizontalScrollBar()->height() );
    emit fitWindowToPage( viewportSize, pageSize );
}

//END private SLOTS

#include "moc_pageview.cpp"

/* kate: replace-tabs on; indent-width 4; */
