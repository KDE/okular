/*
    SPDX-FileCopyrightText: 2004-2005 Enrico Ros <eros.kde@email.it>
    SPDX-FileCopyrightText: 2004-2006 Albert Astals Cid <aacid@kde.org>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    With portions of code from kpdf/kpdf_pagewidget.cc by:
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2003 Christophe Devriese <Christophe.Devriese@student.kuleuven.ac.be>
    SPDX-FileCopyrightText: 2003 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2003 Dirk Mueller <mueller@kde.org>
    SPDX-FileCopyrightText: 2004 James Ots <kde@jamesots.com>
    SPDX-FileCopyrightText: 2011 Jiri Baum - NICTA <jiri@baum.com.au>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pageview.h"

// qt/kde includes
#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QDesktopServices>
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#include <QDesktopWidget>
#endif
#include <QElapsedTimer>
#include <QEvent>
#include <QGestureEvent>
#include <QImage>
#include <QInputDialog>
#include <QLoggingCategory>
#include <QMenu>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPainter>
#include <QScrollBar>
#include <QScroller>
#include <QScrollerProperties>
#include <QSet>
#include <QTimer>
#include <QToolTip>

#include <KActionCollection>
#include <KActionMenu>
#include <KConfigWatcher>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRun>
#include <KSelectAction>
#include <KStandardAction>
#include <KStringHandler>
#include <KToggleAction>
#include <KToolInvocation>
#include <KUriFilter>
#include <QAction>
#include <QDebug>
#include <QIcon>
#include <kwidgetsaddons_version.h>

// system includes
#include <array>
#include <math.h>
#include <stdlib.h>

// local includes
#include "annotationpopup.h"
#include "annotwindow.h"
#include "colormodemenu.h"
#include "core/annotations.h"
#include "cursorwraphelper.h"
#include "debug_ui.h"
#include "formwidgets.h"
#include "guiutils.h"
#include "okmenutitle.h"
#include "pagepainter.h"
#include "pageviewannotator.h"
#include "pageviewmouseannotation.h"
#include "pageviewutils.h"
#include "priorities.h"
#include "toggleactionmenu.h"
#ifdef HAVE_SPEECH
#include "tts.h"
#endif
#include "core/action.h"
#include "core/audioplayer.h"
#include "core/document_p.h"
#include "core/form.h"
#include "core/generator.h"
#include "core/misc.h"
#include "core/movie.h"
#include "core/page.h"
#include "core/page_p.h"
#include "core/sourcereference.h"
#include "core/tile.h"
#include "magnifierview.h"
#include "settings.h"
#include "settings_core.h"
#include "url_utils.h"
#include "videowidget.h"

static const int pageflags = PagePainter::Accessibility | PagePainter::EnhanceLinks | PagePainter::EnhanceImages | PagePainter::Highlights | PagePainter::TextSelection | PagePainter::Annotations;

static const std::array<float, 16> kZoomValues {0.12, 0.25, 0.33, 0.50, 0.66, 0.75, 1.00, 1.25, 1.50, 2.00, 4.00, 8.00, 16.00, 25.00, 50.00, 100.00};

// This is the length of the text that will be shown when the user is searching for a specific piece of text.
static const int searchTextPreviewLength = 21;

// When following a link, only a preview of this length will be used to set the text of the action.
static const int linkTextPreviewLength = 30;

static inline double normClamp(double value, double def)
{
    return (value < 0.0 || value > 1.0) ? def : value;
}

struct TableSelectionPart {
    PageViewItem *item;
    Okular::NormalizedRect rectInItem;
    Okular::NormalizedRect rectInSelection;

    TableSelectionPart(PageViewItem *item_p, const Okular::NormalizedRect &rectInItem_p, const Okular::NormalizedRect &rectInSelection_p);
};

TableSelectionPart::TableSelectionPart(PageViewItem *item_p, const Okular::NormalizedRect &rectInItem_p, const Okular::NormalizedRect &rectInSelection_p)
    : item(item_p)
    , rectInItem(rectInItem_p)
    , rectInSelection(rectInSelection_p)
{
}

// structure used internally by PageView for data storage
class PageViewPrivate
{
public:
    PageViewPrivate(PageView *qq);

    FormWidgetsController *formWidgetsController();
#ifdef HAVE_SPEECH
    OkularTTS *tts();
#endif
    QString selectedText() const;

    // the document, pageviewItems and the 'visible cache'
    PageView *q;
    Okular::Document *document;
    QVector<PageViewItem *> items;
    QLinkedList<PageViewItem *> visibleItems;
    MagnifierView *magnifierView;

    // view layout (columns in Settings), zoom and mouse
    PageView::ZoomMode zoomMode;
    float zoomFactor;
    QPoint mouseGrabOffset;
    QPoint mousePressPos;
    QPoint mouseSelectPos;
    QPoint previousMouseMovePos;
    int mouseMidLastY;
    bool mouseSelecting;
    QRect mouseSelectionRect;
    QColor mouseSelectionColor;
    bool mouseTextSelecting;
    QSet<int> pagesWithTextSelection;
    bool mouseOnRect;
    int mouseMode;
    MouseAnnotation *mouseAnnotation;

    // table selection
    QList<double> tableSelectionCols;
    QList<double> tableSelectionRows;
    QList<TableSelectionPart> tableSelectionParts;
    bool tableDividersGuessed;

    int lastSourceLocationViewportPageNumber;
    double lastSourceLocationViewportNormalizedX;
    double lastSourceLocationViewportNormalizedY;
    int controlWheelAccumulatedDelta;

    // for everything except PgUp/PgDn and scroll to arbitrary locations
    const int baseShortScrollDuration = 100;
    int currentShortScrollDuration;
    // for PgUp/PgDn and scroll to arbitrary locations
    const int baseLongScrollDuration = baseShortScrollDuration * 2;
    int currentLongScrollDuration;

    // auto scroll
    int scrollIncrement;
    QTimer *autoScrollTimer;
    // annotations
    PageViewAnnotator *annotator;
    // text annotation dialogs list
    QSet<AnnotWindow *> m_annowindows;
    // other stuff
    QTimer *delayResizeEventTimer;
    bool dirtyLayout;
    bool blockViewport;             // prevents changes to viewport
    bool blockPixmapsRequest;       // prevent pixmap requests
    PageViewMessage *messageWindow; // in pageviewutils.h
    bool m_formsVisible;
    FormWidgetsController *formsWidgetController;
#ifdef HAVE_SPEECH
    OkularTTS *m_tts;
#endif
    QTimer *refreshTimer;
    QSet<int> refreshPages;

    // bbox state for Trim to Selection mode
    Okular::NormalizedRect trimBoundingBox;

    // infinite resizing loop prevention
    bool verticalScrollBarVisible = false;
    bool horizontalScrollBarVisible = false;

    // drag scroll
    QPoint dragScrollVector;
    QTimer dragScrollTimer;

    // left click depress
    QTimer leftClickTimer;

    // actions
    QAction *aRotateClockwise;
    QAction *aRotateCounterClockwise;
    QAction *aRotateOriginal;
    KActionMenu *aTrimMode;
    KToggleAction *aTrimMargins;
    KToggleAction *aReadingDirection;
    QAction *aMouseNormal;
    QAction *aMouseZoom;
    QAction *aMouseSelect;
    QAction *aMouseTextSelect;
    QAction *aMouseTableSelect;
    QAction *aMouseMagnifier;
    KToggleAction *aTrimToSelection;
    QAction *aSignature;
    KSelectAction *aZoom;
    QAction *aZoomIn;
    QAction *aZoomOut;
    QAction *aZoomActual;
    KToggleAction *aZoomFitWidth;
    KToggleAction *aZoomFitPage;
    KToggleAction *aZoomAutoFit;
    KActionMenu *aViewModeMenu;
    QActionGroup *viewModeActionGroup;
    ColorModeMenu *aColorModeMenu;
    KToggleAction *aViewContinuous;
    QAction *aPrevAction;
    KToggleAction *aToggleForms;
    QAction *aSpeakDoc;
    QAction *aSpeakPage;
    QAction *aSpeakStop;
    QAction *aSpeakPauseResume;
    KActionCollection *actionCollection;
    QActionGroup *mouseModeActionGroup;
    ToggleActionMenu *aMouseModeMenu;
    QAction *aFitWindowToPage;

    int setting_viewCols;
    bool rtl_Mode;
    // Keep track of whether tablet pen is currently pressed down
    bool penDown;

    // Keep track of mouse over link object
    const Okular::ObjectRect *mouseOverLinkObject;

    QScroller *scroller;
};

PageViewPrivate::PageViewPrivate(PageView *qq)
    : q(qq)
#ifdef HAVE_SPEECH
    , m_tts(nullptr)
#endif
{
}

FormWidgetsController *PageViewPrivate::formWidgetsController()
{
    if (!formsWidgetController) {
        formsWidgetController = new FormWidgetsController(document);
        QObject::connect(formsWidgetController, &FormWidgetsController::changed, q, &PageView::slotFormChanged);
        QObject::connect(formsWidgetController, &FormWidgetsController::action, q, &PageView::slotAction);
        QObject::connect(formsWidgetController, &FormWidgetsController::formatAction, q, [this](const Okular::Action *action, Okular::FormFieldText *fft) { document->processFormatAction(action, fft); });
        QObject::connect(formsWidgetController, &FormWidgetsController::keystrokeAction, q, [this](const Okular::Action *action, Okular::FormFieldText *fft, bool &ok) { document->processKeystrokeAction(action, fft, ok); });
        QObject::connect(formsWidgetController, &FormWidgetsController::focusAction, q, [this](const Okular::Action *action, Okular::FormFieldText *fft) { document->processFocusAction(action, fft); });
        QObject::connect(formsWidgetController, &FormWidgetsController::validateAction, q, [this](const Okular::Action *action, Okular::FormFieldText *fft, bool &ok) { document->processValidateAction(action, fft, ok); });
    }

    return formsWidgetController;
}

#ifdef HAVE_SPEECH
OkularTTS *PageViewPrivate::tts()
{
    if (!m_tts) {
        m_tts = new OkularTTS(q);
        if (aSpeakStop) {
            QObject::connect(m_tts, &OkularTTS::canPauseOrResume, aSpeakStop, &QAction::setEnabled);
        }

        if (aSpeakPauseResume) {
            QObject::connect(m_tts, &OkularTTS::canPauseOrResume, aSpeakPauseResume, &QAction::setEnabled);
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
PageView::PageView(QWidget *parent, Okular::Document *document)
    : QAbstractScrollArea(parent)
    , Okular::View(QStringLiteral("PageView"))
{
    // create and initialize private storage structure
    d = new PageViewPrivate(this);
    d->document = document;
    d->aRotateClockwise = nullptr;
    d->aRotateCounterClockwise = nullptr;
    d->aRotateOriginal = nullptr;
    d->aViewModeMenu = nullptr;
    d->zoomMode = PageView::ZoomFitWidth;
    d->zoomFactor = 1.0;
    d->mouseSelecting = false;
    d->mouseTextSelecting = false;
    d->mouseOnRect = false;
    d->mouseMode = Okular::Settings::mouseMode();
    d->mouseAnnotation = new MouseAnnotation(this, document);
    d->tableDividersGuessed = false;
    d->lastSourceLocationViewportPageNumber = -1;
    d->lastSourceLocationViewportNormalizedX = 0.0;
    d->lastSourceLocationViewportNormalizedY = 0.0;
    d->controlWheelAccumulatedDelta = 0;
    d->currentShortScrollDuration = d->baseShortScrollDuration;
    d->currentLongScrollDuration = d->baseLongScrollDuration;
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
    d->aTrimMode = nullptr;
    d->aTrimMargins = nullptr;
    d->aTrimToSelection = nullptr;
    d->aReadingDirection = nullptr;
    d->aMouseNormal = nullptr;
    d->aMouseZoom = nullptr;
    d->aMouseSelect = nullptr;
    d->aMouseTextSelect = nullptr;
    d->aSignature = nullptr;
    d->aZoomFitWidth = nullptr;
    d->aZoomFitPage = nullptr;
    d->aZoomAutoFit = nullptr;
    d->aViewModeMenu = nullptr;
    d->aViewContinuous = nullptr;
    d->viewModeActionGroup = nullptr;
    d->aColorModeMenu = nullptr;
    d->aPrevAction = nullptr;
    d->aToggleForms = nullptr;
    d->aSpeakDoc = nullptr;
    d->aSpeakPage = nullptr;
    d->aSpeakStop = nullptr;
    d->aSpeakPauseResume = nullptr;
    d->actionCollection = nullptr;
    d->setting_viewCols = Okular::Settings::viewColumns();
    d->rtl_Mode = Okular::Settings::rtlReadingDirection();
    d->mouseModeActionGroup = nullptr;
    d->aMouseModeMenu = nullptr;
    d->penDown = false;
    d->aMouseMagnifier = nullptr;
    d->aFitWindowToPage = nullptr;
    d->trimBoundingBox = Okular::NormalizedRect(); // Null box

    switch (Okular::Settings::zoomMode()) {
    case 0: {
        d->zoomFactor = 1;
        d->zoomMode = PageView::ZoomFixed;
        break;
    }
    case 1: {
        d->zoomMode = PageView::ZoomFitWidth;
        break;
    }
    case 2: {
        d->zoomMode = PageView::ZoomFitPage;
        break;
    }
    case 3: {
        d->zoomMode = PageView::ZoomFitAuto;
        break;
    }
    }

    connect(Okular::Settings::self(), &Okular::Settings::viewContinuousChanged, this, [=]() {
        if (d->aViewContinuous && !d->document->isOpened())
            d->aViewContinuous->setChecked(Okular::Settings::viewContinuous());
    });

    d->delayResizeEventTimer = new QTimer(this);
    d->delayResizeEventTimer->setSingleShot(true);
    d->delayResizeEventTimer->setObjectName(QStringLiteral("delayResizeEventTimer"));
    connect(d->delayResizeEventTimer, &QTimer::timeout, this, &PageView::delayedResizeEvent);

    setFrameStyle(QFrame::NoFrame);

    setAttribute(Qt::WA_StaticContents);

    setObjectName(QStringLiteral("okular::pageView"));

    // viewport setup: setup focus, and track mouse
    viewport()->setFocusProxy(this);
    viewport()->setFocusPolicy(Qt::StrongFocus);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
    viewport()->setMouseTracking(true);
    viewport()->setAutoFillBackground(false);

    d->scroller = QScroller::scroller(viewport());

    QScrollerProperties prop;
    prop.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.3);
    prop.setScrollMetric(QScrollerProperties::MaximumVelocity, 1);
    prop.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, 0.2); // Workaround for QTBUG-88249 (non-flick gestures recognized as accelerating flick)
    prop.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::DragStartDistance, 0.0);
    d->scroller->setScrollerProperties(prop);

    connect(d->scroller, &QScroller::stateChanged, this, &PageView::slotRequestVisiblePixmaps);

    // the apparently "magic" value of 20 is the same used internally in QScrollArea
    verticalScrollBar()->setCursor(Qt::ArrowCursor);
    verticalScrollBar()->setSingleStep(20);
    horizontalScrollBar()->setCursor(Qt::ArrowCursor);
    horizontalScrollBar()->setSingleStep(20);

    // make the smooth scroll animation durations respect the global animation
    // scale
    KConfigWatcher::Ptr animationSpeedWatcher = KConfigWatcher::create(KSharedConfig::openConfig());
    connect(animationSpeedWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (group.name() == QLatin1String("KDE") && names.contains(QByteArrayLiteral("AnimationDurationFactor"))) {
            PageView::updateSmoothScrollAnimationSpeed();
        }
    });

    // connect the padding of the viewport to pixmaps requests
    connect(horizontalScrollBar(), &QAbstractSlider::valueChanged, this, &PageView::slotRequestVisiblePixmaps);
    connect(verticalScrollBar(), &QAbstractSlider::valueChanged, this, &PageView::slotRequestVisiblePixmaps);

    // Keep the scroller in sync with user input on the scrollbars.
    // QAbstractSlider::sliderMoved() and sliderReleased are the intuitive signals,
    // but are only emitted when the “slider is down”, i. e. not when the user scrolls on the scrollbar.
    // QAbstractSlider::actionTriggered() is emitted in all user input cases,
    // but before the value() changes, so we need queued connection here.
    auto update_scroller = [=]() {
        d->scroller->scrollTo(QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value()), 0); // sync scroller with scrollbar
    };
    connect(verticalScrollBar(), &QAbstractSlider::actionTriggered, this, update_scroller, Qt::QueuedConnection);
    connect(horizontalScrollBar(), &QAbstractSlider::actionTriggered, this, update_scroller, Qt::QueuedConnection);

    connect(&d->dragScrollTimer, &QTimer::timeout, this, &PageView::slotDragScroll);

    d->leftClickTimer.setSingleShot(true);
    connect(&d->leftClickTimer, &QTimer::timeout, this, &PageView::slotShowSizeAllCursor);

    // set a corner button to resize the view to the page size
    //    QPushButton * resizeButton = new QPushButton( viewport() );
    //    resizeButton->setPixmap( SmallIcon("crop") );
    //    setCornerWidget( resizeButton );
    //    resizeButton->setEnabled( false );
    // connect(...);
    setAttribute(Qt::WA_InputMethodEnabled, true);

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
    if (d->m_tts)
        d->m_tts->stopAllSpeechs();
#endif

    delete d->mouseAnnotation;

    // delete the local storage structure

    // We need to assign it to a different list otherwise slotAnnotationWindowDestroyed
    // will bite us and clear d->m_annowindows
    QSet<AnnotWindow *> annowindows = d->m_annowindows;
    d->m_annowindows.clear();
    qDeleteAll(annowindows);

    // delete all widgets
    qDeleteAll(d->items);
    delete d->formsWidgetController;
    d->document->removeObserver(this);
    delete d;
}

void PageView::setupBaseActions(KActionCollection *ac)
{
    d->actionCollection = ac;

    // Zoom actions ( higher scales takes lots of memory! )
    d->aZoom = new KSelectAction(QIcon::fromTheme(QStringLiteral("page-zoom")), i18n("Zoom"), this);
    ac->addAction(QStringLiteral("zoom_to"), d->aZoom);
    d->aZoom->setEditable(true);
    d->aZoom->setMaxComboViewCount(kZoomValues.size() + 3);
    connect(d->aZoom, QOverload<QAction *>::of(&KSelectAction::triggered), this, &PageView::slotZoom);
    updateZoomText();

    d->aZoomIn = KStandardAction::zoomIn(this, SLOT(slotZoomIn()), ac);

    d->aZoomOut = KStandardAction::zoomOut(this, SLOT(slotZoomOut()), ac);

    d->aZoomActual = KStandardAction::actualSize(this, &PageView::slotZoomActual, ac);
    d->aZoomActual->setText(i18n("Zoom to 100%"));
}

void PageView::setupViewerActions(KActionCollection *ac)
{
    d->actionCollection = ac;

    ac->setDefaultShortcut(d->aZoomIn, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Plus));
    ac->setDefaultShortcut(d->aZoomOut, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Minus));

    // orientation menu actions
    d->aRotateClockwise = new QAction(QIcon::fromTheme(QStringLiteral("object-rotate-right")), i18n("Rotate &Right"), this);
    d->aRotateClockwise->setIconText(i18nc("Rotate right", "Right"));
    ac->addAction(QStringLiteral("view_orientation_rotate_cw"), d->aRotateClockwise);
    d->aRotateClockwise->setEnabled(false);
    connect(d->aRotateClockwise, &QAction::triggered, this, &PageView::slotRotateClockwise);
    d->aRotateCounterClockwise = new QAction(QIcon::fromTheme(QStringLiteral("object-rotate-left")), i18n("Rotate &Left"), this);
    d->aRotateCounterClockwise->setIconText(i18nc("Rotate left", "Left"));
    ac->addAction(QStringLiteral("view_orientation_rotate_ccw"), d->aRotateCounterClockwise);
    d->aRotateCounterClockwise->setEnabled(false);
    connect(d->aRotateCounterClockwise, &QAction::triggered, this, &PageView::slotRotateCounterClockwise);
    d->aRotateOriginal = new QAction(i18n("Original Orientation"), this);
    ac->addAction(QStringLiteral("view_orientation_original"), d->aRotateOriginal);
    d->aRotateOriginal->setEnabled(false);
    connect(d->aRotateOriginal, &QAction::triggered, this, &PageView::slotRotateOriginal);

    // Trim View actions
    d->aTrimMode = new KActionMenu(i18n("&Trim View"), this);
    d->aTrimMode->setDelayed(false);
    ac->addAction(QStringLiteral("view_trim_mode"), d->aTrimMode);

    d->aTrimMargins = new KToggleAction(QIcon::fromTheme(QStringLiteral("trim-margins")), i18n("&Trim Margins"), d->aTrimMode->menu());
    d->aTrimMode->addAction(d->aTrimMargins);
    ac->addAction(QStringLiteral("view_trim_margins"), d->aTrimMargins);
    d->aTrimMargins->setData(QVariant::fromValue((int)Okular::Settings::EnumTrimMode::Margins));
    connect(d->aTrimMargins, &QAction::toggled, this, &PageView::slotTrimMarginsToggled);
    d->aTrimMargins->setChecked(Okular::Settings::trimMargins());

    d->aTrimToSelection = new KToggleAction(QIcon::fromTheme(QStringLiteral("trim-to-selection")), i18n("Trim To &Selection"), d->aTrimMode->menu());
    d->aTrimMode->addAction(d->aTrimToSelection);
    ac->addAction(QStringLiteral("view_trim_selection"), d->aTrimToSelection);
    d->aTrimToSelection->setData(QVariant::fromValue((int)Okular::Settings::EnumTrimMode::Selection));
    connect(d->aTrimToSelection, &QAction::toggled, this, &PageView::slotTrimToSelectionToggled);

    d->aZoomFitWidth = new KToggleAction(QIcon::fromTheme(QStringLiteral("zoom-fit-width")), i18n("Fit &Width"), this);
    ac->addAction(QStringLiteral("view_fit_to_width"), d->aZoomFitWidth);
    connect(d->aZoomFitWidth, &QAction::toggled, this, &PageView::slotFitToWidthToggled);

    d->aZoomFitPage = new KToggleAction(QIcon::fromTheme(QStringLiteral("zoom-fit-best")), i18n("Fit &Page"), this);
    ac->addAction(QStringLiteral("view_fit_to_page"), d->aZoomFitPage);
    connect(d->aZoomFitPage, &QAction::toggled, this, &PageView::slotFitToPageToggled);

    d->aZoomAutoFit = new KToggleAction(QIcon::fromTheme(QStringLiteral("zoom-fit-best")), i18n("&Auto Fit"), this);
    ac->addAction(QStringLiteral("view_auto_fit"), d->aZoomAutoFit);
    connect(d->aZoomAutoFit, &QAction::toggled, this, &PageView::slotAutoFitToggled);

    d->aFitWindowToPage = new QAction(QIcon::fromTheme(QStringLiteral("zoom-fit-width")), i18n("Fit Wi&ndow to Page"), this);
    d->aFitWindowToPage->setEnabled(Okular::Settings::viewMode() == (int)Okular::Settings::EnumViewMode::Single);
    ac->setDefaultShortcut(d->aFitWindowToPage, QKeySequence(Qt::CTRL | Qt::Key_J));
    ac->addAction(QStringLiteral("fit_window_to_page"), d->aFitWindowToPage);
    connect(d->aFitWindowToPage, &QAction::triggered, this, &PageView::slotFitWindowToPage);

    // View Mode action menu (Single Page, Facing Pages,...(choose), and Continuous (on/off))
    d->aViewModeMenu = new KActionMenu(QIcon::fromTheme(QStringLiteral("view-split-left-right")), i18n("&View Mode"), this);
    d->aViewModeMenu->setDelayed(false);
    ac->addAction(QStringLiteral("view_render_mode"), d->aViewModeMenu);

    d->viewModeActionGroup = new QActionGroup(this);
    auto addViewMode = [=](QAction *a, const QString &name, Okular::Settings::EnumViewMode::type id) {
        a->setCheckable(true);
        a->setData(int(id));
        d->aViewModeMenu->addAction(a);
        ac->addAction(name, a);
        d->viewModeActionGroup->addAction(a);
    };
    addViewMode(new QAction(QIcon::fromTheme(QStringLiteral("view-pages-single")), i18nc("@item:inmenu", "&Single Page"), this), QStringLiteral("view_render_mode_single"), Okular::Settings::EnumViewMode::Single);
    addViewMode(new QAction(QIcon::fromTheme(QStringLiteral("view-pages-facing")), i18nc("@item:inmenu", "&Facing Pages"), this), QStringLiteral("view_render_mode_facing"), Okular::Settings::EnumViewMode::Facing);
    addViewMode(new QAction(QIcon::fromTheme(QStringLiteral("view-pages-facing-first-centered")), i18nc("@item:inmenu", "Facing Pages (&Center First Page)"), this),
                QStringLiteral("view_render_mode_facing_center_first"),
                Okular::Settings::EnumViewMode::FacingFirstCentered);
    addViewMode(new QAction(QIcon::fromTheme(QStringLiteral("view-pages-overview")), i18nc("@item:inmenu", "&Overview"), this), QStringLiteral("view_render_mode_overview"), Okular::Settings::EnumViewMode::Summary);
    const QList<QAction *> viewModeActions = d->viewModeActionGroup->actions();
    for (QAction *viewModeAction : viewModeActions) {
        if (viewModeAction->data().toInt() == Okular::Settings::viewMode()) {
            viewModeAction->setChecked(true);
            break;
        }
    }
    connect(d->viewModeActionGroup, &QActionGroup::triggered, this, &PageView::slotViewMode);

    // Continuous view action, add to view mode action menu.
    d->aViewModeMenu->addSeparator();
    d->aViewContinuous = new KToggleAction(QIcon::fromTheme(QStringLiteral("view-pages-continuous")), i18n("&Continuous"), this);
    d->aViewModeMenu->addAction(d->aViewContinuous);
    ac->addAction(QStringLiteral("view_continuous"), d->aViewContinuous);
    connect(d->aViewContinuous, &QAction::toggled, this, &PageView::slotContinuousToggled);
    d->aViewContinuous->setChecked(Okular::Settings::viewContinuous());

    // Reading direction toggle action. (Checked means RTL, unchecked means LTR.)
    d->aReadingDirection = new KToggleAction(QIcon::fromTheme(QStringLiteral("format-text-direction-rtl")), i18nc("@action page layout", "Use Right to Left Reading Direction"), this);
    d->aReadingDirection->setChecked(Okular::Settings::rtlReadingDirection());
    ac->addAction(QStringLiteral("rtl_page_layout"), d->aReadingDirection);
    connect(d->aReadingDirection, &QAction::toggled, this, &PageView::slotReadingDirectionToggled);
    connect(Okular::SettingsCore::self(), &Okular::SettingsCore::configChanged, this, &PageView::slotUpdateReadingDirectionAction);

    // Mouse mode actions for viewer mode
    d->mouseModeActionGroup = new QActionGroup(this);
    d->mouseModeActionGroup->setExclusive(true);
    d->aMouseNormal = new QAction(QIcon::fromTheme(QStringLiteral("transform-browse")), i18n("&Browse"), this);
    ac->addAction(QStringLiteral("mouse_drag"), d->aMouseNormal);
    connect(d->aMouseNormal, &QAction::triggered, this, &PageView::slotSetMouseNormal);
    d->aMouseNormal->setCheckable(true);
    ac->setDefaultShortcut(d->aMouseNormal, QKeySequence(Qt::CTRL | Qt::Key_1));
    d->aMouseNormal->setActionGroup(d->mouseModeActionGroup);
    d->aMouseNormal->setChecked(Okular::Settings::mouseMode() == Okular::Settings::EnumMouseMode::Browse);

    d->aMouseZoom = new QAction(QIcon::fromTheme(QStringLiteral("page-zoom")), i18n("&Zoom"), this);
    ac->addAction(QStringLiteral("mouse_zoom"), d->aMouseZoom);
    connect(d->aMouseZoom, &QAction::triggered, this, &PageView::slotSetMouseZoom);
    d->aMouseZoom->setCheckable(true);
    ac->setDefaultShortcut(d->aMouseZoom, QKeySequence(Qt::CTRL | Qt::Key_2));
    d->aMouseZoom->setActionGroup(d->mouseModeActionGroup);
    d->aMouseZoom->setChecked(Okular::Settings::mouseMode() == Okular::Settings::EnumMouseMode::Zoom);

    d->aColorModeMenu = new ColorModeMenu(ac, this);
}

// WARNING: 'setupViewerActions' must have been called before this method
void PageView::setupActions(KActionCollection *ac)
{
    d->actionCollection = ac;

    ac->setDefaultShortcuts(d->aZoomIn, KStandardShortcut::zoomIn());
    ac->setDefaultShortcuts(d->aZoomOut, KStandardShortcut::zoomOut());

    // Mouse-Mode actions
    d->aMouseSelect = new QAction(QIcon::fromTheme(QStringLiteral("select-rectangular")), i18n("Area &Selection"), this);
    ac->addAction(QStringLiteral("mouse_select"), d->aMouseSelect);
    connect(d->aMouseSelect, &QAction::triggered, this, &PageView::slotSetMouseSelect);
    d->aMouseSelect->setCheckable(true);
    ac->setDefaultShortcut(d->aMouseSelect, Qt::CTRL | Qt::Key_3);
    d->aMouseSelect->setActionGroup(d->mouseModeActionGroup);

    d->aMouseTextSelect = new QAction(QIcon::fromTheme(QStringLiteral("edit-select-text")), i18n("&Text Selection"), this);
    ac->addAction(QStringLiteral("mouse_textselect"), d->aMouseTextSelect);
    connect(d->aMouseTextSelect, &QAction::triggered, this, &PageView::slotSetMouseTextSelect);
    d->aMouseTextSelect->setCheckable(true);
    ac->setDefaultShortcut(d->aMouseTextSelect, Qt::CTRL | Qt::Key_4);
    d->aMouseTextSelect->setActionGroup(d->mouseModeActionGroup);

    d->aMouseTableSelect = new QAction(QIcon::fromTheme(QStringLiteral("table")), i18n("T&able Selection"), this);
    ac->addAction(QStringLiteral("mouse_tableselect"), d->aMouseTableSelect);
    connect(d->aMouseTableSelect, &QAction::triggered, this, &PageView::slotSetMouseTableSelect);
    d->aMouseTableSelect->setCheckable(true);
    ac->setDefaultShortcut(d->aMouseTableSelect, Qt::CTRL | Qt::Key_5);
    d->aMouseTableSelect->setActionGroup(d->mouseModeActionGroup);

    d->aMouseMagnifier = new QAction(QIcon::fromTheme(QStringLiteral("document-preview")), i18n("&Magnifier"), this);
    ac->addAction(QStringLiteral("mouse_magnifier"), d->aMouseMagnifier);
    connect(d->aMouseMagnifier, &QAction::triggered, this, &PageView::slotSetMouseMagnifier);
    d->aMouseMagnifier->setCheckable(true);
    ac->setDefaultShortcut(d->aMouseMagnifier, Qt::CTRL | Qt::Key_6);
    d->aMouseMagnifier->setActionGroup(d->mouseModeActionGroup);
    d->aMouseMagnifier->setChecked(Okular::Settings::mouseMode() == Okular::Settings::EnumMouseMode::Magnifier);

    // Mouse mode selection tools menu
    d->aMouseModeMenu = new ToggleActionMenu(i18nc("@action", "Selection Tools"), this);
#if KWIDGETSADDONS_VERSION < QT_VERSION_CHECK(5, 77, 0)
    d->aMouseModeMenu->setDelayed(false);
    d->aMouseModeMenu->setStickyMenu(false);
#else
    d->aMouseModeMenu->setPopupMode(QToolButton::MenuButtonPopup);
#endif
    d->aMouseModeMenu->addAction(d->aMouseSelect);
    d->aMouseModeMenu->addAction(d->aMouseTextSelect);
    d->aMouseModeMenu->addAction(d->aMouseTableSelect);
    connect(d->aMouseModeMenu->menu(), &QMenu::triggered, d->aMouseModeMenu, &ToggleActionMenu::setDefaultAction);
    ac->addAction(QStringLiteral("mouse_selecttools"), d->aMouseModeMenu);

    switch (Okular::Settings::mouseMode()) {
    case Okular::Settings::EnumMouseMode::TextSelect:
        d->aMouseTextSelect->setChecked(true);
        d->aMouseModeMenu->setDefaultAction(d->aMouseTextSelect);
        break;
    case Okular::Settings::EnumMouseMode::RectSelect:
        d->aMouseSelect->setChecked(true);
        d->aMouseModeMenu->setDefaultAction(d->aMouseSelect);
        break;
    case Okular::Settings::EnumMouseMode::TableSelect:
        d->aMouseTableSelect->setChecked(true);
        d->aMouseModeMenu->setDefaultAction(d->aMouseTableSelect);
        break;
    default:
        d->aMouseModeMenu->setDefaultAction(d->aMouseTextSelect);
    }

    // Create signature action
    d->aSignature = new QAction(QIcon::fromTheme(QStringLiteral("document-edit-sign")), i18n("Digitally &Sign..."), this);
    ac->addAction(QStringLiteral("add_digital_signature"), d->aSignature);
    connect(d->aSignature, &QAction::triggered, this, &PageView::slotSignature);

    // speak actions
#ifdef HAVE_SPEECH
    d->aSpeakDoc = new QAction(QIcon::fromTheme(QStringLiteral("text-speak")), i18n("Speak Whole Document"), this);
    ac->addAction(QStringLiteral("speak_document"), d->aSpeakDoc);
    d->aSpeakDoc->setEnabled(false);
    connect(d->aSpeakDoc, &QAction::triggered, this, &PageView::slotSpeakDocument);

    d->aSpeakPage = new QAction(QIcon::fromTheme(QStringLiteral("text-speak")), i18n("Speak Current Page"), this);
    ac->addAction(QStringLiteral("speak_current_page"), d->aSpeakPage);
    d->aSpeakPage->setEnabled(false);
    connect(d->aSpeakPage, &QAction::triggered, this, &PageView::slotSpeakCurrentPage);

    d->aSpeakStop = new QAction(QIcon::fromTheme(QStringLiteral("media-playback-stop")), i18n("Stop Speaking"), this);
    ac->addAction(QStringLiteral("speak_stop_all"), d->aSpeakStop);
    d->aSpeakStop->setEnabled(false);
    connect(d->aSpeakStop, &QAction::triggered, this, &PageView::slotStopSpeaks);

    d->aSpeakPauseResume = new QAction(QIcon::fromTheme(QStringLiteral("media-playback-pause")), i18n("Pause/Resume Speaking"), this);
    ac->addAction(QStringLiteral("speak_pause_resume"), d->aSpeakPauseResume);
    d->aSpeakPauseResume->setEnabled(false);
    connect(d->aSpeakPauseResume, &QAction::triggered, this, &PageView::slotPauseResumeSpeech);
#else
    d->aSpeakDoc = nullptr;
    d->aSpeakPage = nullptr;
    d->aSpeakStop = nullptr;
    d->aSpeakPauseResume = nullptr;
#endif

    // Other actions
    QAction *su = new QAction(i18n("Scroll Up"), this);
    ac->addAction(QStringLiteral("view_scroll_up"), su);
    connect(su, &QAction::triggered, this, &PageView::slotAutoScrollUp);
    ac->setDefaultShortcut(su, QKeySequence(Qt::SHIFT | Qt::Key_Up));
    addAction(su);

    QAction *sd = new QAction(i18n("Scroll Down"), this);
    ac->addAction(QStringLiteral("view_scroll_down"), sd);
    connect(sd, &QAction::triggered, this, &PageView::slotAutoScrollDown);
    ac->setDefaultShortcut(sd, QKeySequence(Qt::SHIFT | Qt::Key_Down));
    addAction(sd);

    QAction *spu = new QAction(i18n("Scroll Page Up"), this);
    ac->addAction(QStringLiteral("view_scroll_page_up"), spu);
    connect(spu, &QAction::triggered, this, &PageView::slotScrollUp);
    ac->setDefaultShortcut(spu, QKeySequence(Qt::SHIFT | Qt::Key_Space));
    addAction(spu);

    QAction *spd = new QAction(i18n("Scroll Page Down"), this);
    ac->addAction(QStringLiteral("view_scroll_page_down"), spd);
    connect(spd, &QAction::triggered, this, &PageView::slotScrollDown);
    ac->setDefaultShortcut(spd, QKeySequence(Qt::Key_Space));
    addAction(spd);

    d->aToggleForms = new KToggleAction(i18n("Show Forms"), this);
    ac->addAction(QStringLiteral("view_toggle_forms"), d->aToggleForms);
    connect(d->aToggleForms, &QAction::toggled, this, &PageView::slotToggleForms);
    d->aToggleForms->setEnabled(false);
    toggleFormWidgets(false);

    // Setup undo and redo actions
    QAction *kundo = KStandardAction::create(KStandardAction::Undo, d->document, SLOT(undo()), ac);
    QAction *kredo = KStandardAction::create(KStandardAction::Redo, d->document, SLOT(redo()), ac);
    connect(d->document, &Okular::Document::canUndoChanged, kundo, &QAction::setEnabled);
    connect(d->document, &Okular::Document::canRedoChanged, kredo, &QAction::setEnabled);
    kundo->setEnabled(false);
    kredo->setEnabled(false);

    d->annotator = new PageViewAnnotator(this, d->document);
    connect(d->annotator, &PageViewAnnotator::toolActive, this, [&](bool selected) {
        if (selected) {
            QAction *aMouseMode = d->mouseModeActionGroup->checkedAction();
            if (aMouseMode) {
                aMouseMode->setChecked(false);
            }
        } else {
            switch (d->mouseMode) {
            case Okular::Settings::EnumMouseMode::Browse:
                d->aMouseNormal->setChecked(true);
                break;
            case Okular::Settings::EnumMouseMode::Zoom:
                d->aMouseZoom->setChecked(true);
                break;
            case Okular::Settings::EnumMouseMode::RectSelect:
                d->aMouseSelect->setChecked(true);
                break;
            case Okular::Settings::EnumMouseMode::TableSelect:
                d->aMouseTableSelect->setChecked(true);
                break;
            case Okular::Settings::EnumMouseMode::Magnifier:
                d->aMouseMagnifier->setChecked(true);
                break;
            case Okular::Settings::EnumMouseMode::TextSelect:
                d->aMouseTextSelect->setChecked(true);
                break;
            }
        }
    });
    connect(d->annotator, &PageViewAnnotator::toolActive, d->mouseAnnotation, &MouseAnnotation::reset);
    connect(d->annotator, &PageViewAnnotator::requestOpenFile, this, &PageView::requestOpenFile);
    d->annotator->setupActions(ac);
}

bool PageView::canFitPageWidth() const
{
    return Okular::Settings::viewMode() != Okular::Settings::EnumViewMode::Single || d->zoomMode != ZoomFitWidth;
}

void PageView::fitPageWidth(int page)
{
    // zoom: Fit Width, columns: 1. setActions + relayout + setPage + update
    d->zoomMode = ZoomFitWidth;
    Okular::Settings::setViewMode(Okular::Settings::EnumViewMode::Single);
    d->aZoomFitWidth->setChecked(true);
    d->aZoomFitPage->setChecked(false);
    d->aZoomAutoFit->setChecked(false);
    updateViewMode(Okular::Settings::EnumViewMode::Single);
    viewport()->setUpdatesEnabled(false);
    slotRelayoutPages();
    viewport()->setUpdatesEnabled(true);
    d->document->setViewportPage(page);
    updateZoomText();
    setFocus();
}

void PageView::openAnnotationWindow(Okular::Annotation *annotation, int pageNumber)
{
    if (!annotation)
        return;

    // find the annot window
    AnnotWindow *existWindow = nullptr;
    for (AnnotWindow *aw : qAsConst(d->m_annowindows)) {
        if (aw->annotation() == annotation) {
            existWindow = aw;
            break;
        }
    }

    if (existWindow == nullptr) {
        existWindow = new AnnotWindow(this, annotation, d->document, pageNumber);
        connect(existWindow, &QObject::destroyed, this, &PageView::slotAnnotationWindowDestroyed);

        d->m_annowindows << existWindow;
    } else {
        existWindow->raise();
        existWindow->findChild<KTextEdit *>()->setFocus();
    }

    existWindow->show();
}

void PageView::slotAnnotationWindowDestroyed(QObject *window)
{
    d->m_annowindows.remove(static_cast<AnnotWindow *>(window));
}

void PageView::displayMessage(const QString &message, const QString &details, PageViewMessage::Icon icon, int duration)
{
    if (!Okular::Settings::showOSD()) {
        if (icon == PageViewMessage::Error) {
            if (!details.isEmpty())
                KMessageBox::detailedError(this, message, details);
            else
                KMessageBox::error(this, message);
        }
        return;
    }

    // hide messageWindow if string is empty
    if (message.isEmpty()) {
        d->messageWindow->hide();
        return;
    }

    // display message (duration is length dependent)
    if (duration == -1) {
        duration = 500 + 100 * message.length();
        if (!details.isEmpty())
            duration += 500 + 100 * details.length();
    }
    d->messageWindow->display(message, details, icon, duration);
}

void PageView::reparseConfig()
{
    // set smooth scrolling policies
    PageView::updateSmoothScrollAnimationSpeed();

    // set the scroll bars policies
    Qt::ScrollBarPolicy scrollBarMode = Okular::Settings::showScrollBars() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff;
    if (horizontalScrollBarPolicy() != scrollBarMode) {
        setHorizontalScrollBarPolicy(scrollBarMode);
        setVerticalScrollBarPolicy(scrollBarMode);
    }

    if (Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::Summary && ((int)Okular::Settings::viewColumns() != d->setting_viewCols)) {
        d->setting_viewCols = Okular::Settings::viewColumns();

        slotRelayoutPages();
    }

    if (Okular::Settings::rtlReadingDirection() != d->rtl_Mode) {
        d->rtl_Mode = Okular::Settings::rtlReadingDirection();
        slotRelayoutPages();
    }

    updatePageStep();

    if (d->annotator)
        d->annotator->reparseConfig();

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
    return QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());
}

QPoint PageView::contentAreaPoint(const QPoint pos) const
{
    return pos + contentAreaPosition();
}

QPointF PageView::contentAreaPoint(const QPointF pos) const
{
    return pos + contentAreaPosition();
}

QString PageViewPrivate::selectedText() const
{
    if (pagesWithTextSelection.isEmpty())
        return QString();

    QString text;
    QList<int> selpages = pagesWithTextSelection.values();
    std::sort(selpages.begin(), selpages.end());
    const Okular::Page *pg = nullptr;
    if (selpages.count() == 1) {
        pg = document->page(selpages.first());
        text.append(pg->text(pg->textSelection(), Okular::TextPage::CentralPixelTextAreaInclusionBehaviour));
    } else {
        pg = document->page(selpages.first());
        text.append(pg->text(pg->textSelection(), Okular::TextPage::CentralPixelTextAreaInclusionBehaviour));
        int end = selpages.count() - 1;
        for (int i = 1; i < end; ++i) {
            pg = document->page(selpages.at(i));
            text.append(pg->text(nullptr, Okular::TextPage::CentralPixelTextAreaInclusionBehaviour));
        }
        pg = document->page(selpages.last());
        text.append(pg->text(pg->textSelection(), Okular::TextPage::CentralPixelTextAreaInclusionBehaviour));
    }

    if (text.endsWith('\n')) {
        text.chop(1);
    }
    return text;
}

QMimeData *PageView::getTableContents() const
{
    QString selText;
    QString selHtml;
    QList<double> xs = d->tableSelectionCols;
    QList<double> ys = d->tableSelectionRows;
    xs.prepend(0.0);
    xs.append(1.0);
    ys.prepend(0.0);
    ys.append(1.0);
    selHtml = QString::fromLatin1(
        "<html><head>"
        "<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">"
        "</head><body><table>");
    for (int r = 0; r + 1 < ys.length(); r++) {
        selHtml += QLatin1String("<tr>");
        for (int c = 0; c + 1 < xs.length(); c++) {
            Okular::NormalizedRect cell(xs[c], ys[r], xs[c + 1], ys[r + 1]);
            if (c)
                selText += QLatin1Char('\t');
            QString txt;
            for (const TableSelectionPart &tsp : qAsConst(d->tableSelectionParts)) {
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
                rects.append(cellPart);
                txt += tsp.item->page()->text(&rects, Okular::TextPage::CentralPixelTextAreaInclusionBehaviour);
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

    QMimeData *md = new QMimeData();
    md->setText(selText);
    md->setHtml(selHtml);

    return md;
}

void PageView::copyTextSelection() const
{
    switch (d->mouseMode) {
    case Okular::Settings::EnumMouseMode::TableSelect: {
        QClipboard *cb = QApplication::clipboard();
        cb->setMimeData(getTableContents(), QClipboard::Clipboard);
    } break;

    case Okular::Settings::EnumMouseMode::TextSelect: {
        const QString text = d->selectedText();
        if (!text.isEmpty()) {
            QClipboard *cb = QApplication::clipboard();
            cb->setText(text, QClipboard::Clipboard);
        }
    } break;
    }
}

void PageView::selectAll()
{
    for (const PageViewItem *item : qAsConst(d->items)) {
        Okular::RegularAreaRect *area = textSelectionForItem(item);
        d->pagesWithTextSelection.insert(item->pageNumber());
        d->document->setPageTextSelection(item->pageNumber(), area, palette().color(QPalette::Active, QPalette::Highlight));
    }
}

void PageView::createAnnotationsVideoWidgets(PageViewItem *item, const QLinkedList<Okular::Annotation *> &annotations)
{
    qDeleteAll(item->videoWidgets());
    item->videoWidgets().clear();

    for (Okular::Annotation *a : annotations) {
        if (a->subType() == Okular::Annotation::AMovie) {
            Okular::MovieAnnotation *movieAnn = static_cast<Okular::MovieAnnotation *>(a);
            VideoWidget *vw = new VideoWidget(movieAnn, movieAnn->movie(), d->document, viewport());
            item->videoWidgets().insert(movieAnn->movie(), vw);
            vw->pageInitialized();
        } else if (a->subType() == Okular::Annotation::ARichMedia) {
            Okular::RichMediaAnnotation *richMediaAnn = static_cast<Okular::RichMediaAnnotation *>(a);
            VideoWidget *vw = new VideoWidget(richMediaAnn, richMediaAnn->movie(), d->document, viewport());
            item->videoWidgets().insert(richMediaAnn->movie(), vw);
            vw->pageInitialized();
        } else if (a->subType() == Okular::Annotation::AScreen) {
            const Okular::ScreenAnnotation *screenAnn = static_cast<Okular::ScreenAnnotation *>(a);
            Okular::Movie *movie = GuiUtils::renditionMovieFromScreenAnnotation(screenAnn);
            if (movie) {
                VideoWidget *vw = new VideoWidget(screenAnn, movie, d->document, viewport());
                item->videoWidgets().insert(movie, vw);
                vw->pageInitialized();
            }
        }
    }
}

// BEGIN DocumentObserver inherited methods
void PageView::notifySetup(const QVector<Okular::Page *> &pageSet, int setupFlags)
{
    bool documentChanged = setupFlags & Okular::DocumentObserver::DocumentChanged;
    const bool allowfillforms = d->document->isAllowed(Okular::AllowFillForms);

    // reuse current pages if nothing new
    if ((pageSet.count() == d->items.count()) && !documentChanged && !(setupFlags & Okular::DocumentObserver::NewLayoutForPages)) {
        int count = pageSet.count();
        for (int i = 0; (i < count) && !documentChanged; i++) {
            if ((int)pageSet[i]->number() != d->items[i]->pageNumber()) {
                documentChanged = true;
            } else {
                // even if the document has not changed, allowfillforms may have
                // changed, so update all fields' "canBeFilled" flag
                const QSet<FormWidgetIface *> formWidgetsList = d->items[i]->formWidgets();
                for (FormWidgetIface *w : formWidgetsList)
                    w->setCanBeFilled(allowfillforms);
            }
        }

        if (!documentChanged) {
            if (setupFlags & Okular::DocumentObserver::UrlChanged) {
                // Here with UrlChanged and no document changed it means we
                // need to update all the Annotation* and Form* otherwise
                // they still point to the old document ones, luckily the old ones are still
                // around so we can look for the new ones using unique ids, etc
                d->mouseAnnotation->updateAnnotationPointers();

                for (AnnotWindow *aw : qAsConst(d->m_annowindows)) {
                    Okular::Annotation *newA = d->document->page(aw->pageNumber())->annotation(aw->annotation()->uniqueName());
                    aw->updateAnnotation(newA);
                }

                const QRect viewportRect(horizontalScrollBar()->value(), verticalScrollBar()->value(), viewport()->width(), viewport()->height());
                for (int i = 0; i < count; i++) {
                    PageViewItem *item = d->items[i];
                    const QSet<FormWidgetIface *> fws = item->formWidgets();
                    for (FormWidgetIface *w : fws) {
                        Okular::FormField *f = Okular::PagePrivate::findEquivalentForm(d->document->page(i), w->formField());
                        if (f) {
                            w->setFormField(f);
                        } else {
                            qWarning() << "Lost form field on document save, something is wrong";
                            item->formWidgets().remove(w);
                            delete w;
                        }
                    }

                    // For the video widgets we don't really care about reusing them since they don't contain much info so just
                    // create them again
                    createAnnotationsVideoWidgets(item, pageSet[i]->annotations());
                    const QHash<Okular::Movie *, VideoWidget *> videoWidgets = item->videoWidgets();
                    for (VideoWidget *vw : videoWidgets) {
                        const Okular::NormalizedRect r = vw->normGeometry();
                        vw->setGeometry(qRound(item->uncroppedGeometry().left() + item->uncroppedWidth() * r.left) + 1 - viewportRect.left(),
                                        qRound(item->uncroppedGeometry().top() + item->uncroppedHeight() * r.top) + 1 - viewportRect.top(),
                                        qRound(fabs(r.right - r.left) * item->uncroppedGeometry().width()),
                                        qRound(fabs(r.bottom - r.top) * item->uncroppedGeometry().height()));

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
    qDeleteAll(d->items);
    d->items.clear();
    d->visibleItems.clear();
    d->pagesWithTextSelection.clear();
    toggleFormWidgets(false);
    if (d->formsWidgetController)
        d->formsWidgetController->dropRadioButtons();

    bool haspages = !pageSet.isEmpty();
    bool hasformwidgets = false;
    // create children widgets
    for (const Okular::Page *page : pageSet) {
        PageViewItem *item = new PageViewItem(page);
        d->items.push_back(item);
#ifdef PAGEVIEW_DEBUG
        qCDebug(OkularUiDebug).nospace() << "cropped geom for " << d->items.last()->pageNumber() << " is " << d->items.last()->croppedGeometry();
#endif
        const QLinkedList<Okular::FormField *> pageFields = page->formFields();
        for (Okular::FormField *ff : pageFields) {
            FormWidgetIface *w = FormWidgetFactory::createWidget(ff, viewport());
            if (w) {
                w->setPageItem(item);
                w->setFormWidgetsController(d->formWidgetsController());
                w->setVisibility(false);
                w->setCanBeFilled(allowfillforms);
                item->formWidgets().insert(w);
                hasformwidgets = true;
            }
        }

        createAnnotationsVideoWidgets(item, page->annotations());
    }

    // invalidate layout so relayout/repaint will happen on next viewport change
    if (haspages) {
        // We do a delayed call to slotRelayoutPages but also set the dirtyLayout
        // because we might end up in notifyViewportChanged while slotRelayoutPages
        // has not been done and we don't want that to happen
        d->dirtyLayout = true;
        QMetaObject::invokeMethod(this, "slotRelayoutPages", Qt::QueuedConnection);
    } else {
        // update the mouse cursor when closing because we may have close through a link and
        // want the cursor to come back to the normal cursor
        updateCursor();
        // then, make the message window and scrollbars disappear, and trigger a repaint
        d->messageWindow->hide();
        resizeContentArea(QSize(0, 0));
        viewport()->update(); // when there is no change to the scrollbars, no repaint would
                              // be done and the old document would still be shown
    }

    // OSD (Message balloons) to display pages
    if (documentChanged && pageSet.count() > 0)
        d->messageWindow->display(i18np(" Loaded a one-page document.", " Loaded a %1-page document.", pageSet.count()), QString(), PageViewMessage::Info, 4000);

    updateActionState(haspages, hasformwidgets);

    // We need to assign it to a different list otherwise slotAnnotationWindowDestroyed
    // will bite us and clear d->m_annowindows
    QSet<AnnotWindow *> annowindows = d->m_annowindows;
    d->m_annowindows.clear();
    qDeleteAll(annowindows);

    selectionClear();
}

void PageView::updateActionState(bool haspages, bool hasformwidgets)
{
    if (d->aTrimMargins)
        d->aTrimMargins->setEnabled(haspages);

    if (d->aTrimToSelection)
        d->aTrimToSelection->setEnabled(haspages);

    if (d->aViewModeMenu)
        d->aViewModeMenu->setEnabled(haspages);

    if (d->aViewContinuous)
        d->aViewContinuous->setEnabled(haspages);

    if (d->aZoomFitWidth)
        d->aZoomFitWidth->setEnabled(haspages);
    if (d->aZoomFitPage)
        d->aZoomFitPage->setEnabled(haspages);
    if (d->aZoomAutoFit)
        d->aZoomAutoFit->setEnabled(haspages);

    if (d->aZoom) {
        d->aZoom->selectableActionGroup()->setEnabled(haspages);
        d->aZoom->setEnabled(haspages);
    }
    if (d->aZoomIn)
        d->aZoomIn->setEnabled(haspages);
    if (d->aZoomOut)
        d->aZoomOut->setEnabled(haspages);
    if (d->aZoomActual)
        d->aZoomActual->setEnabled(haspages && d->zoomFactor != 1.0);

    if (d->aColorModeMenu)
        d->aColorModeMenu->setEnabled(haspages);

    if (d->aReadingDirection) {
        d->aReadingDirection->setEnabled(haspages);
    }

    if (d->mouseModeActionGroup)
        d->mouseModeActionGroup->setEnabled(haspages);
    if (d->aMouseModeMenu)
        d->aMouseModeMenu->setEnabled(haspages);

    if (d->aRotateClockwise)
        d->aRotateClockwise->setEnabled(haspages);
    if (d->aRotateCounterClockwise)
        d->aRotateCounterClockwise->setEnabled(haspages);
    if (d->aRotateOriginal)
        d->aRotateOriginal->setEnabled(haspages);
    if (d->aToggleForms) { // may be null if dummy mode is on
        d->aToggleForms->setEnabled(haspages && hasformwidgets);
    }
    bool allowAnnotations = d->document->isAllowed(Okular::AllowNotes);
    if (d->annotator) {
        bool allowTools = haspages && allowAnnotations;
        d->annotator->setToolsEnabled(allowTools);
        d->annotator->setTextToolsEnabled(allowTools && d->document->supportsSearching());
    }

    if (d->aSignature) {
        const bool canSign = d->document->canSign();
        d->aSignature->setEnabled(canSign && haspages);
    }

#ifdef HAVE_SPEECH
    if (d->aSpeakDoc) {
        const bool enablettsactions = haspages ? Okular::Settings::useTTS() : false;
        d->aSpeakDoc->setEnabled(enablettsactions);
        d->aSpeakPage->setEnabled(enablettsactions);
    }
#endif
    if (d->aMouseMagnifier)
        d->aMouseMagnifier->setEnabled(d->document->supportsTiles());
    if (d->aFitWindowToPage)
        d->aFitWindowToPage->setEnabled(haspages && !getContinuousMode());
}

void PageView::setupActionsPostGUIActivated()
{
    d->annotator->setupActionsPostGUIActivated();
}

bool PageView::areSourceLocationsShownGraphically() const
{
    return Okular::Settings::showSourceLocationsGraphically();
}

void PageView::setShowSourceLocationsGraphically(bool show)
{
    if (show == Okular::Settings::showSourceLocationsGraphically()) {
        return;
    }
    Okular::Settings::setShowSourceLocationsGraphically(show);
    viewport()->update();
}

void PageView::setLastSourceLocationViewport(const Okular::DocumentViewport &vp)
{
    if (vp.rePos.enabled) {
        d->lastSourceLocationViewportNormalizedX = normClamp(vp.rePos.normalizedX, 0.5);
        d->lastSourceLocationViewportNormalizedY = normClamp(vp.rePos.normalizedY, 0.0);
    } else {
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

void PageView::notifyViewportChanged(bool smoothMove)
{
    QMetaObject::invokeMethod(this, "slotRealNotifyViewportChanged", Qt::QueuedConnection, Q_ARG(bool, smoothMove));
}

void PageView::slotRealNotifyViewportChanged(bool smoothMove)
{
    // if we are the one changing viewport, skip this notify
    if (d->blockViewport)
        return;

    // block setViewport outgoing calls
    d->blockViewport = true;

    // find PageViewItem matching the viewport description
    const Okular::DocumentViewport &vp = d->document->viewport();
    const PageViewItem *item = nullptr;
    for (const PageViewItem *tmpItem : qAsConst(d->items))
        if (tmpItem->pageNumber() == vp.pageNumber) {
            item = tmpItem;
            break;
        }
    if (!item) {
        qCWarning(OkularUiDebug) << "viewport for page" << vp.pageNumber << "has no matching item!";
        d->blockViewport = false;
        return;
    }
#ifdef PAGEVIEW_DEBUG
    qCDebug(OkularUiDebug) << "document viewport changed";
#endif
    // relayout in "Single Pages" mode or if a relayout is pending
    d->blockPixmapsRequest = true;
    if (!getContinuousMode() || d->dirtyLayout)
        slotRelayoutPages();

    // restore viewport center or use default {x-center,v-top} alignment
    const QPoint centerCoord = viewportToContentArea(vp);

    // if smooth movement requested, setup parameters and start it
    center(centerCoord.x(), centerCoord.y(), smoothMove);

    d->blockPixmapsRequest = false;

    // request visible pixmaps in the current viewport and recompute it
    slotRequestVisiblePixmaps();

    // enable setViewport calls
    d->blockViewport = false;

    if (viewport()) {
        viewport()->update();
    }

    // since the page has moved below cursor, update it
    updateCursor();
}

void PageView::notifyPageChanged(int pageNumber, int changedFlags)
{
    // only handle pixmap / highlight changes notifies
    if (changedFlags & DocumentObserver::Bookmark)
        return;

    if (changedFlags & DocumentObserver::Annotations) {
        const QLinkedList<Okular::Annotation *> annots = d->document->page(pageNumber)->annotations();
        const QLinkedList<Okular::Annotation *>::ConstIterator annItEnd = annots.end();
        QSet<AnnotWindow *>::Iterator it = d->m_annowindows.begin();
        for (; it != d->m_annowindows.end();) {
            QLinkedList<Okular::Annotation *>::ConstIterator annIt = std::find(annots.begin(), annots.end(), (*it)->annotation());
            if (annIt != annItEnd) {
                (*it)->reloadInfo();
                ++it;
            } else {
                AnnotWindow *w = *it;
                it = d->m_annowindows.erase(it);
                // Need to delete after removing from the list
                // otherwise deleting will call slotAnnotationWindowDestroyed which will mess
                // the list and the iterators
                delete w;
            }
        }

        d->mouseAnnotation->notifyAnnotationChanged(pageNumber);
    }

    if (changedFlags & DocumentObserver::BoundingBox) {
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
    for (const PageViewItem *visibleItem : qAsConst(d->visibleItems))
        if (visibleItem->pageNumber() == pageNumber && visibleItem->isVisible()) {
            // update item's rectangle plus the little outline
            QRect expandedRect = visibleItem->croppedGeometry();
            // a PageViewItem is placed in the global page layout,
            // while we need to map its position in the viewport coordinates
            // (to get the correct area to repaint)
            expandedRect.translate(-contentAreaPosition());
            expandedRect.adjust(-1, -1, 3, 3);
            viewport()->update(expandedRect);

            // if we were "zoom-dragging" do not overwrite the "zoom-drag" cursor
            if (cursor().shape() != Qt::SizeVerCursor) {
                // since the page has been regenerated below cursor, update it
                updateCursor();
            }
            break;
        }
}

void PageView::notifyContentsCleared(int changedFlags)
{
    // if pixmaps were cleared, re-ask them
    if (changedFlags & DocumentObserver::Pixmap)
        QMetaObject::invokeMethod(this, "slotRequestVisiblePixmaps", Qt::QueuedConnection);
}

void PageView::notifyZoom(int factor)
{
    if (factor > 0)
        updateZoom(ZoomIn);
    else
        updateZoom(ZoomOut);
}

bool PageView::canUnloadPixmap(int pageNumber) const
{
    if (Okular::SettingsCore::memoryLevel() == Okular::SettingsCore::EnumMemoryLevel::Low || Okular::SettingsCore::memoryLevel() == Okular::SettingsCore::EnumMemoryLevel::Normal) {
        // if the item is visible, forbid unloading
        for (const PageViewItem *visibleItem : qAsConst(d->visibleItems))
            if (visibleItem->pageNumber() == pageNumber)
                return false;
    } else {
        // forbid unloading of the visible items, and of the previous and next
        for (const PageViewItem *visibleItem : qAsConst(d->visibleItems))
            if (abs(visibleItem->pageNumber() - pageNumber) <= 1)
                return false;
    }
    // if hidden premit unloading
    return true;
}

void PageView::notifyCurrentPageChanged(int previous, int current)
{
    if (previous != -1) {
        PageViewItem *item = d->items.at(previous);
        if (item) {
            const QHash<Okular::Movie *, VideoWidget *> videoWidgetsList = item->videoWidgets();
            for (VideoWidget *videoWidget : videoWidgetsList)
                videoWidget->pageLeft();
        }

        // On close, run the widget scripts, needed for running animated PDF
        const Okular::Page *page = d->document->page(previous);
        const QLinkedList<Okular::Annotation *> annotations = page->annotations();
        for (Okular::Annotation *annotation : annotations) {
            if (annotation->subType() == Okular::Annotation::AWidget) {
                Okular::WidgetAnnotation *widgetAnnotation = static_cast<Okular::WidgetAnnotation *>(annotation);
                d->document->processAction(widgetAnnotation->additionalAction(Okular::Annotation::PageClosing));
            }
        }
    }

    if (current != -1) {
        PageViewItem *item = d->items.at(current);
        if (item) {
            const QHash<Okular::Movie *, VideoWidget *> videoWidgetsList = item->videoWidgets();
            for (VideoWidget *videoWidget : videoWidgetsList)
                videoWidget->pageEntered();
        }

        // update zoom text and factor if in a ZoomFit/* zoom mode
        if (d->zoomMode != ZoomFixed)
            updateZoomText();

        // Opening any widget scripts, needed for running animated PDF
        const Okular::Page *page = d->document->page(current);
        const QLinkedList<Okular::Annotation *> annotations = page->annotations();
        for (Okular::Annotation *annotation : annotations) {
            if (annotation->subType() == Okular::Annotation::AWidget) {
                Okular::WidgetAnnotation *widgetAnnotation = static_cast<Okular::WidgetAnnotation *>(annotation);
                d->document->processAction(widgetAnnotation->additionalAction(Okular::Annotation::PageOpening));
            }
        }
    }

    // if the view is paged (or not continuous) and there is a selected annotation,
    // we call reset to avoid creating an artifact in the next page.
    if (!getContinuousMode()) {
        if (d->mouseAnnotation && d->mouseAnnotation->isFocused()) {
            d->mouseAnnotation->reset();
        }
    }
}

// END DocumentObserver inherited methods

// BEGIN View inherited methods
bool PageView::supportsCapability(ViewCapability capability) const
{
    switch (capability) {
    case Zoom:
    case ZoomModality:
    case Continuous:
    case ViewModeModality:
    case TrimMargins:
        return true;
    }
    return false;
}

Okular::View::CapabilityFlags PageView::capabilityFlags(ViewCapability capability) const
{
    switch (capability) {
    case Zoom:
    case ZoomModality:
    case Continuous:
    case ViewModeModality:
    case TrimMargins:
        return CapabilityRead | CapabilityWrite | CapabilitySerializable;
    }
    return NoFlag;
}

QVariant PageView::capability(ViewCapability capability) const
{
    switch (capability) {
    case Zoom:
        return d->zoomFactor;
    case ZoomModality:
        return d->zoomMode;
    case Continuous:
        return getContinuousMode();
    case ViewModeModality: {
        if (d->viewModeActionGroup) {
            const QList<QAction *> actions = d->viewModeActionGroup->actions();
            for (const QAction *action : actions) {
                if (action->isChecked()) {
                    return action->data();
                }
            }
        }
        return QVariant();
    }
    case TrimMargins:
        return d->aTrimMargins ? d->aTrimMargins->isChecked() : false;
    }
    return QVariant();
}

void PageView::setCapability(ViewCapability capability, const QVariant &option)
{
    switch (capability) {
    case Zoom: {
        bool ok = true;
        double factor = option.toDouble(&ok);
        if (ok && factor > 0.0) {
            d->zoomFactor = static_cast<float>(factor);
            updateZoom(ZoomRefreshCurrent);
        }
        break;
    }
    case ZoomModality: {
        bool ok = true;
        int mode = option.toInt(&ok);
        if (ok) {
            if (mode >= 0 && mode < 3)
                updateZoom((ZoomMode)mode);
        }
        break;
    }
    case ViewModeModality: {
        bool ok = true;
        int mode = option.toInt(&ok);
        if (ok) {
            if (mode >= 0 && mode < Okular::Settings::EnumViewMode::COUNT)
                updateViewMode(mode);
        }
        break;
    }
    case Continuous: {
        bool mode = option.toBool();
        d->aViewContinuous->setChecked(mode);
        break;
    }
    case TrimMargins: {
        bool value = option.toBool();
        d->aTrimMargins->setChecked(value);
        slotTrimMarginsToggled(value);
        break;
    }
    }
}

// END View inherited methods

// BEGIN widget events
bool PageView::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture) {
        return gestureEvent(static_cast<QGestureEvent *>(event));
    }

    // do not stop the event
    return QAbstractScrollArea::event(event);
}

bool PageView::gestureEvent(QGestureEvent *event)
{
    QPinchGesture *pinch = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture));

    if (pinch) {
        // Viewport zoom level at the moment where the pinch gesture starts.
        // The viewport zoom level _during_ the gesture will be this value
        // times the relative zoom reported by QGestureEvent.
        static qreal vanillaZoom = d->zoomFactor;

        if (pinch->state() == Qt::GestureStarted) {
            vanillaZoom = d->zoomFactor;
        }

        const QPinchGesture::ChangeFlags changeFlags = pinch->changeFlags();

        // Zoom
        if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged) {
            d->zoomFactor = vanillaZoom * pinch->totalScaleFactor();

            d->blockPixmapsRequest = true;
            updateZoom(ZoomRefreshCurrent);
            d->blockPixmapsRequest = false;
            viewport()->update();
        }

        // Count the number of 90-degree rotations we did since the start of the pinch gesture.
        // Otherwise a pinch turned to 90 degrees and held there will rotate the page again and again.
        static int rotations = 0;

        if (changeFlags & QPinchGesture::RotationAngleChanged) {
            // Rotation angle relative to the accumulated page rotations triggered by the current pinch
            // We actually turn at 80 degrees rather than at 90 degrees.  That's less strain on the hands.
            const qreal relativeAngle = pinch->rotationAngle() - rotations * 90;
            if (relativeAngle > 80) {
                slotRotateClockwise();
                rotations++;
            }
            if (relativeAngle < -80) {
                slotRotateCounterClockwise();
                rotations--;
            }
        }

        if (pinch->state() == Qt::GestureFinished) {
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
    viewportRect.translate(areaPos);
    QRect contentsRect = pe->rect().translated(areaPos).intersected(viewportRect);
    if (!contentsRect.isValid())
        return;

#ifdef PAGEVIEW_DEBUG
    qCDebug(OkularUiDebug) << "paintevent" << contentsRect;
#endif

    // create the screen painter. a pixel painted at contentsX,contentsY
    // appears to the top-left corner of the scrollview.
    QPainter screenPainter(viewport());
    // translate to simulate the scrolled content widget
    screenPainter.translate(-areaPos);

    // selectionRect is the normalized mouse selection rect
    QRect selectionRect = d->mouseSelectionRect;
    if (!selectionRect.isNull())
        selectionRect = selectionRect.normalized();
    // selectionRectInternal without the border
    QRect selectionRectInternal = selectionRect;
    selectionRectInternal.adjust(1, 1, -1, -1);
    // color for blending
    QColor selBlendColor = (selectionRect.width() > 8 || selectionRect.height() > 8) ? d->mouseSelectionColor : Qt::red;

    // subdivide region into rects
    QRegion rgn = pe->region();
    // preprocess rects area to see if it worths or not using subdivision
    uint summedArea = 0;
    for (const QRect &r : rgn) {
        summedArea += r.width() * r.height();
    }
    // very elementary check: SUMj(Region[j].area) is less than boundingRect.area
    const bool useSubdivision = summedArea < (0.6 * contentsRect.width() * contentsRect.height());
    if (!useSubdivision) {
        rgn = contentsRect;
    }

    // iterate over the rects (only one loop if not using subdivision)
    for (const QRect &r : rgn) {
        if (useSubdivision) {
            // set 'contentsRect' to a part of the sub-divided region
            contentsRect = r.translated(areaPos).intersected(viewportRect);
            if (!contentsRect.isValid())
                continue;
        }
#ifdef PAGEVIEW_DEBUG
        qCDebug(OkularUiDebug) << contentsRect;
#endif

        // note: this check will take care of all things requiring alpha blending (not only selection)
        bool wantCompositing = !selectionRect.isNull() && contentsRect.intersects(selectionRect);
        // also alpha-blend when there is a table selection...
        wantCompositing |= !d->tableSelectionParts.isEmpty();

        if (wantCompositing && Okular::Settings::enableCompositing()) {
            // create pixmap and open a painter over it (contents{left,top} becomes pixmap {0,0})
            QPixmap doubleBuffer(contentsRect.size() * devicePixelRatioF());
            doubleBuffer.setDevicePixelRatio(devicePixelRatioF());
            QPainter pixmapPainter(&doubleBuffer);

            pixmapPainter.translate(-contentsRect.left(), -contentsRect.top());

            // 1) Layer 0: paint items and clear bg on unpainted rects
            drawDocumentOnPainter(contentsRect, &pixmapPainter);
            // 2a) Layer 1a: paint (blend) transparent selection (rectangle)
            if (!selectionRect.isNull() && selectionRect.intersects(contentsRect) && !selectionRectInternal.contains(contentsRect)) {
                QRect blendRect = selectionRectInternal.intersected(contentsRect);
                // skip rectangles covered by the selection's border
                if (blendRect.isValid()) {
                    // grab current pixmap into a new one to colorize contents
                    QPixmap blendedPixmap(blendRect.width() * devicePixelRatioF(), blendRect.height() * devicePixelRatioF());
                    blendedPixmap.setDevicePixelRatio(devicePixelRatioF());
                    QPainter p(&blendedPixmap);

                    p.drawPixmap(0,
                                 0,
                                 doubleBuffer,
                                 (blendRect.left() - contentsRect.left()) * devicePixelRatioF(),
                                 (blendRect.top() - contentsRect.top()) * devicePixelRatioF(),
                                 blendRect.width() * devicePixelRatioF(),
                                 blendRect.height() * devicePixelRatioF());

                    QColor blCol = selBlendColor.darker(140);
                    blCol.setAlphaF(0.2);
                    p.fillRect(blendedPixmap.rect(), blCol);
                    p.end();
                    // copy the blended pixmap back to its place
                    pixmapPainter.drawPixmap(blendRect.left(), blendRect.top(), blendedPixmap);
                }
                // draw border (red if the selection is too small)
                pixmapPainter.setPen(selBlendColor);
                pixmapPainter.drawRect(selectionRect.adjusted(0, 0, -1, -1));
            }
            // 2b) Layer 1b: paint (blend) transparent selection (table)
            for (const TableSelectionPart &tsp : qAsConst(d->tableSelectionParts)) {
                QRect selectionPartRect = tsp.rectInItem.geometry(tsp.item->uncroppedWidth(), tsp.item->uncroppedHeight());
                selectionPartRect.translate(tsp.item->uncroppedGeometry().topLeft());
                QRect selectionPartRectInternal = selectionPartRect;
                selectionPartRectInternal.adjust(1, 1, -1, -1);
                if (!selectionPartRect.isNull() && selectionPartRect.intersects(contentsRect) && !selectionPartRectInternal.contains(contentsRect)) {
                    QRect blendRect = selectionPartRectInternal.intersected(contentsRect);
                    // skip rectangles covered by the selection's border
                    if (blendRect.isValid()) {
                        // grab current pixmap into a new one to colorize contents
                        QPixmap blendedPixmap(blendRect.width() * devicePixelRatioF(), blendRect.height() * devicePixelRatioF());
                        blendedPixmap.setDevicePixelRatio(devicePixelRatioF());
                        QPainter p(&blendedPixmap);
                        p.drawPixmap(0,
                                     0,
                                     doubleBuffer,
                                     (blendRect.left() - contentsRect.left()) * devicePixelRatioF(),
                                     (blendRect.top() - contentsRect.top()) * devicePixelRatioF(),
                                     blendRect.width() * devicePixelRatioF(),
                                     blendRect.height() * devicePixelRatioF());

                        QColor blCol = d->mouseSelectionColor.darker(140);
                        blCol.setAlphaF(0.2);
                        p.fillRect(blendedPixmap.rect(), blCol);
                        p.end();
                        // copy the blended pixmap back to its place
                        pixmapPainter.drawPixmap(blendRect.left(), blendRect.top(), blendedPixmap);
                    }
                    // draw border (red if the selection is too small)
                    pixmapPainter.setPen(d->mouseSelectionColor);
                    pixmapPainter.drawRect(selectionPartRect.adjusted(0, 0, -1, -1));
                }
            }
            drawTableDividers(&pixmapPainter);
            // 3a) Layer 1: give annotator painting control
            if (d->annotator && d->annotator->routePaints(contentsRect))
                d->annotator->routePaint(&pixmapPainter, contentsRect);
            // 3b) Layer 1: give mouseAnnotation painting control
            d->mouseAnnotation->routePaint(&pixmapPainter, contentsRect);

            // 4) Layer 2: overlays
            if (Okular::Settings::debugDrawBoundaries()) {
                pixmapPainter.setPen(Qt::blue);
                pixmapPainter.drawRect(contentsRect);
            }

            // finish painting and draw contents
            pixmapPainter.end();
            screenPainter.drawPixmap(contentsRect.left(), contentsRect.top(), doubleBuffer);
        } else {
            // 1) Layer 0: paint items and clear bg on unpainted rects
            drawDocumentOnPainter(contentsRect, &screenPainter);
            // 2a) Layer 1a: paint opaque selection (rectangle)
            if (!selectionRect.isNull() && selectionRect.intersects(contentsRect) && !selectionRectInternal.contains(contentsRect)) {
                screenPainter.setPen(palette().color(QPalette::Active, QPalette::Highlight).darker(110));
                screenPainter.drawRect(selectionRect);
            }
            // 2b) Layer 1b: paint opaque selection (table)
            for (const TableSelectionPart &tsp : qAsConst(d->tableSelectionParts)) {
                QRect selectionPartRect = tsp.rectInItem.geometry(tsp.item->uncroppedWidth(), tsp.item->uncroppedHeight());
                selectionPartRect.translate(tsp.item->uncroppedGeometry().topLeft());
                QRect selectionPartRectInternal = selectionPartRect;
                selectionPartRectInternal.adjust(1, 1, -1, -1);
                if (!selectionPartRect.isNull() && selectionPartRect.intersects(contentsRect) && !selectionPartRectInternal.contains(contentsRect)) {
                    screenPainter.setPen(palette().color(QPalette::Active, QPalette::Highlight).darker(110));
                    screenPainter.drawRect(selectionPartRect);
                }
            }
            drawTableDividers(&screenPainter);
            // 3a) Layer 1: give annotator painting control
            if (d->annotator && d->annotator->routePaints(contentsRect))
                d->annotator->routePaint(&screenPainter, contentsRect);
            // 3b) Layer 1: give mouseAnnotation painting control
            d->mouseAnnotation->routePaint(&screenPainter, contentsRect);

            // 4) Layer 2: overlays
            if (Okular::Settings::debugDrawBoundaries()) {
                screenPainter.setPen(Qt::red);
                screenPainter.drawRect(contentsRect);
            }
        }
    }
}

void PageView::drawTableDividers(QPainter *screenPainter)
{
    if (!d->tableSelectionParts.isEmpty()) {
        screenPainter->setPen(d->mouseSelectionColor.darker());
        if (d->tableDividersGuessed) {
            QPen p = screenPainter->pen();
            p.setStyle(Qt::DashLine);
            screenPainter->setPen(p);
        }
        for (const TableSelectionPart &tsp : qAsConst(d->tableSelectionParts)) {
            QRect selectionPartRect = tsp.rectInItem.geometry(tsp.item->uncroppedWidth(), tsp.item->uncroppedHeight());
            selectionPartRect.translate(tsp.item->uncroppedGeometry().topLeft());
            QRect selectionPartRectInternal = selectionPartRect;
            selectionPartRectInternal.adjust(1, 1, -1, -1);
            for (double col : qAsConst(d->tableSelectionCols)) {
                if (col >= tsp.rectInSelection.left && col <= tsp.rectInSelection.right) {
                    col = (col - tsp.rectInSelection.left) / (tsp.rectInSelection.right - tsp.rectInSelection.left);
                    const int x = selectionPartRect.left() + col * selectionPartRect.width() + 0.5;
                    screenPainter->drawLine(x, selectionPartRectInternal.top(), x, selectionPartRectInternal.top() + selectionPartRectInternal.height());
                }
            }
            for (double row : qAsConst(d->tableSelectionRows)) {
                if (row >= tsp.rectInSelection.top && row <= tsp.rectInSelection.bottom) {
                    row = (row - tsp.rectInSelection.top) / (tsp.rectInSelection.bottom - tsp.rectInSelection.top);
                    const int y = selectionPartRect.top() + row * selectionPartRect.height() + 0.5;
                    screenPainter->drawLine(selectionPartRectInternal.left(), y, selectionPartRectInternal.left() + selectionPartRectInternal.width(), y);
                }
            }
        }
    }
}

void PageView::resizeEvent(QResizeEvent *e)
{
    if (d->items.isEmpty()) {
        resizeContentArea(e->size());
        return;
    }

    if ((d->zoomMode == ZoomFitWidth || d->zoomMode == ZoomFitAuto) && !verticalScrollBar()->isVisible() && qAbs(e->oldSize().height() - e->size().height()) < verticalScrollBar()->width() && d->verticalScrollBarVisible) {
        // this saves us from infinite resizing loop because of scrollbars appearing and disappearing
        // see bug 160628 for more info
        // TODO looks are still a bit ugly because things are left uncentered
        // but better a bit ugly than unusable
        d->verticalScrollBarVisible = false;
        resizeContentArea(e->size());
        return;
    } else if (d->zoomMode == ZoomFitAuto && !horizontalScrollBar()->isVisible() && qAbs(e->oldSize().width() - e->size().width()) < horizontalScrollBar()->height() && d->horizontalScrollBarVisible) {
        // this saves us from infinite resizing loop because of scrollbars appearing and disappearing
        // TODO looks are still a bit ugly because things are left uncentered
        // but better a bit ugly than unusable
        d->horizontalScrollBarVisible = false;
        resizeContentArea(e->size());
        return;
    }

    // start a timer that will refresh the pixmap after 0.2s
    d->delayResizeEventTimer->start(200);
    d->verticalScrollBarVisible = verticalScrollBar()->isVisible();
    d->horizontalScrollBarVisible = horizontalScrollBar()->isVisible();
}

void PageView::keyPressEvent(QKeyEvent *e)
{
    // Ignore ESC key press to send to shell.cpp
    if (e->key() != Qt::Key_Escape) {
        e->accept();
    } else {
        e->ignore();
    }

    // if performing a selection or dyn zooming, disable keys handling
    if ((d->mouseSelecting && e->key() != Qt::Key_Escape) || (QApplication::mouseButtons() & Qt::MiddleButton))
        return;

    // move/scroll page by using keys
    switch (e->key()) {
    case Qt::Key_J:
    case Qt::Key_Down:
        slotScrollDown(1 /* go down 1 step */);
        break;

    case Qt::Key_PageDown:
        slotScrollDown();
        break;

    case Qt::Key_K:
    case Qt::Key_Up:
        slotScrollUp(1 /* go up 1 step */);
        break;

    case Qt::Key_PageUp:
    case Qt::Key_Backspace:
        slotScrollUp();
        break;

    case Qt::Key_Left:
    case Qt::Key_H:
        if (horizontalScrollBar()->maximum() == 0) {
            // if we cannot scroll we go to the previous page vertically
            int next_page = d->document->currentPage() - viewColumns();
            d->document->setViewportPage(next_page);
        } else {
            d->scroller->scrollTo(d->scroller->finalPosition() + QPoint(-horizontalScrollBar()->singleStep(), 0), d->currentShortScrollDuration);
        }
        break;
    case Qt::Key_Right:
    case Qt::Key_L:
        if (horizontalScrollBar()->maximum() == 0) {
            // if we cannot scroll we advance the page vertically
            int next_page = d->document->currentPage() + viewColumns();
            d->document->setViewportPage(next_page);
        } else {
            d->scroller->scrollTo(d->scroller->finalPosition() + QPoint(horizontalScrollBar()->singleStep(), 0), d->currentShortScrollDuration);
        }
        break;
    case Qt::Key_Escape:
        emit escPressed();
        selectionClear(d->tableDividersGuessed ? ClearOnlyDividers : ClearAllSelection);
        d->mousePressPos = QPoint();
        if (d->aPrevAction) {
            d->aPrevAction->trigger();
            d->aPrevAction = nullptr;
        }
        d->mouseAnnotation->routeKeyPressEvent(e);
        break;
    case Qt::Key_Delete:
        d->mouseAnnotation->routeKeyPressEvent(e);
        break;
    case Qt::Key_Shift:
    case Qt::Key_Control:
        if (d->autoScrollTimer) {
            if (d->autoScrollTimer->isActive())
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
    if (d->autoScrollTimer) {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }
}

void PageView::keyReleaseEvent(QKeyEvent *e)
{
    e->accept();

    if (d->annotator && d->annotator->active()) {
        if (d->annotator->routeKeyEvent(e))
            return;
    }

    if (e->key() == Qt::Key_Escape && d->autoScrollTimer) {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }
}

void PageView::inputMethodEvent(QInputMethodEvent *e)
{
    Q_UNUSED(e)
}

void PageView::tabletEvent(QTabletEvent *e)
{
    // Ignore tablet events that we don't care about
    if (!(e->type() == QEvent::TabletPress || e->type() == QEvent::TabletRelease || e->type() == QEvent::TabletMove)) {
        e->ignore();
        return;
    }

    // Determine pen state
    bool penReleased = false;
    if (e->type() == QEvent::TabletPress) {
        d->penDown = true;
    }
    if (e->type() == QEvent::TabletRelease) {
        d->penDown = false;
        penReleased = true;
    }

    // If we're editing an annotation and the tablet pen is either down or just released
    // then dispatch event to annotator
    if (d->annotator && d->annotator->active() && (d->penDown || penReleased)) {
        // accept the event, otherwise it comes back as a mouse event
        e->accept();

        const QPoint eventPos = contentAreaPoint(e->pos());
        PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
        const QPoint localOriginInGlobal = mapToGlobal(QPoint(0, 0));

        // routeTabletEvent will accept or ignore event as appropriate
        d->annotator->routeTabletEvent(e, pageItem, localOriginInGlobal);
    } else {
        e->ignore();
    }
}

void PageView::mouseMoveEvent(QMouseEvent *e)
{
    // For some reason in Qt 5.11.2 (no idea when this started) all wheel
    // events are followed by mouse move events (without changing position),
    // so we only actually reset the controlWheelAccumulatedDelta if there is a mouse movement
    if (e->globalPos() != d->previousMouseMovePos) {
        d->controlWheelAccumulatedDelta = 0;
    }
    d->previousMouseMovePos = e->globalPos();

    // don't perform any mouse action when no document is shown
    if (d->items.isEmpty())
        return;

    // if holding mouse mid button, perform zoom
    if (e->buttons() & Qt::MidButton) {
        int deltaY = d->mouseMidLastY - e->globalPos().y();
        d->mouseMidLastY = e->globalPos().y();

        const float upperZoomLimit = d->document->supportsTiles() ? 99.99 : 3.99;

        // Wrap mouse cursor
        Qt::Edges wrapEdges;
        wrapEdges.setFlag(Qt::TopEdge, d->zoomFactor < upperZoomLimit);
        wrapEdges.setFlag(Qt::BottomEdge, d->zoomFactor > 0.101);

        deltaY += CursorWrapHelper::wrapCursor(e->globalPos(), wrapEdges).y();

        // update zoom level, perform zoom and redraw
        if (deltaY) {
            d->zoomFactor *= (1.0 + ((double)deltaY / 500.0));
            d->blockPixmapsRequest = true;
            updateZoom(ZoomRefreshCurrent);
            d->blockPixmapsRequest = false;
            viewport()->update();
        }
        return;
    }

    const QPoint eventPos = contentAreaPoint(e->pos());

    // if we're editing an annotation, dispatch event to it
    if (d->annotator && d->annotator->active()) {
        PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
        updateCursor(eventPos);
        d->annotator->routeMouseEvent(e, pageItem);
        return;
    }

    bool leftButton = (e->buttons() == Qt::LeftButton);
    bool rightButton = (e->buttons() == Qt::RightButton);

    switch (d->mouseMode) {
    case Okular::Settings::EnumMouseMode::Browse: {
        PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
        if (leftButton) {
            d->leftClickTimer.stop();
            if (pageItem && d->mouseAnnotation->isActive()) {
                // if left button pressed and annotation is focused, forward move event
                d->mouseAnnotation->routeMouseMoveEvent(pageItem, eventPos, leftButton);
            }
            // drag page
            else {
                if (d->scroller->state() == QScroller::Inactive || d->scroller->state() == QScroller::Scrolling) {
                    d->mouseGrabOffset = QPoint(0, 0);
                    d->scroller->handleInput(QScroller::InputPress, e->pos(), e->timestamp() - 1);
                }

                setCursor(Qt::ClosedHandCursor);

                // Wrap mouse cursor
                Qt::Edges wrapEdges;
                wrapEdges.setFlag(Qt::TopEdge, verticalScrollBar()->value() < verticalScrollBar()->maximum());
                wrapEdges.setFlag(Qt::BottomEdge, verticalScrollBar()->value() > verticalScrollBar()->minimum());
                wrapEdges.setFlag(Qt::LeftEdge, horizontalScrollBar()->value() < horizontalScrollBar()->maximum());
                wrapEdges.setFlag(Qt::RightEdge, horizontalScrollBar()->value() > horizontalScrollBar()->minimum());

                d->mouseGrabOffset -= CursorWrapHelper::wrapCursor(e->pos(), wrapEdges);

                d->scroller->handleInput(QScroller::InputMove, e->pos() + d->mouseGrabOffset, e->timestamp());
            }
        } else if (rightButton && !d->mousePressPos.isNull() && d->aMouseSelect) {
            // if mouse moves 5 px away from the press point, switch to 'selection'
            int deltaX = d->mousePressPos.x() - e->globalPos().x(), deltaY = d->mousePressPos.y() - e->globalPos().y();
            if (deltaX > 5 || deltaX < -5 || deltaY > 5 || deltaY < -5) {
                d->aPrevAction = d->aMouseNormal;
                d->aMouseSelect->trigger();
                QPoint newPos = eventPos + QPoint(deltaX, deltaY);
                selectionStart(newPos, palette().color(QPalette::Active, QPalette::Highlight).lighter(120), false);
                updateSelection(eventPos);
                break;
            }
        } else {
            /* Forward move events which are still not yet consumed by "mouse grab" or aMouseSelect */
            d->mouseAnnotation->routeMouseMoveEvent(pageItem, eventPos, leftButton);
            updateCursor();
        }
    } break;

    case Okular::Settings::EnumMouseMode::Zoom:
    case Okular::Settings::EnumMouseMode::RectSelect:
    case Okular::Settings::EnumMouseMode::TableSelect:
    case Okular::Settings::EnumMouseMode::TrimSelect:
        // set second corner of selection
        if (d->mouseSelecting) {
            updateSelection(eventPos);
            d->mouseOverLinkObject = nullptr;
        }
        updateCursor();
        break;

    case Okular::Settings::EnumMouseMode::Magnifier:
        if (e->buttons()) // if any button is pressed at all
        {
            moveMagnifier(e->pos());
            updateMagnifier(eventPos);
        }
        break;

    case Okular::Settings::EnumMouseMode::TextSelect:
        // if mouse moves 5 px away from the press point and the document supports text extraction, do 'textselection'
        if (!d->mouseTextSelecting && !d->mousePressPos.isNull() && d->document->supportsSearching() && ((eventPos - d->mouseSelectPos).manhattanLength() > 5)) {
            d->mouseTextSelecting = true;
        }
        updateSelection(eventPos);
        updateCursor();
        break;
    }
}

void PageView::mousePressEvent(QMouseEvent *e)
{
    d->controlWheelAccumulatedDelta = 0;

    // don't perform any mouse action when no document is shown
    if (d->items.isEmpty())
        return;

    // if performing a selection or dyn zooming, disable mouse press
    if (d->mouseSelecting || (e->button() != Qt::MiddleButton && (e->buttons() & Qt::MiddleButton)))
        return;

    // if the page is scrolling, stop it
    if (d->autoScrollTimer) {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }

    // if pressing mid mouse button while not doing other things, begin 'continuous zoom' mode
    if (e->button() == Qt::MiddleButton) {
        d->mouseMidLastY = e->globalPos().y();
        setCursor(Qt::SizeVerCursor);
        CursorWrapHelper::startDrag();
        return;
    }

    const QPoint eventPos = contentAreaPoint(e->pos());

    // if we're editing an annotation, dispatch event to it
    if (d->annotator && d->annotator->active()) {
        d->scroller->stop();
        PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
        d->annotator->routeMouseEvent(e, pageItem);
        return;
    }

    // trigger history navigation for additional mouse buttons
    if (e->button() == Qt::XButton1) {
        emit mouseBackButtonClick();
        return;
    }
    if (e->button() == Qt::XButton2) {
        emit mouseForwardButtonClick();
        return;
    }

    // update press / 'start drag' mouse position
    d->mousePressPos = e->globalPos();
    CursorWrapHelper::startDrag();

    // handle mode dependent mouse press actions
    bool leftButton = e->button() == Qt::LeftButton, rightButton = e->button() == Qt::RightButton;

    //   Not sure we should erase the selection when clicking with left.
    if (d->mouseMode != Okular::Settings::EnumMouseMode::TextSelect)
        textSelectionClear();

    switch (d->mouseMode) {
    case Okular::Settings::EnumMouseMode::Browse: // drag start / click / link following
    {
        PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
        if (leftButton) {
            if (pageItem) {
                d->mouseAnnotation->routeMousePressEvent(pageItem, eventPos);
            }

            if (!d->mouseOnRect) {
                d->mouseGrabOffset = QPoint(0, 0);
                d->scroller->handleInput(QScroller::InputPress, e->pos(), e->timestamp());
                d->leftClickTimer.start(QApplication::doubleClickInterval() + 10);
            }
        }
    } break;

    case Okular::Settings::EnumMouseMode::Zoom: // set first corner of the zoom rect
        if (leftButton)
            selectionStart(eventPos, palette().color(QPalette::Active, QPalette::Highlight), false);
        else if (rightButton)
            updateZoom(ZoomOut);
        break;

    case Okular::Settings::EnumMouseMode::Magnifier:
        moveMagnifier(e->pos());
        d->magnifierView->show();
        updateMagnifier(eventPos);
        break;

    case Okular::Settings::EnumMouseMode::RectSelect: // set first corner of the selection rect
    case Okular::Settings::EnumMouseMode::TrimSelect:
        if (leftButton) {
            selectionStart(eventPos, palette().color(QPalette::Active, QPalette::Highlight).lighter(120), false);
        }
        break;
    case Okular::Settings::EnumMouseMode::TableSelect:
        if (leftButton) {
            if (d->tableSelectionParts.isEmpty()) {
                selectionStart(eventPos, palette().color(QPalette::Active, QPalette::Highlight).lighter(120), false);
            } else {
                QRect updatedRect;
                for (const TableSelectionPart &tsp : qAsConst(d->tableSelectionParts)) {
                    QRect selectionPartRect = tsp.rectInItem.geometry(tsp.item->uncroppedWidth(), tsp.item->uncroppedHeight());
                    selectionPartRect.translate(tsp.item->uncroppedGeometry().topLeft());

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
                    const int colScore = fromTop < fromBottom ? fromTop : fromBottom;
                    const int rowScore = fromLeft < fromRight ? fromLeft : fromRight;

                    if (colScore < rowScore) {
                        bool deleted = false;
                        for (int i = 0; i < d->tableSelectionCols.length(); i++) {
                            const double col = (d->tableSelectionCols[i] - tsp.rectInSelection.left) / (tsp.rectInSelection.right - tsp.rectInSelection.left);
                            const int colX = selectionPartRect.left() + col * selectionPartRect.width() + 0.5;
                            if (abs(colX - eventPos.x()) <= 3) {
                                d->tableSelectionCols.removeAt(i);
                                deleted = true;

                                break;
                            }
                        }
                        if (!deleted) {
                            double col = eventPos.x() - selectionPartRect.left();
                            col /= selectionPartRect.width(); // at this point, it's normalised within the part
                            col *= (tsp.rectInSelection.right - tsp.rectInSelection.left);
                            col += tsp.rectInSelection.left; // at this point, it's normalised within the whole table

                            d->tableSelectionCols.append(col);
                            std::sort(d->tableSelectionCols.begin(), d->tableSelectionCols.end());
                        }
                    } else {
                        bool deleted = false;
                        for (int i = 0; i < d->tableSelectionRows.length(); i++) {
                            const double row = (d->tableSelectionRows[i] - tsp.rectInSelection.top) / (tsp.rectInSelection.bottom - tsp.rectInSelection.top);
                            const int rowY = selectionPartRect.top() + row * selectionPartRect.height() + 0.5;
                            if (abs(rowY - eventPos.y()) <= 3) {
                                d->tableSelectionRows.removeAt(i);
                                deleted = true;

                                break;
                            }
                        }
                        if (!deleted) {
                            double row = eventPos.y() - selectionPartRect.top();
                            row /= selectionPartRect.height(); // at this point, it's normalised within the part
                            row *= (tsp.rectInSelection.bottom - tsp.rectInSelection.top);
                            row += tsp.rectInSelection.top; // at this point, it's normalised within the whole table

                            d->tableSelectionRows.append(row);
                            std::sort(d->tableSelectionRows.begin(), d->tableSelectionRows.end());
                        }
                    }
                }
                updatedRect.translate(-contentAreaPosition());
                viewport()->update(updatedRect);
            }
        } else if (rightButton && !d->tableSelectionParts.isEmpty()) {
            QMenu menu(this);
            QAction *copyToClipboard = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Table Contents to Clipboard"));
            const bool copyAllowed = d->document->isAllowed(Okular::AllowCopy);

            if (!copyAllowed) {
                copyToClipboard->setEnabled(false);
                copyToClipboard->setText(i18n("Copy forbidden by DRM"));
            }

            QAction *choice = menu.exec(e->globalPos());
            if (choice == copyToClipboard)
                copyTextSelection();
        }
        break;
    case Okular::Settings::EnumMouseMode::TextSelect:
        d->mouseSelectPos = eventPos;
        if (!rightButton) {
            textSelectionClear();
        }
        break;
    }
}

void PageView::mouseReleaseEvent(QMouseEvent *e)
{
    d->controlWheelAccumulatedDelta = 0;

    // stop the drag scrolling
    d->dragScrollTimer.stop();

    d->leftClickTimer.stop();

    const bool leftButton = e->button() == Qt::LeftButton;
    const bool rightButton = e->button() == Qt::RightButton;

    if (d->mouseAnnotation->isActive() && leftButton) {
        // Just finished to move the annotation
        d->mouseAnnotation->routeMouseReleaseEvent();
    }

    // don't perform any mouse action when no document is shown..
    if (d->items.isEmpty()) {
        // ..except for right Clicks (emitted even it viewport is empty)
        if (e->button() == Qt::RightButton)
            emit rightClick(nullptr, e->globalPos());
        return;
    }

    const QPoint eventPos = contentAreaPoint(e->pos());

    // handle mode independent mid bottom zoom
    if (e->button() == Qt::MiddleButton) {
        // request pixmaps since it was disabled during drag
        slotRequestVisiblePixmaps();
        // the cursor may now be over a link.. update it
        updateCursor(eventPos);
        return;
    }

    // if we're editing an annotation, dispatch event to it
    if (d->annotator && d->annotator->active()) {
        PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
        d->annotator->routeMouseEvent(e, pageItem);
        return;
    }

    switch (d->mouseMode) {
    case Okular::Settings::EnumMouseMode::Browse: {
        d->scroller->handleInput(QScroller::InputRelease, e->pos() + d->mouseGrabOffset, e->timestamp());

        // return the cursor to its normal state after dragging
        if (cursor().shape() == Qt::ClosedHandCursor)
            updateCursor(eventPos);

        PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
        const QPoint pressPos = contentAreaPoint(mapFromGlobal(d->mousePressPos));
        const PageViewItem *pageItemPressPos = pickItemOnPoint(pressPos.x(), pressPos.y());

        // if the mouse has not moved since the press, that's a -click-
        if (leftButton && pageItem && pageItem == pageItemPressPos && ((d->mousePressPos - e->globalPos()).manhattanLength() < QApplication::startDragDistance())) {
            if (!mouseReleaseOverLink(d->mouseOverLinkObject) && (e->modifiers() == Qt::ShiftModifier)) {
                const double nX = pageItem->absToPageX(eventPos.x());
                const double nY = pageItem->absToPageY(eventPos.y());
                const Okular::ObjectRect *rect;
                // TODO: find a better way to activate the source reference "links"
                // for the moment they are activated with Shift + left click
                // Search the nearest source reference.
                rect = pageItem->page()->objectRect(Okular::ObjectRect::SourceRef, nX, nY, pageItem->uncroppedWidth(), pageItem->uncroppedHeight());
                if (!rect) {
                    static const double s_minDistance = 0.025; // FIXME?: empirical value?
                    double distance = 0.0;
                    rect = pageItem->page()->nearestObjectRect(Okular::ObjectRect::SourceRef, nX, nY, pageItem->uncroppedWidth(), pageItem->uncroppedHeight(), &distance);
                    // distance is distanceSqr, adapt it to a normalized value
                    distance = distance / (pow(pageItem->uncroppedWidth(), 2) + pow(pageItem->uncroppedHeight(), 2));
                    if (rect && (distance > s_minDistance))
                        rect = nullptr;
                }
                if (rect) {
                    const Okular::SourceReference *ref = static_cast<const Okular::SourceReference *>(rect->object());
                    d->document->processSourceReference(ref);
                } else {
                    const Okular::SourceReference *ref = d->document->dynamicSourceReference(pageItem->pageNumber(), nX * pageItem->page()->width(), nY * pageItem->page()->height());
                    if (ref) {
                        d->document->processSourceReference(ref);
                        delete ref;
                    }
                }
            }
        } else if (rightButton && !d->mouseAnnotation->isModified()) {
            if (pageItem && pageItem == pageItemPressPos && ((d->mousePressPos - e->globalPos()).manhattanLength() < QApplication::startDragDistance())) {
                QMenu *menu = createProcessLinkMenu(pageItem, eventPos);

                const QRect &itemRect = pageItem->uncroppedGeometry();
                const double nX = pageItem->absToPageX(eventPos.x());
                const double nY = pageItem->absToPageY(eventPos.y());

                const QLinkedList<const Okular::ObjectRect *> annotRects = pageItem->page()->objectRects(Okular::ObjectRect::OAnnotation, nX, nY, itemRect.width(), itemRect.height());

                AnnotationPopup annotPopup(d->document, AnnotationPopup::MultiAnnotationMode, this);
                // Do not move annotPopup inside the if, it needs to live until menu->exec()
                if (!annotRects.isEmpty()) {
                    for (const Okular::ObjectRect *annotRect : annotRects) {
                        Okular::Annotation *ann = ((Okular::AnnotationObjectRect *)annotRect)->annotation();
                        if (ann && (ann->subType() != Okular::Annotation::AWidget)) {
                            annotPopup.addAnnotation(ann, pageItem->pageNumber());
                        }
                    }

                    connect(&annotPopup, &AnnotationPopup::openAnnotationWindow, this, &PageView::openAnnotationWindow);

                    if (!menu) {
                        menu = new QMenu(this);
                    }
                    annotPopup.addActionsToMenu(menu);
                }

                if (menu) {
                    menu->exec(e->globalPos());
                    menu->deleteLater();
                } else {
                    // a link can move us to another page or even to another document, there's no point in trying to
                    //  process the click on the image once we have processes the click on the link
                    const Okular::ObjectRect *rect = pageItem->page()->objectRect(Okular::ObjectRect::Image, nX, nY, itemRect.width(), itemRect.height());
                    if (rect) {
                        // handle right click over a image
                    } else {
                        // right click (if not within 5 px of the press point, the mode
                        // had been already changed to 'Selection' instead of 'Normal')
                        emit rightClick(pageItem->page(), e->globalPos());
                    }
                }
            } else {
                // right click (if not within 5 px of the press point, the mode
                // had been already changed to 'Selection' instead of 'Normal')
                emit rightClick(pageItem ? pageItem->page() : nullptr, e->globalPos());
            }
        }
    } break;

    case Okular::Settings::EnumMouseMode::Zoom:
        // if a selection rect has been defined, zoom into it
        if (leftButton && d->mouseSelecting) {
            QRect selRect = d->mouseSelectionRect.normalized();
            if (selRect.width() <= 8 && selRect.height() <= 8) {
                selectionClear();
                break;
            }

            // find out new zoom ratio and normalized view center (relative to the contentsRect)
            double zoom = qMin((double)viewport()->width() / (double)selRect.width(), (double)viewport()->height() / (double)selRect.height());
            double nX = (double)(selRect.left() + selRect.right()) / (2.0 * (double)contentAreaWidth());
            double nY = (double)(selRect.top() + selRect.bottom()) / (2.0 * (double)contentAreaHeight());

            const float upperZoomLimit = d->document->supportsTiles() ? 100.0 : 4.0;
            if (d->zoomFactor <= upperZoomLimit || zoom <= 1.0) {
                d->zoomFactor *= zoom;
                viewport()->setUpdatesEnabled(false);
                updateZoom(ZoomRefreshCurrent);
                viewport()->setUpdatesEnabled(true);
            }

            // recenter view and update the viewport
            center((int)(nX * contentAreaWidth()), (int)(nY * contentAreaHeight()));
            viewport()->update();

            // hide message box and delete overlay window
            selectionClear();
        }
        break;

    case Okular::Settings::EnumMouseMode::Magnifier:
        d->magnifierView->hide();
        break;

    case Okular::Settings::EnumMouseMode::TrimSelect: {
        // if it is a left release checks if is over a previous link press
        if (leftButton && mouseReleaseOverLink(d->mouseOverLinkObject)) {
            selectionClear();
            break;
        }

        // if mouse is released and selection is null this is a rightClick
        if (rightButton && !d->mouseSelecting) {
            break;
        }
        PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
        // ensure end point rests within a page, or ignore
        if (!pageItem) {
            break;
        }
        QRect selectionRect = d->mouseSelectionRect.normalized();

        double nLeft = pageItem->absToPageX(selectionRect.left());
        double nRight = pageItem->absToPageX(selectionRect.right());
        double nTop = pageItem->absToPageY(selectionRect.top());
        double nBottom = pageItem->absToPageY(selectionRect.bottom());
        if (nLeft < 0)
            nLeft = 0;
        if (nTop < 0)
            nTop = 0;
        if (nRight > 1)
            nRight = 1;
        if (nBottom > 1)
            nBottom = 1;
        d->trimBoundingBox = Okular::NormalizedRect(nLeft, nTop, nRight, nBottom);

        // Trim Selection successfully done, hide prompt
        d->messageWindow->hide();

        // clear widget selection and invalidate rect
        selectionClear();

        // When Trim selection bbox interaction is over, we should switch to another mousemode.
        if (d->aPrevAction) {
            d->aPrevAction->trigger();
            d->aPrevAction = nullptr;
        } else {
            d->aMouseNormal->trigger();
        }

        // with d->trimBoundingBox defined, redraw for trim to take visual effect
        if (d->document->pages() > 0) {
            slotRelayoutPages();
            slotRequestVisiblePixmaps(); // TODO: slotRelayoutPages() may have done this already!
        }

        break;
    }
    case Okular::Settings::EnumMouseMode::RectSelect: {
        // if it is a left release checks if is over a previous link press
        if (leftButton && mouseReleaseOverLink(d->mouseOverLinkObject)) {
            selectionClear();
            break;
        }

        // if mouse is released and selection is null this is a rightClick
        if (rightButton && !d->mouseSelecting) {
            PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
            emit rightClick(pageItem ? pageItem->page() : nullptr, e->globalPos());
            break;
        }

        // if a selection is defined, display a popup
        if ((!leftButton && !d->aPrevAction) || (!rightButton && d->aPrevAction) || !d->mouseSelecting)
            break;

        QRect selectionRect = d->mouseSelectionRect.normalized();
        if (selectionRect.width() <= 8 && selectionRect.height() <= 8) {
            selectionClear();
            if (d->aPrevAction) {
                d->aPrevAction->trigger();
                d->aPrevAction = nullptr;
            }
            break;
        }

        // if we support text generation
        QString selectedText;
        if (d->document->supportsSearching()) {
            // grab text in selection by extracting it from all intersected pages
            const Okular::Page *okularPage = nullptr;
            for (const PageViewItem *item : qAsConst(d->items)) {
                if (!item->isVisible())
                    continue;

                const QRect &itemRect = item->croppedGeometry();
                if (selectionRect.intersects(itemRect)) {
                    // request the textpage if there isn't one
                    okularPage = item->page();
                    qCDebug(OkularUiDebug) << "checking if page" << item->pageNumber() << "has text:" << okularPage->hasTextPage();
                    if (!okularPage->hasTextPage())
                        d->document->requestTextPage(okularPage->number());
                    // grab text in the rect that intersects itemRect
                    QRect relativeRect = selectionRect.intersected(itemRect);
                    relativeRect.translate(-item->uncroppedGeometry().topLeft());
                    Okular::RegularAreaRect rects;
                    rects.append(Okular::NormalizedRect(relativeRect, item->uncroppedWidth(), item->uncroppedHeight()));
                    selectedText += okularPage->text(&rects);
                }
            }
        }

        // popup that ask to copy:text and copy/save:image
        QMenu menu(this);
        menu.setObjectName(QStringLiteral("PopupMenu"));
        QAction *textToClipboard = nullptr;
#ifdef HAVE_SPEECH
        QAction *speakText = nullptr;
#endif
        QAction *imageToClipboard = nullptr;
        QAction *imageToFile = nullptr;
        if (d->document->supportsSearching() && !selectedText.isEmpty()) {
            menu.addAction(new OKMenuTitle(&menu, i18np("Text (1 character)", "Text (%1 characters)", selectedText.length())));
            textToClipboard = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy to Clipboard"));
            textToClipboard->setObjectName(QStringLiteral("CopyTextToClipboard"));
            bool copyAllowed = d->document->isAllowed(Okular::AllowCopy);
            if (!copyAllowed) {
                textToClipboard->setEnabled(false);
                textToClipboard->setText(i18n("Copy forbidden by DRM"));
            }
#ifdef HAVE_SPEECH
            if (Okular::Settings::useTTS())
                speakText = menu.addAction(QIcon::fromTheme(QStringLiteral("text-speak")), i18n("Speak Text"));
#endif
            if (copyAllowed) {
                addSearchWithinDocumentAction(&menu, selectedText);
                addWebShortcutsMenu(&menu, selectedText);
            }
        }
        menu.addAction(new OKMenuTitle(&menu, i18n("Image (%1 by %2 pixels)", selectionRect.width(), selectionRect.height())));
        imageToClipboard = menu.addAction(QIcon::fromTheme(QStringLiteral("image-x-generic")), i18n("Copy to Clipboard"));
        imageToFile = menu.addAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save to File..."));
        QAction *choice = menu.exec(e->globalPos());
        // check if the user really selected an action
        if (choice) {
            // IMAGE operation chosen
            if (choice == imageToClipboard || choice == imageToFile) {
                // renders page into a pixmap
                QPixmap copyPix(selectionRect.width(), selectionRect.height());
                QPainter copyPainter(&copyPix);
                copyPainter.translate(-selectionRect.left(), -selectionRect.top());
                drawDocumentOnPainter(selectionRect, &copyPainter);
                copyPainter.end();

                if (choice == imageToClipboard) {
                    // [2] copy pixmap to clipboard
                    QClipboard *cb = QApplication::clipboard();
                    cb->setPixmap(copyPix, QClipboard::Clipboard);
                    if (cb->supportsSelection())
                        cb->setPixmap(copyPix, QClipboard::Selection);
                    d->messageWindow->display(i18n("Image [%1x%2] copied to clipboard.", copyPix.width(), copyPix.height()));
                } else if (choice == imageToFile) {
                    // [3] save pixmap to file
                    QString fileName = QFileDialog::getSaveFileName(this, i18n("Save file"), QString(), i18n("Images (*.png *.jpeg)"));
                    if (fileName.isEmpty())
                        d->messageWindow->display(i18n("File not saved."), QString(), PageViewMessage::Warning);
                    else {
                        QMimeDatabase db;
                        QMimeType mime = db.mimeTypeForUrl(QUrl::fromLocalFile(fileName));
                        QString type;
                        if (!mime.isDefault())
                            type = QStringLiteral("PNG");
                        else
                            type = mime.name().section(QLatin1Char('/'), -1).toUpper();
                        copyPix.save(fileName, qPrintable(type));
                        d->messageWindow->display(i18n("Image [%1x%2] saved to %3 file.", copyPix.width(), copyPix.height(), type));
                    }
                }
            }
            // TEXT operation chosen
            else {
                if (choice == textToClipboard) {
                    // [1] copy text to clipboard
                    QClipboard *cb = QApplication::clipboard();
                    cb->setText(selectedText, QClipboard::Clipboard);
                    if (cb->supportsSelection())
                        cb->setText(selectedText, QClipboard::Selection);
                }
#ifdef HAVE_SPEECH
                else if (choice == speakText) {
                    // [2] speech selection using TTS
                    d->tts()->say(selectedText);
                }
#endif
            }
        }
        // clear widget selection and invalidate rect
        selectionClear();

        // restore previous action if came from it using right button
        if (d->aPrevAction) {
            d->aPrevAction->trigger();
            d->aPrevAction = nullptr;
        }
    } break;

    case Okular::Settings::EnumMouseMode::TableSelect: {
        // if it is a left release checks if is over a previous link press
        if (leftButton && mouseReleaseOverLink(d->mouseOverLinkObject)) {
            selectionClear();
            break;
        }

        // if mouse is released and selection is null this is a rightClick
        if (rightButton && !d->mouseSelecting) {
            PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
            emit rightClick(pageItem ? pageItem->page() : nullptr, e->globalPos());
            break;
        }

        QRect selectionRect = d->mouseSelectionRect.normalized();
        if (selectionRect.width() <= 8 && selectionRect.height() <= 8 && d->tableSelectionParts.isEmpty()) {
            selectionClear();
            if (d->aPrevAction) {
                d->aPrevAction->trigger();
                d->aPrevAction = nullptr;
            }
            break;
        }

        if (d->mouseSelecting) {
            // break up the selection into page-relative pieces
            d->tableSelectionParts.clear();
            const Okular::Page *okularPage = nullptr;
            for (PageViewItem *item : qAsConst(d->items)) {
                if (!item->isVisible())
                    continue;

                const QRect &itemRect = item->croppedGeometry();
                if (selectionRect.intersects(itemRect)) {
                    // request the textpage if there isn't one
                    okularPage = item->page();
                    qCDebug(OkularUiDebug) << "checking if page" << item->pageNumber() << "has text:" << okularPage->hasTextPage();
                    if (!okularPage->hasTextPage())
                        d->document->requestTextPage(okularPage->number());
                    // grab text in the rect that intersects itemRect
                    QRect rectInItem = selectionRect.intersected(itemRect);
                    rectInItem.translate(-item->uncroppedGeometry().topLeft());
                    QRect rectInSelection = selectionRect.intersected(itemRect);
                    rectInSelection.translate(-selectionRect.topLeft());
                    d->tableSelectionParts.append(
                        TableSelectionPart(item, Okular::NormalizedRect(rectInItem, item->uncroppedWidth(), item->uncroppedHeight()), Okular::NormalizedRect(rectInSelection, selectionRect.width(), selectionRect.height())));
                }
            }

            QRect updatedRect = d->mouseSelectionRect.normalized().adjusted(0, 0, 1, 1);
            updatedRect.translate(-contentAreaPosition());
            d->mouseSelecting = false;
            d->mouseSelectionRect.setCoords(0, 0, 0, 0);
            d->tableSelectionCols.clear();
            d->tableSelectionRows.clear();
            guessTableDividers();
            viewport()->update(updatedRect);
        }

        if (!d->document->isAllowed(Okular::AllowCopy)) {
            d->messageWindow->display(i18n("Copy forbidden by DRM"), QString(), PageViewMessage::Info, -1);
            break;
        }

        QClipboard *cb = QApplication::clipboard();
        if (cb->supportsSelection())
            cb->setMimeData(getTableContents(), QClipboard::Selection);

    } break;

    case Okular::Settings::EnumMouseMode::TextSelect:
        // if it is a left release checks if is over a previous link press
        if (leftButton && mouseReleaseOverLink(d->mouseOverLinkObject)) {
            selectionClear();
            break;
        }

        if (d->mouseTextSelecting) {
            d->mouseTextSelecting = false;
            //                    textSelectionClear();
            if (d->document->isAllowed(Okular::AllowCopy)) {
                const QString text = d->selectedText();
                if (!text.isEmpty()) {
                    QClipboard *cb = QApplication::clipboard();
                    if (cb->supportsSelection())
                        cb->setText(text, QClipboard::Selection);
                }
            }
        } else if (!d->mousePressPos.isNull() && rightButton) {
            PageViewItem *item = pickItemOnPoint(eventPos.x(), eventPos.y());
            const Okular::Page *page;
            // if there is text selected in the page
            if (item) {
                QAction *httpLink = nullptr;
                QAction *textToClipboard = nullptr;
                QString url;

                QMenu *menu = createProcessLinkMenu(item, eventPos);
                const bool mouseClickOverLink = (menu != nullptr);
#ifdef HAVE_SPEECH
                QAction *speakText = nullptr;
#endif
                if ((page = item->page())->textSelection()) {
                    if (!menu) {
                        menu = new QMenu(this);
                    }
                    textToClipboard = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Text"));

#ifdef HAVE_SPEECH
                    if (Okular::Settings::useTTS())
                        speakText = menu->addAction(QIcon::fromTheme(QStringLiteral("text-speak")), i18n("Speak Text"));
#endif
                    if (!d->document->isAllowed(Okular::AllowCopy)) {
                        textToClipboard->setEnabled(false);
                        textToClipboard->setText(i18n("Copy forbidden by DRM"));
                    } else {
                        addSearchWithinDocumentAction(menu, d->selectedText());
                        addWebShortcutsMenu(menu, d->selectedText());
                    }

                    // if the right-click was over a link add "Follow This link" instead of "Go to"
                    if (!mouseClickOverLink) {
                        url = UrlUtils::getUrl(d->selectedText());
                        if (!url.isEmpty()) {
                            const QString squeezedText = KStringHandler::rsqueeze(url, linkTextPreviewLength);
                            httpLink = menu->addAction(i18n("Go to '%1'", squeezedText));
                            httpLink->setObjectName(QStringLiteral("GoToAction"));
                        }
                    }
                }

                if (menu) {
                    menu->setObjectName(QStringLiteral("PopupMenu"));

                    QAction *choice = menu->exec(e->globalPos());
                    // check if the user really selected an action
                    if (choice) {
                        if (choice == textToClipboard)
                            copyTextSelection();
#ifdef HAVE_SPEECH
                        else if (choice == speakText) {
                            const QString text = d->selectedText();
                            d->tts()->say(text);
                        }
#endif
                        else if (choice == httpLink) {
                            new KRun(QUrl(url), this);
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
    QList<QPair<double, int>> colTicks, rowTicks, colSelectionTicks, rowSelectionTicks;

    for (const TableSelectionPart &tsp : qAsConst(d->tableSelectionParts)) {
        // add ticks for the edges of this area...
        colSelectionTicks.append(qMakePair(tsp.rectInSelection.left, +1));
        colSelectionTicks.append(qMakePair(tsp.rectInSelection.right, -1));
        rowSelectionTicks.append(qMakePair(tsp.rectInSelection.top, +1));
        rowSelectionTicks.append(qMakePair(tsp.rectInSelection.bottom, -1));

        // get the words in this part
        Okular::RegularAreaRect rects;
        rects.append(tsp.rectInItem);
        const Okular::TextEntity::List words = tsp.item->page()->words(&rects, Okular::TextPage::CentralPixelTextAreaInclusionBehaviour);

        for (const Okular::TextEntity *te : words) {
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
            colTicks.append(qMakePair(wordArea.left, +1));
            colTicks.append(qMakePair(wordArea.right, -1));
            rowTicks.append(qMakePair(wordArea.top, +1));
            rowTicks.append(qMakePair(wordArea.bottom, -1));

            delete te;
        }
    }

    int tally = 0;

    std::sort(colSelectionTicks.begin(), colSelectionTicks.end());
    std::sort(rowSelectionTicks.begin(), rowSelectionTicks.end());

    for (int i = 0; i < colSelectionTicks.length(); ++i) {
        tally += colSelectionTicks[i].second;
        if (tally == 0 && i + 1 < colSelectionTicks.length() && colSelectionTicks[i + 1].first != colSelectionTicks[i].first) {
            colTicks.append(qMakePair(colSelectionTicks[i].first, +1));
            colTicks.append(qMakePair(colSelectionTicks[i + 1].first, -1));
        }
    }
    Q_ASSERT(tally == 0);

    for (int i = 0; i < rowSelectionTicks.length(); ++i) {
        tally += rowSelectionTicks[i].second;
        if (tally == 0 && i + 1 < rowSelectionTicks.length() && rowSelectionTicks[i + 1].first != rowSelectionTicks[i].first) {
            rowTicks.append(qMakePair(rowSelectionTicks[i].first, +1));
            rowTicks.append(qMakePair(rowSelectionTicks[i + 1].first, -1));
        }
    }
    Q_ASSERT(tally == 0);

    std::sort(colTicks.begin(), colTicks.end());
    std::sort(rowTicks.begin(), rowTicks.end());

    for (int i = 0; i < colTicks.length(); ++i) {
        tally += colTicks[i].second;
        if (tally == 0 && i + 1 < colTicks.length() && colTicks[i + 1].first != colTicks[i].first) {
            d->tableSelectionCols.append((colTicks[i].first + colTicks[i + 1].first) / 2);
            d->tableDividersGuessed = true;
        }
    }
    Q_ASSERT(tally == 0);

    for (int i = 0; i < rowTicks.length(); ++i) {
        tally += rowTicks[i].second;
        if (tally == 0 && i + 1 < rowTicks.length() && rowTicks[i + 1].first != rowTicks[i].first) {
            d->tableSelectionRows.append((rowTicks[i].first + rowTicks[i + 1].first) / 2);
            d->tableDividersGuessed = true;
        }
    }
    Q_ASSERT(tally == 0);
}

void PageView::mouseDoubleClickEvent(QMouseEvent *e)
{
    d->controlWheelAccumulatedDelta = 0;

    if (e->button() == Qt::LeftButton) {
        const QPoint eventPos = contentAreaPoint(e->pos());
        PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
        if (pageItem) {
            // find out normalized mouse coords inside current item
            double nX = pageItem->absToPageX(eventPos.x());
            double nY = pageItem->absToPageY(eventPos.y());

            if (d->mouseMode == Okular::Settings::EnumMouseMode::TextSelect) {
                textSelectionClear();

                Okular::RegularAreaRect *wordRect = pageItem->page()->wordAt(Okular::NormalizedPoint(nX, nY));
                if (wordRect) {
                    // TODO words with hyphens across pages
                    d->document->setPageTextSelection(pageItem->pageNumber(), wordRect, palette().color(QPalette::Active, QPalette::Highlight));
                    d->pagesWithTextSelection << pageItem->pageNumber();
                    if (d->document->isAllowed(Okular::AllowCopy)) {
                        const QString text = d->selectedText();
                        if (!text.isEmpty()) {
                            QClipboard *cb = QApplication::clipboard();
                            if (cb->supportsSelection())
                                cb->setText(text, QClipboard::Selection);
                        }
                    }
                    return;
                }
            }

            const QRect &itemRect = pageItem->uncroppedGeometry();
            Okular::Annotation *ann = nullptr;
            const Okular::ObjectRect *orect = pageItem->page()->objectRect(Okular::ObjectRect::OAnnotation, nX, nY, itemRect.width(), itemRect.height());
            if (orect)
                ann = ((Okular::AnnotationObjectRect *)orect)->annotation();
            if (ann && ann->subType() != Okular::Annotation::AWidget) {
                openAnnotationWindow(ann, pageItem->pageNumber());
            }
        }
    }
}

void PageView::wheelEvent(QWheelEvent *e)
{
    if (!d->document->isOpened()) {
        QAbstractScrollArea::wheelEvent(e);
        return;
    }

    int delta = e->angleDelta().y(), vScroll = verticalScrollBar()->value();
    e->accept();
    if ((e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) {
        d->controlWheelAccumulatedDelta += delta;
        if (d->controlWheelAccumulatedDelta <= -QWheelEvent::DefaultDeltasPerStep) {
            slotZoomOut();
            d->controlWheelAccumulatedDelta = 0;
        } else if (d->controlWheelAccumulatedDelta >= QWheelEvent::DefaultDeltasPerStep) {
            slotZoomIn();
            d->controlWheelAccumulatedDelta = 0;
        }
    } else {
        d->controlWheelAccumulatedDelta = 0;

        if (delta <= -QWheelEvent::DefaultDeltasPerStep && !getContinuousMode() && vScroll == verticalScrollBar()->maximum()) {
            // go to next page
            if ((int)d->document->currentPage() < d->items.count() - 1) {
                // more optimized than document->setNextPage and then move view to top
                Okular::DocumentViewport newViewport = d->document->viewport();
                newViewport.pageNumber += viewColumns();
                if (newViewport.pageNumber >= (int)d->items.count())
                    newViewport.pageNumber = d->items.count() - 1;
                newViewport.rePos.enabled = true;
                newViewport.rePos.normalizedY = 0.0;
                d->document->setViewport(newViewport);
                d->scroller->scrollTo(QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value()), 0); // sync scroller with scrollbar
            }
        } else if (delta >= QWheelEvent::DefaultDeltasPerStep && !getContinuousMode() && vScroll == verticalScrollBar()->minimum()) {
            // go to prev page
            if (d->document->currentPage() > 0) {
                // more optimized than document->setPrevPage and then move view to bottom
                Okular::DocumentViewport newViewport = d->document->viewport();
                newViewport.pageNumber -= viewColumns();
                if (newViewport.pageNumber < 0)
                    newViewport.pageNumber = 0;
                newViewport.rePos.enabled = true;
                newViewport.rePos.normalizedY = 1.0;
                d->document->setViewport(newViewport);
                d->scroller->scrollTo(QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value()), 0); // sync scroller with scrollbar
            }
        } else {
            // When the shift key is held down, scroll ten times faster
            int multiplier = e->modifiers() & Qt::ShiftModifier ? 10 : 1;

            if (delta != 0 && delta % QWheelEvent::DefaultDeltasPerStep == 0) {
                // number of scroll wheel steps Qt gives to us at the same time
                int count = abs(delta / QWheelEvent::DefaultDeltasPerStep) * multiplier;
                if (delta < 0) {
                    slotScrollDown(count);
                } else {
                    slotScrollUp(count);
                }
            } else {
                d->scroller->scrollTo(d->scroller->finalPosition() - e->angleDelta() * multiplier / 4.0, 0);
            }
        }
    }
}

bool PageView::viewportEvent(QEvent *e)
{
    if (e->type() == QEvent::ToolTip
        // Show tool tips only for those modes that change the cursor
        // to a hand when hovering over the link.
        && (d->mouseMode == Okular::Settings::EnumMouseMode::Browse || d->mouseMode == Okular::Settings::EnumMouseMode::RectSelect || d->mouseMode == Okular::Settings::EnumMouseMode::TextSelect ||
            d->mouseMode == Okular::Settings::EnumMouseMode::TrimSelect)) {
        QHelpEvent *he = static_cast<QHelpEvent *>(e);
        if (d->mouseAnnotation->isMouseOver()) {
            d->mouseAnnotation->routeTooltipEvent(he);
        } else {
            const QPoint eventPos = contentAreaPoint(he->pos());
            PageViewItem *pageItem = pickItemOnPoint(eventPos.x(), eventPos.y());
            const Okular::ObjectRect *rect = nullptr;
            const Okular::Action *link = nullptr;
            if (pageItem) {
                double nX = pageItem->absToPageX(eventPos.x());
                double nY = pageItem->absToPageY(eventPos.y());
                rect = pageItem->page()->objectRect(Okular::ObjectRect::Action, nX, nY, pageItem->uncroppedWidth(), pageItem->uncroppedHeight());
                if (rect)
                    link = static_cast<const Okular::Action *>(rect->object());
            }

            if (link) {
                QRect r = rect->boundingRect(pageItem->uncroppedWidth(), pageItem->uncroppedHeight());
                r.translate(pageItem->uncroppedGeometry().topLeft());
                r.translate(-contentAreaPosition());
                QString tip = link->actionTip();
                if (!tip.isEmpty())
                    QToolTip::showText(he->globalPos(), tip, viewport(), r);
            }
        }
        e->accept();
        return true;
    } else
        // do not stop the event
        return QAbstractScrollArea::viewportEvent(e);
}

void PageView::scrollContentsBy(int dx, int dy)
{
    const QRect r = viewport()->rect();
    viewport()->scroll(dx, dy, r);
    // HACK manually repaint the damaged regions, as it seems some updates are missed
    // thus leaving artifacts around
    QRegion rgn(r);
    rgn -= rgn & r.translated(dx, dy);

    for (const QRect &rect : rgn)
        viewport()->update(rect);

    updateCursor();
}
// END widget events

QList<Okular::RegularAreaRect *> PageView::textSelections(const QPoint start, const QPoint end, int &firstpage)
{
    firstpage = -1;
    QList<Okular::RegularAreaRect *> ret;
    QSet<int> affectedItemsSet;
    QRect selectionRect = QRect(start, end).normalized();
    for (const PageViewItem *item : qAsConst(d->items)) {
        if (item->isVisible() && selectionRect.intersects(item->croppedGeometry()))
            affectedItemsSet.insert(item->pageNumber());
    }
#ifdef PAGEVIEW_DEBUG
    qCDebug(OkularUiDebug) << ">>>> item selected by mouse:" << affectedItemsSet.count();
#endif

    if (!affectedItemsSet.isEmpty()) {
        // is the mouse drag line the ne-sw diagonal of the selection rect?
        bool direction_ne_sw = start == selectionRect.topRight() || start == selectionRect.bottomLeft();

        int tmpmin = d->document->pages();
        int tmpmax = 0;
        for (const int p : qAsConst(affectedItemsSet)) {
            if (p < tmpmin)
                tmpmin = p;
            if (p > tmpmax)
                tmpmax = p;
        }

        PageViewItem *a = pickItemOnPoint((int)(direction_ne_sw ? selectionRect.right() : selectionRect.left()), (int)selectionRect.top());
        int min = a && (a->pageNumber() != tmpmax) ? a->pageNumber() : tmpmin;
        PageViewItem *b = pickItemOnPoint((int)(direction_ne_sw ? selectionRect.left() : selectionRect.right()), (int)selectionRect.bottom());
        int max = b && (b->pageNumber() != tmpmin) ? b->pageNumber() : tmpmax;

        QList<int> affectedItemsIds;
        for (int i = min; i <= max; ++i)
            affectedItemsIds.append(i);
#ifdef PAGEVIEW_DEBUG
        qCDebug(OkularUiDebug) << ">>>> pages:" << affectedItemsIds;
#endif
        firstpage = affectedItemsIds.first();

        if (affectedItemsIds.count() == 1) {
            PageViewItem *item = d->items[affectedItemsIds.first()];
            selectionRect.translate(-item->uncroppedGeometry().topLeft());
            ret.append(textSelectionForItem(item, direction_ne_sw ? selectionRect.topRight() : selectionRect.topLeft(), direction_ne_sw ? selectionRect.bottomLeft() : selectionRect.bottomRight()));
        } else if (affectedItemsIds.count() > 1) {
            // first item
            PageViewItem *first = d->items[affectedItemsIds.first()];
            QRect geom = first->croppedGeometry().intersected(selectionRect).translated(-first->uncroppedGeometry().topLeft());
            ret.append(textSelectionForItem(first, selectionRect.bottom() > geom.height() ? (direction_ne_sw ? geom.topRight() : geom.topLeft()) : (direction_ne_sw ? geom.bottomRight() : geom.bottomLeft()), QPoint()));
            // last item
            PageViewItem *last = d->items[affectedItemsIds.last()];
            geom = last->croppedGeometry().intersected(selectionRect).translated(-last->uncroppedGeometry().topLeft());
            // the last item needs to appended at last...
            Okular::RegularAreaRect *lastArea =
                textSelectionForItem(last, QPoint(), selectionRect.bottom() > geom.height() ? (direction_ne_sw ? geom.bottomLeft() : geom.bottomRight()) : (direction_ne_sw ? geom.topLeft() : geom.topRight()));
            affectedItemsIds.removeFirst();
            affectedItemsIds.removeLast();
            // item between the two above
            for (const int page : qAsConst(affectedItemsIds)) {
                ret.append(textSelectionForItem(d->items[page]));
            }
            ret.append(lastArea);
        }
    }
    return ret;
}

void PageView::drawDocumentOnPainter(const QRect contentsRect, QPainter *p)
{
    QColor backColor;

    if (Okular::Settings::useCustomBackgroundColor())
        backColor = Okular::Settings::backgroundColor();
    else
        backColor = viewport()->palette().color(QPalette::Dark);

    // create a region from which we'll subtract painted rects
    QRegion remainingArea(contentsRect);

    // This loop draws the actual pages
    // iterate over all items painting the ones intersecting contentsRect
    for (const PageViewItem *item : qAsConst(d->items)) {
        // check if a piece of the page intersects the contents rect
        if (!item->isVisible() || !item->croppedGeometry().intersects(contentsRect))
            continue;

        // get item and item's outline geometries
        QRect itemGeometry = item->croppedGeometry();

        // move the painter to the top-left corner of the real page
        p->save();
        p->translate(itemGeometry.left(), itemGeometry.top());

        // draw the page using the PagePainter with all flags active
        if (contentsRect.intersects(itemGeometry)) {
            Okular::NormalizedPoint *viewPortPoint = nullptr;
            Okular::NormalizedPoint point(d->lastSourceLocationViewportNormalizedX, d->lastSourceLocationViewportNormalizedY);
            if (Okular::Settings::showSourceLocationsGraphically() && item->pageNumber() == d->lastSourceLocationViewportPageNumber) {
                viewPortPoint = &point;
            }
            QRect pixmapRect = contentsRect.intersected(itemGeometry);
            pixmapRect.translate(-item->croppedGeometry().topLeft());
            PagePainter::paintCroppedPageOnPainter(p, item->page(), this, pageflags, item->uncroppedWidth(), item->uncroppedHeight(), pixmapRect, item->crop(), viewPortPoint);
        }

        // remove painted area from 'remainingArea' and restore painter
        remainingArea -= itemGeometry;
        p->restore();
    }

    // fill the visible area around the page with the background color
    for (const QRect &backRect : remainingArea)
        p->fillRect(backRect, backColor);

    // take outline and shadow into account when testing whether a repaint is necessary
    auto dpr = devicePixelRatioF();
    QRect checkRect = contentsRect;
    checkRect.adjust(-3, -3, 1, 1);

    // Method to linearly interpolate between black (=(0,0,0), omitted) and the background color
    auto interpolateColor = [&backColor](double t) { return QColor(t * backColor.red(), t * backColor.green(), t * backColor.blue()); };

    // width of the shadow in device pixels
    static const int shadowWidth = 2 * dpr;

    // iterate over all items painting a black outline and a simple bottom/right gradient
    for (const PageViewItem *item : qAsConst(d->items)) {
        // check if a piece of the page intersects the contents rect
        if (!item->isVisible() || !item->croppedGeometry().intersects(checkRect))
            continue;

        // get item and item's outline geometries
        QRect itemGeometry = item->croppedGeometry();

        // move the painter to the top-left corner of the real page
        p->save();
        p->translate(itemGeometry.left(), itemGeometry.top());

        // draw the page outline (black border and bottom-right shadow)
        if (!itemGeometry.contains(contentsRect)) {
            int itemWidth = itemGeometry.width();
            int itemHeight = itemGeometry.height();
            // draw simple outline
            QPen pen(Qt::black);
            pen.setWidth(0);
            p->setPen(pen);

            QRectF outline(-1.0 / dpr, -1.0 / dpr, itemWidth + 1.0 / dpr, itemHeight + 1.0 / dpr);
            p->drawRect(outline);

            // draw bottom/right gradient
            for (int i = 1; i <= shadowWidth; i++) {
                pen.setColor(interpolateColor(double(i) / (shadowWidth + 1)));
                p->setPen(pen);
                QPointF left((i - 1) / dpr, itemHeight + i / dpr);
                QPointF up(itemWidth + i / dpr, (i - 1) / dpr);
                QPointF corner(itemWidth + i / dpr, itemHeight + i / dpr);
                p->drawLine(left, corner);
                p->drawLine(up, corner);
            }
        }

        p->restore();
    }
}

void PageView::updateItemSize(PageViewItem *item, int colWidth, int rowHeight)
{
    const Okular::Page *okularPage = item->page();
    double width = okularPage->width(), height = okularPage->height(), zoom = d->zoomFactor;
    Okular::NormalizedRect crop(0., 0., 1., 1.);

    // Handle cropping, due to either "Trim Margin" or "Trim to Selection" cases
    if ((Okular::Settings::trimMargins() && okularPage->isBoundingBoxKnown() && !okularPage->boundingBox().isNull()) || (d->aTrimToSelection && d->aTrimToSelection->isChecked() && !d->trimBoundingBox.isNull())) {
        crop = Okular::Settings::trimMargins() ? okularPage->boundingBox() : d->trimBoundingBox;

        // Rotate the bounding box
        for (int i = okularPage->rotation(); i > 0; --i) {
            Okular::NormalizedRect rot = crop;
            crop.left = 1 - rot.bottom;
            crop.top = rot.left;
            crop.right = 1 - rot.top;
            crop.bottom = rot.right;
        }

        // Expand the crop slightly beyond the bounding box (for Trim Margins only)
        if (Okular::Settings::trimMargins()) {
            static const double cropExpandRatio = 0.04;
            const double cropExpand = cropExpandRatio * ((crop.right - crop.left) + (crop.bottom - crop.top)) / 2;
            crop = Okular::NormalizedRect(crop.left - cropExpand, crop.top - cropExpand, crop.right + cropExpand, crop.bottom + cropExpand) & Okular::NormalizedRect(0, 0, 1, 1);
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
        if ((crop.right - crop.left) < minCropRatio) {
            const double newLeft = (crop.left + crop.right) / 2 - minCropRatio / 2;
            crop.left = qMax(0.0, qMin(1.0 - minCropRatio, newLeft));
            crop.right = crop.left + minCropRatio;
        }
        if ((crop.bottom - crop.top) < minCropRatio) {
            const double newTop = (crop.top + crop.bottom) / 2 - minCropRatio / 2;
            crop.top = qMax(0.0, qMin(1.0 - minCropRatio, newTop));
            crop.bottom = crop.top + minCropRatio;
        }

        width *= (crop.right - crop.left);
        height *= (crop.bottom - crop.top);
#ifdef PAGEVIEW_DEBUG
        qCDebug(OkularUiDebug) << "Cropped page" << okularPage->number() << "to" << crop << "width" << width << "height" << height << "by bbox" << okularPage->boundingBox();
#endif
    }

    if (d->zoomMode == ZoomFixed) {
        width *= zoom;
        height *= zoom;
        item->setWHZC((int)width, (int)height, d->zoomFactor, crop);
    } else if (d->zoomMode == ZoomFitWidth) {
        height = (height / width) * colWidth;
        zoom = (double)colWidth / width;
        item->setWHZC(colWidth, (int)height, zoom, crop);
        if ((uint)item->pageNumber() == d->document->currentPage())
            d->zoomFactor = zoom;
    } else if (d->zoomMode == ZoomFitPage) {
        const double scaleW = (double)colWidth / (double)width;
        const double scaleH = (double)rowHeight / (double)height;
        zoom = qMin(scaleW, scaleH);
        item->setWHZC((int)(zoom * width), (int)(zoom * height), zoom, crop);
        if ((uint)item->pageNumber() == d->document->currentPage())
            d->zoomFactor = zoom;
    } else if (d->zoomMode == ZoomFitAuto) {
        const double aspectRatioRelation = 1.25; // relation between aspect ratios for "auto fit"
        const double uiAspect = (double)rowHeight / (double)colWidth;
        const double pageAspect = (double)height / (double)width;
        const double rel = uiAspect / pageAspect;

        if (!getContinuousMode() && rel > aspectRatioRelation) {
            // UI space is relatively much higher than the page
            zoom = (double)rowHeight / (double)height;
        } else if (rel < 1.0 / aspectRatioRelation) {
            // UI space is relatively much wider than the page in relation
            zoom = (double)colWidth / (double)width;
        } else {
            // aspect ratios of page and UI space are very similar
            const double scaleW = (double)colWidth / (double)width;
            const double scaleH = (double)rowHeight / (double)height;
            zoom = qMin(scaleW, scaleH);
        }
        item->setWHZC((int)(zoom * width), (int)(zoom * height), zoom, crop);
        if ((uint)item->pageNumber() == d->document->currentPage())
            d->zoomFactor = zoom;
    }
#ifndef NDEBUG
    else
        qCDebug(OkularUiDebug) << "calling updateItemSize with unrecognized d->zoomMode!";
#endif
}

PageViewItem *PageView::pickItemOnPoint(int x, int y)
{
    PageViewItem *item = nullptr;
    for (PageViewItem *i : qAsConst(d->visibleItems)) {
        const QRect &r = i->croppedGeometry();
        if (x < r.right() && x > r.left() && y < r.bottom()) {
            if (y > r.top())
                item = i;
            break;
        }
    }
    return item;
}

void PageView::textSelectionClear()
{
    // something to clear
    if (!d->pagesWithTextSelection.isEmpty()) {
        for (const int page : qAsConst(d->pagesWithTextSelection))
            d->document->setPageTextSelection(page, nullptr, QColor());
        d->pagesWithTextSelection.clear();
    }
}

void PageView::selectionStart(const QPoint pos, const QColor &color, bool /*aboveAll*/)
{
    selectionClear();
    d->mouseSelecting = true;
    d->mouseSelectionRect.setRect(pos.x(), pos.y(), 1, 1);
    d->mouseSelectionColor = color;
    // ensures page doesn't scroll
    if (d->autoScrollTimer) {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }
}

void PageView::scrollPosIntoView(const QPoint pos)
{
    // this number slows the speed of the page by its value, chosen not to be too fast or too slow, the actual speed is determined from the mouse position, not critical
    const int damping = 6;

    if (pos.x() < horizontalScrollBar()->value())
        d->dragScrollVector.setX((pos.x() - horizontalScrollBar()->value()) / damping);
    else if (horizontalScrollBar()->value() + viewport()->width() < pos.x())
        d->dragScrollVector.setX((pos.x() - horizontalScrollBar()->value() - viewport()->width()) / damping);
    else
        d->dragScrollVector.setX(0);

    if (pos.y() < verticalScrollBar()->value())
        d->dragScrollVector.setY((pos.y() - verticalScrollBar()->value()) / damping);
    else if (verticalScrollBar()->value() + viewport()->height() < pos.y())
        d->dragScrollVector.setY((pos.y() - verticalScrollBar()->value() - viewport()->height()) / damping);
    else
        d->dragScrollVector.setY(0);

    if (d->dragScrollVector != QPoint(0, 0)) {
        if (!d->dragScrollTimer.isActive())
            d->dragScrollTimer.start(1000 / 60); // 60 fps
    } else
        d->dragScrollTimer.stop();
}

QPoint PageView::viewportToContentArea(const Okular::DocumentViewport &vp) const
{
    Q_ASSERT(vp.pageNumber >= 0);

    const QRect &r = d->items[vp.pageNumber]->croppedGeometry();
    QPoint c {r.left(), r.top()};

    if (vp.rePos.enabled) {
        // Convert the coordinates of vp to normalized coordinates on the cropped page.
        // This is a no-op if the page isn't cropped.
        const Okular::NormalizedRect &crop = d->items[vp.pageNumber]->crop();
        const double normalized_on_crop_x = (vp.rePos.normalizedX - crop.left) / (crop.right - crop.left);
        const double normalized_on_crop_y = (vp.rePos.normalizedY - crop.top) / (crop.bottom - crop.top);

        if (vp.rePos.pos == Okular::DocumentViewport::Center) {
            c.rx() += qRound(normClamp(normalized_on_crop_x, 0.5) * (double)r.width());
            c.ry() += qRound(normClamp(normalized_on_crop_y, 0.0) * (double)r.height());
        } else {
            // TopLeft
            c.rx() += qRound(normClamp(normalized_on_crop_x, 0.0) * (double)r.width() + viewport()->width() / 2.0);
            c.ry() += qRound(normClamp(normalized_on_crop_y, 0.0) * (double)r.height() + viewport()->height() / 2.0);
        }
    } else {
        // exact repositioning disabled, align page top margin with viewport top border by default
        c.rx() += r.width() / 2;
        c.ry() += viewport()->height() / 2 - 10;
    }
    return c;
}

void PageView::updateSelection(const QPoint pos)
{
    if (d->mouseSelecting) {
        scrollPosIntoView(pos);
        // update the selection rect
        QRect updateRect = d->mouseSelectionRect;
        d->mouseSelectionRect.setBottomLeft(pos);
        updateRect |= d->mouseSelectionRect;
        updateRect.translate(-contentAreaPosition());
        viewport()->update(updateRect.adjusted(-1, -2, 2, 1));
    } else if (d->mouseTextSelecting) {
        scrollPosIntoView(pos);
        int first = -1;
        const QList<Okular::RegularAreaRect *> selections = textSelections(pos, d->mouseSelectPos, first);
        QSet<int> pagesWithSelectionSet;
        for (int i = 0; i < selections.count(); ++i)
            pagesWithSelectionSet.insert(i + first);

        const QSet<int> noMoreSelectedPages = d->pagesWithTextSelection - pagesWithSelectionSet;
        // clear the selection from pages not selected anymore
        for (int p : noMoreSelectedPages) {
            d->document->setPageTextSelection(p, nullptr, QColor());
        }
        // set the new selection for the selected pages
        for (int p : qAsConst(pagesWithSelectionSet)) {
            d->document->setPageTextSelection(p, selections[p - first], palette().color(QPalette::Active, QPalette::Highlight));
        }
        d->pagesWithTextSelection = pagesWithSelectionSet;
    }
}

static Okular::NormalizedPoint rotateInNormRect(const QPoint rotated, const QRect rect, Okular::Rotation rotation)
{
    Okular::NormalizedPoint ret;

    switch (rotation) {
    case Okular::Rotation0:
        ret = Okular::NormalizedPoint(rotated.x(), rotated.y(), rect.width(), rect.height());
        break;
    case Okular::Rotation90:
        ret = Okular::NormalizedPoint(rotated.y(), rect.width() - rotated.x(), rect.height(), rect.width());
        break;
    case Okular::Rotation180:
        ret = Okular::NormalizedPoint(rect.width() - rotated.x(), rect.height() - rotated.y(), rect.width(), rect.height());
        break;
    case Okular::Rotation270:
        ret = Okular::NormalizedPoint(rect.height() - rotated.y(), rotated.x(), rect.height(), rect.width());
        break;
    }

    return ret;
}

Okular::RegularAreaRect *PageView::textSelectionForItem(const PageViewItem *item, const QPoint startPoint, const QPoint endPoint)
{
    const QRect &geometry = item->uncroppedGeometry();
    Okular::NormalizedPoint startCursor(0.0, 0.0);
    if (!startPoint.isNull()) {
        startCursor = rotateInNormRect(startPoint, geometry, item->page()->rotation());
    }
    Okular::NormalizedPoint endCursor(1.0, 1.0);
    if (!endPoint.isNull()) {
        endCursor = rotateInNormRect(endPoint, geometry, item->page()->rotation());
    }
    Okular::TextSelection mouseTextSelectionInfo(startCursor, endCursor);

    const Okular::Page *okularPage = item->page();

    if (!okularPage->hasTextPage())
        d->document->requestTextPage(okularPage->number());

    Okular::RegularAreaRect *selectionArea = okularPage->textArea(&mouseTextSelectionInfo);
#ifdef PAGEVIEW_DEBUG
    qCDebug(OkularUiDebug).nospace() << "text areas (" << okularPage->number() << "): " << (selectionArea ? QString::number(selectionArea->count()) : "(none)");
#endif
    return selectionArea;
}

void PageView::selectionClear(const ClearMode mode)
{
    QRect updatedRect = d->mouseSelectionRect.normalized().adjusted(-2, -2, 2, 2);
    d->mouseSelecting = false;
    d->mouseSelectionRect.setCoords(0, 0, 0, 0);
    d->tableSelectionCols.clear();
    d->tableSelectionRows.clear();
    d->tableDividersGuessed = false;
    for (const TableSelectionPart &tsp : qAsConst(d->tableSelectionParts)) {
        QRect selectionPartRect = tsp.rectInItem.geometry(tsp.item->uncroppedWidth(), tsp.item->uncroppedHeight());
        selectionPartRect.translate(tsp.item->uncroppedGeometry().topLeft());
        // should check whether this is on-screen here?
        updatedRect = updatedRect.united(selectionPartRect);
    }
    if (mode != ClearOnlyDividers) {
        d->tableSelectionParts.clear();
    }
    d->tableSelectionParts.clear();
    updatedRect.translate(-contentAreaPosition());
    viewport()->update(updatedRect);
}

// const to be used for both zoomFactorFitMode function and slotRelayoutPages.
static const int kcolWidthMargin = 6;
static const int krowHeightMargin = 12;

double PageView::zoomFactorFitMode(ZoomMode mode)
{
    const int pageCount = d->items.count();
    if (pageCount == 0)
        return 0;
    const bool facingCentered = Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::FacingFirstCentered || (Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::Facing && pageCount == 1);
    const bool overrideCentering = facingCentered && pageCount < 3;
    const int nCols = overrideCentering ? 1 : viewColumns();
    const int colWidth = viewport()->width() / nCols - kcolWidthMargin;
    const double rowHeight = viewport()->height() - krowHeightMargin;
    const PageViewItem *currentItem = d->items[qMax(0, (int)d->document->currentPage())];
    // prevent segmentation fault when opening a new document;
    if (!currentItem)
        return 0;

    // We need the real width/height of the cropped page.
    const Okular::Page *okularPage = currentItem->page();
    const double width = okularPage->width() * currentItem->crop().width();
    const double height = okularPage->height() * currentItem->crop().height();

    if (mode == ZoomFitWidth)
        return (double)colWidth / width;
    if (mode == ZoomFitPage) {
        const double scaleW = (double)colWidth / (double)width;
        const double scaleH = (double)rowHeight / (double)height;
        return qMin(scaleW, scaleH);
    }
    return 0;
}

void PageView::updateZoom(ZoomMode newZoomMode)
{
    if (newZoomMode == ZoomFixed) {
        if (d->aZoom->currentItem() == 0)
            newZoomMode = ZoomFitWidth;
        else if (d->aZoom->currentItem() == 1)
            newZoomMode = ZoomFitPage;
        else if (d->aZoom->currentItem() == 2)
            newZoomMode = ZoomFitAuto;
    }

    float newFactor = d->zoomFactor;
    QAction *checkedZoomAction = nullptr;
    switch (newZoomMode) {
    case ZoomFixed: { // ZoomFixed case
        QString z = d->aZoom->currentText();
        // kdelibs4 sometimes adds accelerators to actions' text directly :(
        z.remove(QLatin1Char('&'));
        z.remove(QLatin1Char('%'));
        newFactor = QLocale().toDouble(z) / 100.0;
    } break;
    case ZoomIn:
    case ZoomOut: {
        const float zoomFactorFitWidth = zoomFactorFitMode(ZoomFitWidth);
        const float zoomFactorFitPage = zoomFactorFitMode(ZoomFitPage);

        QVector<float> zoomValue(kZoomValues.size());

        std::copy(kZoomValues.begin(), kZoomValues.end(), zoomValue.begin());
        zoomValue.append(zoomFactorFitWidth);
        zoomValue.append(zoomFactorFitPage);
        std::sort(zoomValue.begin(), zoomValue.end());

        QVector<float>::iterator i;
        if (newZoomMode == ZoomOut) {
            if (newFactor <= zoomValue.first())
                return;
            i = std::lower_bound(zoomValue.begin(), zoomValue.end(), newFactor) - 1;
        } else {
            if (newFactor >= zoomValue.last())
                return;
            i = std::upper_bound(zoomValue.begin(), zoomValue.end(), newFactor);
        }
        const float tmpFactor = *i;
        if (tmpFactor == zoomFactorFitWidth) {
            newZoomMode = ZoomFitWidth;
            checkedZoomAction = d->aZoomFitWidth;
        } else if (tmpFactor == zoomFactorFitPage) {
            newZoomMode = ZoomFitPage;
            checkedZoomAction = d->aZoomFitPage;
        } else {
            newFactor = tmpFactor;
            newZoomMode = ZoomFixed;
        }
    } break;
    case ZoomActual:
        newZoomMode = ZoomFixed;
        newFactor = 1.0;
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
    const float upperZoomLimit = d->document->supportsTiles() ? 100.0 : 4.0;
    if (newFactor > upperZoomLimit)
        newFactor = upperZoomLimit;
    if (newFactor < 0.1)
        newFactor = 0.1;

    if (newZoomMode != d->zoomMode || (newZoomMode == ZoomFixed && newFactor != d->zoomFactor)) {
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
        if (d->aZoomFitWidth) {
            d->aZoomFitWidth->setChecked(checkedZoomAction == d->aZoomFitWidth);
            d->aZoomFitPage->setChecked(checkedZoomAction == d->aZoomFitPage);
            d->aZoomAutoFit->setChecked(checkedZoomAction == d->aZoomAutoFit);
        }
    } else if (newZoomMode == ZoomFixed && newFactor == d->zoomFactor)
        updateZoomText();

    d->aZoomIn->setEnabled(d->zoomFactor < upperZoomLimit - 0.001);
    d->aZoomOut->setEnabled(d->zoomFactor > 0.101);
    d->aZoomActual->setEnabled(d->zoomFactor != 1.0);
}

void PageView::updateZoomText()
{
    // use current page zoom as zoomFactor if in ZoomFit/* mode
    if (d->zoomMode != ZoomFixed && d->items.count() > 0)
        d->zoomFactor = d->items[qMax(0, (int)d->document->currentPage())]->zoomFactor();
    float newFactor = d->zoomFactor;
    d->aZoom->removeAllActions();

    // add items that describe fit actions
    QStringList translated;
    translated << i18n("Fit Width") << i18n("Fit Page") << i18n("Auto Fit");

    // add percent items
    int idx = 0, selIdx = 3;
    bool inserted = false; // use: "d->zoomMode != ZoomFixed" to hide Fit/* zoom ratio
    int zoomValueCount = 11;
    if (d->document->supportsTiles())
        zoomValueCount = kZoomValues.size();
    while (idx < zoomValueCount || !inserted) {
        float value = idx < zoomValueCount ? kZoomValues[idx] : newFactor;
        if (!inserted && newFactor < (value - 0.0001))
            value = newFactor;
        else
            idx++;
        if (value > (newFactor - 0.0001) && value < (newFactor + 0.0001))
            inserted = true;
        if (!inserted)
            selIdx++;
        // we do not need to display 2-digit precision
        QString localValue(QLocale().toString(value * 100.0, 'f', 1));
        localValue.remove(QLocale().decimalPoint() + QLatin1Char('0'));
        // remove a trailing zero in numbers like 66.70
        if (localValue.right(1) == QLatin1String("0") && localValue.indexOf(QLocale().decimalPoint()) > -1)
            localValue.chop(1);
        translated << QStringLiteral("%1%").arg(localValue);
    }
    d->aZoom->setItems(translated);

    // select current item in list
    if (d->zoomMode == ZoomFitWidth)
        selIdx = 0;
    else if (d->zoomMode == ZoomFitPage)
        selIdx = 1;
    else if (d->zoomMode == ZoomFitAuto)
        selIdx = 2;
    // we have to temporarily enable the actions as otherwise we can't set a new current item
    d->aZoom->setEnabled(true);
    d->aZoom->selectableActionGroup()->setEnabled(true);
    d->aZoom->setCurrentItem(selIdx);
    d->aZoom->setEnabled(d->items.size() > 0);
    d->aZoom->selectableActionGroup()->setEnabled(d->items.size() > 0);
}

void PageView::updateViewMode(const int nr)
{
    const QList<QAction *> actions = d->viewModeActionGroup->actions();
    for (QAction *action : actions) {
        QVariant mode_id = action->data();
        if (mode_id.toInt() == nr) {
            action->trigger();
        }
    }
}

void PageView::updateCursor()
{
    const QPoint p = contentAreaPosition() + viewport()->mapFromGlobal(QCursor::pos());
    updateCursor(p);
}

void PageView::updateCursor(const QPoint p)
{
    // reset mouse over link it will be re-set if that still valid
    d->mouseOverLinkObject = nullptr;

    // detect the underlaying page (if present)
    PageViewItem *pageItem = pickItemOnPoint(p.x(), p.y());
    QScroller::State scrollerState = d->scroller->state();

    if (d->annotator && d->annotator->active()) {
        if (pageItem || d->annotator->annotating())
            setCursor(d->annotator->cursor());
        else
            setCursor(Qt::ForbiddenCursor);
    } else if (scrollerState == QScroller::Pressed || scrollerState == QScroller::Dragging) {
        setCursor(Qt::ClosedHandCursor);
    } else if (pageItem) {
        double nX = pageItem->absToPageX(p.x());
        double nY = pageItem->absToPageY(p.y());
        Qt::CursorShape cursorShapeFallback;

        // if over a ObjectRect (of type Link) change cursor to hand
        switch (d->mouseMode) {
        case Okular::Settings::EnumMouseMode::TextSelect:
            if (d->mouseTextSelecting) {
                setCursor(Qt::IBeamCursor);
                return;
            }
            cursorShapeFallback = Qt::IBeamCursor;
            break;
        case Okular::Settings::EnumMouseMode::Magnifier:
            setCursor(Qt::CrossCursor);
            return;
        case Okular::Settings::EnumMouseMode::RectSelect:
        case Okular::Settings::EnumMouseMode::TrimSelect:
            if (d->mouseSelecting) {
                setCursor(Qt::CrossCursor);
                return;
            }
            cursorShapeFallback = Qt::CrossCursor;
            break;
        case Okular::Settings::EnumMouseMode::Browse:
            d->mouseOnRect = false;
            if (d->mouseAnnotation->isMouseOver()) {
                d->mouseOnRect = true;
                setCursor(d->mouseAnnotation->cursor());
                return;
            } else {
                cursorShapeFallback = Qt::OpenHandCursor;
            }
            break;
        default:
            setCursor(Qt::ArrowCursor);
            return;
        }

        const Okular::ObjectRect *linkobj = pageItem->page()->objectRect(Okular::ObjectRect::Action, nX, nY, pageItem->uncroppedWidth(), pageItem->uncroppedHeight());
        if (linkobj) {
            d->mouseOverLinkObject = linkobj;
            d->mouseOnRect = true;
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(cursorShapeFallback);
        }
    } else {
        // if there's no page over the cursor and we were showing the pointingHandCursor
        // go back to the normal one
        d->mouseOnRect = false;
        setCursor(Qt::ArrowCursor);
    }
}

void PageView::reloadForms()
{
    if (d->m_formsVisible) {
        for (PageViewItem *item : qAsConst(d->visibleItems)) {
            item->reloadFormWidgetsState();
        }
    }
}

void PageView::moveMagnifier(const QPoint p) // non scaled point
{
    const int w = d->magnifierView->width() * 0.5;
    const int h = d->magnifierView->height() * 0.5;

    int x = p.x() - w;
    int y = p.y() - h;

    const int max_x = viewport()->width();
    const int max_y = viewport()->height();

    QPoint scroll(0, 0);

    if (x < 0) {
        if (horizontalScrollBar()->value() > 0)
            scroll.setX(x - w);
        x = 0;
    }

    if (y < 0) {
        if (verticalScrollBar()->value() > 0)
            scroll.setY(y - h);
        y = 0;
    }

    if (p.x() + w > max_x) {
        if (horizontalScrollBar()->value() < horizontalScrollBar()->maximum())
            scroll.setX(p.x() + 2 * w - max_x);
        x = max_x - d->magnifierView->width() - 1;
    }

    if (p.y() + h > max_y) {
        if (verticalScrollBar()->value() < verticalScrollBar()->maximum())
            scroll.setY(p.y() + 2 * h - max_y);
        y = max_y - d->magnifierView->height() - 1;
    }

    if (!scroll.isNull())
        scrollPosIntoView(contentAreaPoint(p + scroll));

    d->magnifierView->move(x, y);
}

void PageView::updateMagnifier(const QPoint p) // scaled point
{
    /* translate mouse coordinates to page coordinates and inform the magnifier of the situation */
    PageViewItem *item = pickItemOnPoint(p.x(), p.y());
    if (item) {
        Okular::NormalizedPoint np(item->absToPageX(p.x()), item->absToPageY(p.y()));
        d->magnifierView->updateView(np, item->page());
    }
}

int PageView::viewColumns() const
{
    int vm = Okular::Settings::viewMode();
    if (vm == Okular::Settings::EnumViewMode::Single)
        return 1;
    else if (vm == Okular::Settings::EnumViewMode::Facing || vm == Okular::Settings::EnumViewMode::FacingFirstCentered)
        return 2;
    else if (vm == Okular::Settings::EnumViewMode::Summary && d->document->pages() < Okular::Settings::viewColumns())
        return d->document->pages();
    else
        return Okular::Settings::viewColumns();
}

void PageView::center(int cx, int cy, bool smoothMove)
{
    scrollTo(cx - viewport()->width() / 2, cy - viewport()->height() / 2, smoothMove);
}

void PageView::scrollTo(int x, int y, bool smoothMove)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    // Workaround for QTBUG-88288, (KDE bug 425188): To avoid a crash in QScroller,
    // we need to make sure the target widget intersects a physical screen.
    // QScroller queries QDesktopWidget::screenNumber().

    // If we are not on a physical screen, we try to make our widget big enough.
    // The geometry will be restored to a sensible value once the Part is shown.

    // It should be enough to add this workaround ony in PageView::scrollTo(),
    // because we don’t expect other QScroller::scrollTo() calls before PageView is shown.
    if (QApplication::desktop()->screenNumber(this) < 0) {
        setGeometry(QRect(-1000, -1000, 5000, 5000).united(QApplication::desktop()->availableGeometry()));
    }
#endif

    bool prevState = d->blockPixmapsRequest;

    int newValue = -1;
    if (x != horizontalScrollBar()->value() || y != verticalScrollBar()->value())
        newValue = 1; // Pretend this call is the result of a scrollbar event

    d->blockPixmapsRequest = true;

    if (smoothMove)
        d->scroller->scrollTo(QPoint(x, y), d->currentLongScrollDuration);
    else
        d->scroller->scrollTo(QPoint(x, y), 0);

    d->blockPixmapsRequest = prevState;

    slotRequestVisiblePixmaps(newValue);
}

void PageView::toggleFormWidgets(bool on)
{
    bool somehadfocus = false;
    for (PageViewItem *item : qAsConst(d->items)) {
        const bool hadfocus = item->setFormWidgetsVisible(on);
        somehadfocus = somehadfocus || hadfocus;
    }
    if (somehadfocus)
        setFocus();
    d->m_formsVisible = on;
}

void PageView::resizeContentArea(const QSize newSize)
{
    const QSize vs = viewport()->size();
    int hRange = newSize.width() - vs.width();
    int vRange = newSize.height() - vs.height();
    if (horizontalScrollBar()->isVisible() && hRange == verticalScrollBar()->width() && verticalScrollBar()->isVisible() && vRange == horizontalScrollBar()->height() && Okular::Settings::showScrollBars()) {
        hRange = 0;
        vRange = 0;
    }
    horizontalScrollBar()->setRange(0, hRange);
    verticalScrollBar()->setRange(0, vRange);
    updatePageStep();
}

void PageView::updatePageStep()
{
    const QSize vs = viewport()->size();
    horizontalScrollBar()->setPageStep(vs.width());
    verticalScrollBar()->setPageStep(vs.height() * (100 - Okular::Settings::scrollOverlap()) / 100);
}

void PageView::addWebShortcutsMenu(QMenu *menu, const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    QString searchText = text;
    searchText = searchText.replace(QLatin1Char('\n'), QLatin1Char(' ')).replace(QLatin1Char('\r'), QLatin1Char(' ')).simplified();

    if (searchText.isEmpty()) {
        return;
    }

    KUriFilterData filterData(searchText);

    filterData.setSearchFilteringOptions(KUriFilterData::RetrievePreferredSearchProvidersOnly);

    if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::NormalTextFilter)) {
        const QStringList searchProviders = filterData.preferredSearchProviders();

        if (!searchProviders.isEmpty()) {
            QMenu *webShortcutsMenu = new QMenu(menu);
            webShortcutsMenu->setIcon(QIcon::fromTheme(QStringLiteral("preferences-web-browser-shortcuts")));

            const QString squeezedText = KStringHandler::rsqueeze(searchText, searchTextPreviewLength);
            webShortcutsMenu->setTitle(i18n("Search for '%1' with", squeezedText));

            QAction *action = nullptr;

            for (const QString &searchProvider : searchProviders) {
                action = new QAction(searchProvider, webShortcutsMenu);
                action->setIcon(QIcon::fromTheme(filterData.iconNameForPreferredSearchProvider(searchProvider)));
                action->setData(filterData.queryForPreferredSearchProvider(searchProvider));
                connect(action, &QAction::triggered, this, &PageView::slotHandleWebShortcutAction);
                webShortcutsMenu->addAction(action);
            }

            webShortcutsMenu->addSeparator();

            action = new QAction(i18n("Configure Web Shortcuts..."), webShortcutsMenu);
            action->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
            connect(action, &QAction::triggered, this, &PageView::slotConfigureWebShortcuts);
            webShortcutsMenu->addAction(action);

            menu->addMenu(webShortcutsMenu);
        }
    }
}

QMenu *PageView::createProcessLinkMenu(PageViewItem *item, const QPoint eventPos)
{
    // check if the right-click was over a link
    const double nX = item->absToPageX(eventPos.x());
    const double nY = item->absToPageY(eventPos.y());
    const Okular::ObjectRect *rect = item->page()->objectRect(Okular::ObjectRect::Action, nX, nY, item->uncroppedWidth(), item->uncroppedHeight());
    if (rect) {
        const Okular::Action *link = static_cast<const Okular::Action *>(rect->object());

        if (!link)
            return nullptr;

        QMenu *menu = new QMenu(this);

        // creating the menu and its actions
        QAction *processLink = menu->addAction(i18n("Follow This Link"));
        processLink->setObjectName(QStringLiteral("ProcessLinkAction"));
        if (link->actionType() == Okular::Action::Sound) {
            processLink->setText(i18n("Play this Sound"));
            if (Okular::AudioPlayer::instance()->state() == Okular::AudioPlayer::PlayingState) {
                QAction *actStopSound = menu->addAction(i18n("Stop Sound"));
                connect(actStopSound, &QAction::triggered, []() { Okular::AudioPlayer::instance()->stopPlaybacks(); });
            }
        }

        if (dynamic_cast<const Okular::BrowseAction *>(link)) {
            QAction *actCopyLinkLocation = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Link Address"));
            actCopyLinkLocation->setObjectName(QStringLiteral("CopyLinkLocationAction"));
            connect(actCopyLinkLocation, &QAction::triggered, menu, [link]() {
                const Okular::BrowseAction *browseLink = static_cast<const Okular::BrowseAction *>(link);
                QClipboard *cb = QApplication::clipboard();
                cb->setText(browseLink->url().toDisplayString(), QClipboard::Clipboard);
                if (cb->supportsSelection())
                    cb->setText(browseLink->url().toDisplayString(), QClipboard::Selection);
            });
        }

        connect(processLink, &QAction::triggered, this, [this, link]() { d->document->processAction(link); });
        return menu;
    }
    return nullptr;
}

void PageView::addSearchWithinDocumentAction(QMenu *menu, const QString &searchText)
{
    const QString squeezedText = KStringHandler::rsqueeze(searchText, searchTextPreviewLength);
    QAction *action = new QAction(i18n("Search for '%1' in this document", squeezedText), menu);
    action->setIcon(QIcon::fromTheme(QStringLiteral("document-preview")));
    connect(action, &QAction::triggered, this, [this, searchText] { Q_EMIT triggerSearch(searchText); });
    menu->addAction(action);
}

void PageView::updateSmoothScrollAnimationSpeed()
{
    // If it's turned off in Okular's own settings, don't bother to look at the
    // global settings
    if (!Okular::Settings::smoothScrolling()) {
        d->currentShortScrollDuration = 0;
        d->currentLongScrollDuration = 0;
        return;
    }

    // If we are using smooth scrolling, scale the speed of the animated
    // transitions according to the global animation speed setting
    KConfigGroup kdeglobalsConfig = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("KDE"));
    const qreal globalAnimationScale = qMax(0.0, kdeglobalsConfig.readEntry("AnimationDurationFactor", 1.0));
    d->currentShortScrollDuration = d->baseShortScrollDuration * globalAnimationScale;
    d->currentLongScrollDuration = d->baseLongScrollDuration * globalAnimationScale;
}

bool PageView::getContinuousMode() const
{
    return d->aViewContinuous ? d->aViewContinuous->isChecked() : Okular::Settings::viewContinuous();
}

// BEGIN private SLOTS
void PageView::slotRelayoutPages()
// called by: notifySetup, viewportResizeEvent, slotViewMode, slotContinuousToggled, updateZoom
{
    // set an empty container if we have no pages
    const int pageCount = d->items.count();
    if (pageCount < 1) {
        return;
    }

    int viewportWidth = viewport()->width(), viewportHeight = viewport()->height(), fullWidth = 0, fullHeight = 0;

    // handle the 'center first page in row' stuff
    const bool facing = Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::Facing && pageCount > 1;
    const bool facingCentered = Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::FacingFirstCentered || (Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::Facing && pageCount == 1);
    const bool overrideCentering = facingCentered && pageCount < 3;
    const bool centerFirstPage = facingCentered && !overrideCentering;
    const bool facingPages = facing || centerFirstPage;
    const bool centerLastPage = centerFirstPage && pageCount % 2 == 0;
    const bool continuousView = getContinuousMode();
    const int nCols = overrideCentering ? 1 : viewColumns();
    const bool singlePageViewMode = Okular::Settings::viewMode() == Okular::Settings::EnumViewMode::Single;

    if (d->aFitWindowToPage)
        d->aFitWindowToPage->setEnabled(!continuousView && singlePageViewMode);

    // set all items geometry and resize contents. handle 'continuous' and 'single' modes separately

    PageViewItem *currentItem = d->items[qMax(0, (int)d->document->currentPage())];

    // Here we find out column's width and row's height to compute a table
    // so we can place widgets 'centered in virtual cells'.
    const int nRows = (int)ceil((float)(centerFirstPage ? (pageCount + nCols - 1) : pageCount) / (float)nCols);

    int *colWidth = new int[nCols], *rowHeight = new int[nRows], cIdx = 0, rIdx = 0;
    for (int i = 0; i < nCols; i++)
        colWidth[i] = viewportWidth / nCols;
    for (int i = 0; i < nRows; i++)
        rowHeight[i] = 0;
    // handle the 'centering on first row' stuff
    if (centerFirstPage)
        cIdx += nCols - 1;

    // 1) find the maximum columns width and rows height for a grid in
    // which each page must well-fit inside a cell
    for (PageViewItem *item : qAsConst(d->items)) {
        // update internal page size (leaving a little margin in case of Fit* modes)
        updateItemSize(item, colWidth[cIdx] - kcolWidthMargin, viewportHeight - krowHeightMargin);
        // find row's maximum height and column's max width
        if (item->croppedWidth() + kcolWidthMargin > colWidth[cIdx])
            colWidth[cIdx] = item->croppedWidth() + kcolWidthMargin;
        if (item->croppedHeight() + krowHeightMargin > rowHeight[rIdx])
            rowHeight[rIdx] = item->croppedHeight() + krowHeightMargin;
        // handle the 'centering on first row' stuff
        // update col/row indices
        if (++cIdx == nCols) {
            cIdx = 0;
            rIdx++;
        }
    }

    const int pageRowIdx = ((centerFirstPage ? nCols - 1 : 0) + currentItem->pageNumber()) / nCols;

    // 2) compute full size
    for (int i = 0; i < nCols; i++)
        fullWidth += colWidth[i];
    if (continuousView) {
        for (int i = 0; i < nRows; i++)
            fullHeight += rowHeight[i];
    } else
        fullHeight = rowHeight[pageRowIdx];

    // 3) arrange widgets inside cells (and refine fullHeight if needed)
    int insertX = 0, insertY = fullHeight < viewportHeight ? (viewportHeight - fullHeight) / 2 : 0;
    const int origInsertY = insertY;
    cIdx = 0;
    rIdx = 0;
    if (centerFirstPage) {
        cIdx += nCols - 1;
        for (int i = 0; i < cIdx; ++i)
            insertX += colWidth[i];
    }
    for (PageViewItem *item : qAsConst(d->items)) {
        int cWidth = colWidth[cIdx], rHeight = rowHeight[rIdx];
        if (continuousView || rIdx == pageRowIdx) {
            const bool reallyDoCenterFirst = item->pageNumber() == 0 && centerFirstPage;
            const bool reallyDoCenterLast = item->pageNumber() == pageCount - 1 && centerLastPage;
            int actualX = 0;
            if (reallyDoCenterFirst || reallyDoCenterLast) {
                // page is centered across entire viewport
                actualX = (fullWidth - item->croppedWidth()) / 2;
            } else if (facingPages) {
                if (Okular::Settings::rtlReadingDirection()) {
                    // RTL reading mode
                    actualX = ((centerFirstPage && item->pageNumber() % 2 == 0) || (!centerFirstPage && item->pageNumber() % 2 == 1)) ? (fullWidth / 2) - item->croppedWidth() - 1 : (fullWidth / 2) + 1;
                } else {
                    // page edges 'touch' the center of the viewport
                    actualX = ((centerFirstPage && item->pageNumber() % 2 == 1) || (!centerFirstPage && item->pageNumber() % 2 == 0)) ? (fullWidth / 2) - item->croppedWidth() - 1 : (fullWidth / 2) + 1;
                }
            } else {
                // page is centered within its virtual column
                // actualX = insertX + (cWidth - item->croppedWidth()) / 2;
                if (Okular::Settings::rtlReadingDirection()) {
                    actualX = fullWidth - insertX - cWidth + ((cWidth - item->croppedWidth()) / 2);
                } else {
                    actualX = insertX + (cWidth - item->croppedWidth()) / 2;
                }
            }
            item->moveTo(actualX, (continuousView ? insertY : origInsertY) + (rHeight - item->croppedHeight()) / 2);
            item->setVisible(true);
        } else {
            item->moveTo(0, 0);
            item->setVisible(false);
        }
        item->setFormWidgetsVisible(d->m_formsVisible);
        // advance col/row index
        insertX += cWidth;
        if (++cIdx == nCols) {
            cIdx = 0;
            rIdx++;
            insertX = 0;
            insertY += rHeight;
        }
#ifdef PAGEVIEW_DEBUG
        kWarning() << "updating size for pageno" << item->pageNumber() << "cropped" << item->croppedGeometry() << "uncropped" << item->uncroppedGeometry();
#endif
    }

    delete[] colWidth;
    delete[] rowHeight;

    // 3) reset dirty state
    d->dirtyLayout = false;

    // 4) update scrollview's contents size and recenter view
    bool wasUpdatesEnabled = viewport()->updatesEnabled();
    if (fullWidth != contentAreaWidth() || fullHeight != contentAreaHeight()) {
        const Okular::DocumentViewport vp = d->document->viewport();
        // disable updates and resize the viewportContents
        if (wasUpdatesEnabled)
            viewport()->setUpdatesEnabled(false);
        resizeContentArea(QSize(fullWidth, fullHeight));
        // restore previous viewport if defined and updates enabled
        if (wasUpdatesEnabled) {
            if (vp.pageNumber >= 0) {
                int prevX = horizontalScrollBar()->value(), prevY = verticalScrollBar()->value();

                const QPoint centerPos = viewportToContentArea(vp);
                center(centerPos.x(), centerPos.y());

                // center() usually moves the viewport, that requests pixmaps too.
                // if that doesn't happen we have to request them by hand
                if (prevX == horizontalScrollBar()->value() && prevY == verticalScrollBar()->value())
                    slotRequestVisiblePixmaps();
            }
            // or else go to center page
            else
                center(fullWidth / 2, 0);
            viewport()->setUpdatesEnabled(true);
        }
    } else {
        slotRequestVisiblePixmaps();
    }

    // 5) update the whole viewport if updated enabled
    if (wasUpdatesEnabled)
        viewport()->update();
}

void PageView::delayedResizeEvent()
{
    // If we already got here we don't need to execute the timer slot again
    d->delayResizeEventTimer->stop();
    slotRelayoutPages();
    slotRequestVisiblePixmaps();
}

static void slotRequestPreloadPixmap(PageView *pageView, const PageViewItem *i, const QRect expandedViewportRect, QLinkedList<Okular::PixmapRequest *> *requestedPixmaps)
{
    Okular::NormalizedRect preRenderRegion;
    const QRect intersectionRect = expandedViewportRect.intersected(i->croppedGeometry());
    if (!intersectionRect.isEmpty())
        preRenderRegion = Okular::NormalizedRect(intersectionRect.translated(-i->uncroppedGeometry().topLeft()), i->uncroppedWidth(), i->uncroppedHeight());

    // request the pixmap if not already present
    if (!i->page()->hasPixmap(pageView, i->uncroppedWidth(), i->uncroppedHeight(), preRenderRegion) && i->uncroppedWidth() > 0) {
        Okular::PixmapRequest::PixmapRequestFeatures requestFeatures = Okular::PixmapRequest::Preload;
        requestFeatures |= Okular::PixmapRequest::Asynchronous;
        const bool pageHasTilesManager = i->page()->hasTilesManager(pageView);
        if (pageHasTilesManager && !preRenderRegion.isNull()) {
            Okular::PixmapRequest *p = new Okular::PixmapRequest(pageView, i->pageNumber(), i->uncroppedWidth(), i->uncroppedHeight(), pageView->devicePixelRatioF(), PAGEVIEW_PRELOAD_PRIO, requestFeatures);
            requestedPixmaps->push_back(p);

            p->setNormalizedRect(preRenderRegion);
            p->setTile(true);
        } else if (!pageHasTilesManager) {
            Okular::PixmapRequest *p = new Okular::PixmapRequest(pageView, i->pageNumber(), i->uncroppedWidth(), i->uncroppedHeight(), pageView->devicePixelRatioF(), PAGEVIEW_PRELOAD_PRIO, requestFeatures);
            requestedPixmaps->push_back(p);
            p->setNormalizedRect(preRenderRegion);
        }
    }
}

void PageView::slotRequestVisiblePixmaps(int newValue)
{
    // if requests are blocked (because raised by an unwanted event), exit
    if (d->blockPixmapsRequest)
        return;

    // precalc view limits for intersecting with page coords inside the loop
    const bool isEvent = newValue != -1 && !d->blockViewport;
    const QRect viewportRect(horizontalScrollBar()->value(), verticalScrollBar()->value(), viewport()->width(), viewport()->height());
    const QRect viewportRectAtZeroZero(0, 0, viewport()->width(), viewport()->height());

    // some variables used to determine the viewport
    int nearPageNumber = -1;
    const double viewportCenterX = (viewportRect.left() + viewportRect.right()) / 2.0;
    const double viewportCenterY = (viewportRect.top() + viewportRect.bottom()) / 2.0;
    double focusedX = 0.5, focusedY = 0.0, minDistance = -1.0;
    // Margin (in pixels) around the viewport to preload
    const int pixelsToExpand = 512;

    // iterate over all items
    d->visibleItems.clear();
    QLinkedList<Okular::PixmapRequest *> requestedPixmaps;
    QVector<Okular::VisiblePageRect *> visibleRects;
    for (PageViewItem *i : qAsConst(d->items)) {
        const QSet<FormWidgetIface *> formWidgetsList = i->formWidgets();
        for (FormWidgetIface *fwi : formWidgetsList) {
            Okular::NormalizedRect r = fwi->rect();
            fwi->moveTo(qRound(i->uncroppedGeometry().left() + i->uncroppedWidth() * r.left) + 1 - viewportRect.left(), qRound(i->uncroppedGeometry().top() + i->uncroppedHeight() * r.top) + 1 - viewportRect.top());
        }
        const QHash<Okular::Movie *, VideoWidget *> videoWidgets = i->videoWidgets();
        for (VideoWidget *vw : videoWidgets) {
            const Okular::NormalizedRect r = vw->normGeometry();
            vw->move(qRound(i->uncroppedGeometry().left() + i->uncroppedWidth() * r.left) + 1 - viewportRect.left(), qRound(i->uncroppedGeometry().top() + i->uncroppedHeight() * r.top) + 1 - viewportRect.top());

            if (vw->isPlaying() && viewportRectAtZeroZero.intersected(vw->geometry()).isEmpty()) {
                vw->stop();
                vw->pageLeft();
            }
        }

        if (!i->isVisible())
            continue;
#ifdef PAGEVIEW_DEBUG
        kWarning() << "checking page" << i->pageNumber();
        kWarning().nospace() << "viewportRect is " << viewportRect << ", page item is " << i->croppedGeometry() << " intersect : " << viewportRect.intersects(i->croppedGeometry());
#endif
        // if the item doesn't intersect the viewport, skip it
        QRect intersectionRect = viewportRect.intersected(i->croppedGeometry());
        if (intersectionRect.isEmpty()) {
            continue;
        }

        // add the item to the 'visible list'
        d->visibleItems.push_back(i);
        Okular::VisiblePageRect *vItem = new Okular::VisiblePageRect(i->pageNumber(), Okular::NormalizedRect(intersectionRect.translated(-i->uncroppedGeometry().topLeft()), i->uncroppedWidth(), i->uncroppedHeight()));
        visibleRects.push_back(vItem);
#ifdef PAGEVIEW_DEBUG
        kWarning() << "checking for pixmap for page" << i->pageNumber() << "=" << i->page()->hasPixmap(this, i->uncroppedWidth(), i->uncroppedHeight());
        kWarning() << "checking for text for page" << i->pageNumber() << "=" << i->page()->hasTextPage();
#endif

        Okular::NormalizedRect expandedVisibleRect = vItem->rect;
        if (i->page()->hasTilesManager(this) && Okular::Settings::memoryLevel() != Okular::Settings::EnumMemoryLevel::Low) {
            double rectMargin = pixelsToExpand / (double)i->uncroppedHeight();
            expandedVisibleRect.left = qMax(0.0, vItem->rect.left - rectMargin);
            expandedVisibleRect.top = qMax(0.0, vItem->rect.top - rectMargin);
            expandedVisibleRect.right = qMin(1.0, vItem->rect.right + rectMargin);
            expandedVisibleRect.bottom = qMin(1.0, vItem->rect.bottom + rectMargin);
        }

        // if the item has not the right pixmap, add a request for it
        if (!i->page()->hasPixmap(this, i->uncroppedWidth(), i->uncroppedHeight(), expandedVisibleRect)) {
#ifdef PAGEVIEW_DEBUG
            kWarning() << "rerequesting visible pixmaps for page" << i->pageNumber() << "!";
#endif
            Okular::PixmapRequest *p = new Okular::PixmapRequest(this, i->pageNumber(), i->uncroppedWidth(), i->uncroppedHeight(), devicePixelRatioF(), PAGEVIEW_PRIO, Okular::PixmapRequest::Asynchronous);
            requestedPixmaps.push_back(p);

            if (i->page()->hasTilesManager(this)) {
                p->setNormalizedRect(expandedVisibleRect);
                p->setTile(true);
            } else
                p->setNormalizedRect(vItem->rect);
        }

        // look for the item closest to viewport center and the relative
        // position between the item and the viewport center
        if (isEvent) {
            const QRect &geometry = i->croppedGeometry();
            // compute distance between item center and viewport center (slightly moved left)
            const double distance = hypot((geometry.left() + geometry.right()) / 2.0 - (viewportCenterX - 4), (geometry.top() + geometry.bottom()) / 2.0 - viewportCenterY);
            if (distance >= minDistance && nearPageNumber != -1)
                continue;
            nearPageNumber = i->pageNumber();
            minDistance = distance;
            if (geometry.height() > 0 && geometry.width() > 0) {
                // Compute normalized coordinates w.r.t. cropped page
                focusedX = (viewportCenterX - (double)geometry.left()) / (double)geometry.width();
                focusedY = (viewportCenterY - (double)geometry.top()) / (double)geometry.height();
                // Convert to normalized coordinates w.r.t. full page (no-op if not cropped)
                focusedX = i->crop().left + focusedX * i->crop().width();
                focusedY = i->crop().top + focusedY * i->crop().height();
            }
        }
    }

    // if preloading is enabled, add the pages before and after in preloading
    if (!d->visibleItems.isEmpty() && Okular::SettingsCore::memoryLevel() != Okular::SettingsCore::EnumMemoryLevel::Low) {
        // as the requests are done in the order as they appear in the list,
        // request first the next page and then the previous

        int pagesToPreload = viewColumns();

        // if the greedy option is set, preload all pages
        if (Okular::SettingsCore::memoryLevel() == Okular::SettingsCore::EnumMemoryLevel::Greedy)
            pagesToPreload = d->items.count();

        const QRect expandedViewportRect = viewportRect.adjusted(0, -pixelsToExpand, 0, pixelsToExpand);

        for (int j = 1; j <= pagesToPreload; j++) {
            // add the page after the 'visible series' in preload
            const int tailRequest = d->visibleItems.last()->pageNumber() + j;
            if (tailRequest < (int)d->items.count()) {
                slotRequestPreloadPixmap(this, d->items[tailRequest], expandedViewportRect, &requestedPixmaps);
            }

            // add the page before the 'visible series' in preload
            const int headRequest = d->visibleItems.first()->pageNumber() - j;
            if (headRequest >= 0) {
                slotRequestPreloadPixmap(this, d->items[headRequest], expandedViewportRect, &requestedPixmaps);
            }

            // stop if we've already reached both ends of the document
            if (headRequest < 0 && tailRequest >= (int)d->items.count())
                break;
        }
    }

    // send requests to the document
    if (!requestedPixmaps.isEmpty()) {
        d->document->requestPixmaps(requestedPixmaps);
    }
    // if this functions was invoked by viewport events, send update to document
    if (isEvent && nearPageNumber != -1) {
        // determine the document viewport
        Okular::DocumentViewport newViewport(nearPageNumber);
        newViewport.rePos.enabled = true;
        newViewport.rePos.normalizedX = focusedX;
        newViewport.rePos.normalizedY = focusedY;
        // set the viewport to other observers
        // do not update history if the viewport is autoscrolling
        d->document->setViewportWithHistory(newViewport, this, false, d->scroller->state() != QScroller::Scrolling);
    }
    d->document->setVisiblePageRects(visibleRects, this);
}

void PageView::slotAutoScroll()
{
    // the first time create the timer
    if (!d->autoScrollTimer) {
        d->autoScrollTimer = new QTimer(this);
        d->autoScrollTimer->setSingleShot(true);
        connect(d->autoScrollTimer, &QTimer::timeout, this, &PageView::slotAutoScroll);
    }

    // if scrollIncrement is zero, stop the timer
    if (!d->scrollIncrement) {
        d->autoScrollTimer->stop();
        return;
    }

    // compute delay between timer ticks and scroll amount per tick
    int index = abs(d->scrollIncrement) - 1; // 0..9
    const int scrollDelay[10] = {200, 100, 50, 30, 20, 30, 25, 20, 30, 20};
    const int scrollOffset[10] = {1, 1, 1, 1, 1, 2, 2, 2, 4, 4};
    d->autoScrollTimer->start(scrollDelay[index]);
    int delta = d->scrollIncrement > 0 ? scrollOffset[index] : -scrollOffset[index];
    d->scroller->scrollTo(d->scroller->finalPosition() + QPoint(0, delta), scrollDelay[index]);
}

void PageView::slotDragScroll()
{
    scrollTo(horizontalScrollBar()->value() + d->dragScrollVector.x(), verticalScrollBar()->value() + d->dragScrollVector.y());
    QPoint p = contentAreaPosition() + viewport()->mapFromGlobal(QCursor::pos());
    updateSelection(p);
}

void PageView::slotShowWelcome()
{
    // show initial welcome text
    d->messageWindow->display(i18n("Welcome"), QString(), PageViewMessage::Info, 2000);
}

void PageView::slotShowSizeAllCursor()
{
    setCursor(Qt::SizeAllCursor);
}

void PageView::slotHandleWebShortcutAction()
{
    QAction *action = qobject_cast<QAction *>(sender());

    if (action) {
        KUriFilterData filterData(action->data().toString());

        if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::WebShortcutFilter)) {
            QDesktopServices::openUrl(filterData.uri());
        }
    }
}

void PageView::slotConfigureWebShortcuts()
{
    KToolInvocation::kdeinitExec(QStringLiteral("kcmshell5"), QStringList() << QStringLiteral("webshortcuts"));
}

void PageView::slotZoom()
{
    if (!d->aZoom->selectableActionGroup()->isEnabled())
        return;

    setFocus();
    updateZoom(ZoomFixed);
}

void PageView::slotZoomIn()
{
    updateZoom(ZoomIn);
}

void PageView::slotZoomOut()
{
    updateZoom(ZoomOut);
}

void PageView::slotZoomActual()
{
    updateZoom(ZoomActual);
}

void PageView::slotFitToWidthToggled(bool on)
{
    if (on)
        updateZoom(ZoomFitWidth);
}

void PageView::slotFitToPageToggled(bool on)
{
    if (on)
        updateZoom(ZoomFitPage);
}

void PageView::slotAutoFitToggled(bool on)
{
    if (on)
        updateZoom(ZoomFitAuto);
}

void PageView::slotViewMode(QAction *action)
{
    const int nr = action->data().toInt();
    if ((int)Okular::Settings::viewMode() != nr) {
        Okular::Settings::setViewMode(nr);
        Okular::Settings::self()->save();
        if (d->document->pages() > 0)
            slotRelayoutPages();
    }
}

void PageView::slotContinuousToggled()
{
    if (d->document->pages() > 0)
        slotRelayoutPages();
}

void PageView::slotReadingDirectionToggled(bool leftToRight)
{
    Okular::Settings::setRtlReadingDirection(leftToRight);
    Okular::Settings::self()->save();
}

void PageView::slotUpdateReadingDirectionAction()
{
    d->aReadingDirection->setChecked(Okular::Settings::rtlReadingDirection());
}

void PageView::slotSetMouseNormal()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::Browse;
    Okular::Settings::setMouseMode(d->mouseMode);
    // hide the messageWindow
    d->messageWindow->hide();
    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
    d->annotator->detachAnnotation();
}

void PageView::slotSetMouseZoom()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::Zoom;
    Okular::Settings::setMouseMode(d->mouseMode);
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display(i18n("Select zooming area. Right-click to zoom out."), QString(), PageViewMessage::Info, -1);
    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
    d->annotator->detachAnnotation();
}

void PageView::slotSetMouseMagnifier()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::Magnifier;
    Okular::Settings::setMouseMode(d->mouseMode);
    d->messageWindow->display(i18n("Click to see the magnified view."), QString());

    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
    d->annotator->detachAnnotation();
}

void PageView::slotSetMouseSelect()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::RectSelect;
    Okular::Settings::setMouseMode(d->mouseMode);
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display(i18n("Draw a rectangle around the text/graphics to copy."), QString(), PageViewMessage::Info, -1);
    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
    d->annotator->detachAnnotation();
}

void PageView::slotSetMouseTextSelect()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::TextSelect;
    Okular::Settings::setMouseMode(d->mouseMode);
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display(i18n("Select text"), QString(), PageViewMessage::Info, -1);
    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
    d->annotator->detachAnnotation();
}

void PageView::slotSetMouseTableSelect()
{
    d->mouseMode = Okular::Settings::EnumMouseMode::TableSelect;
    Okular::Settings::setMouseMode(d->mouseMode);
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display(i18n("Draw a rectangle around the table, then click near edges to divide up; press Esc to clear."), QString(), PageViewMessage::Info, -1);
    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
    d->annotator->detachAnnotation();
}

void PageView::slotSignature()
{
    if (!d->document->isHistoryClean()) {
        KMessageBox::information(this, i18n("You have unsaved changes. Please save the document before signing it."));
        return;
    }

    d->messageWindow->display(i18n("Draw a rectangle to insert the signature field"), QString(), PageViewMessage::Info, -1);

    d->annotator->setSignatureMode(true);

    // force an update of the cursor
    updateCursor();
    Okular::Settings::self()->save();
}

void PageView::slotAutoScrollUp()
{
    if (d->scrollIncrement < -9)
        return;
    d->scrollIncrement--;
    slotAutoScroll();
    setFocus();
}

void PageView::slotAutoScrollDown()
{
    if (d->scrollIncrement > 9)
        return;
    d->scrollIncrement++;
    slotAutoScroll();
    setFocus();
}

void PageView::slotScrollUp(int nSteps)
{
    if (verticalScrollBar()->value() > verticalScrollBar()->minimum()) {
        if (nSteps) {
            d->scroller->scrollTo(d->scroller->finalPosition() + QPoint(0, -100 * nSteps), d->currentShortScrollDuration);
        } else {
            if (d->scroller->finalPosition().y() > verticalScrollBar()->minimum())
                d->scroller->scrollTo(d->scroller->finalPosition() + QPoint(0, -(1 - Okular::Settings::scrollOverlap() / 100.0) * viewport()->height()), d->currentLongScrollDuration);
        }
    } else if (!getContinuousMode() && d->document->currentPage() > 0) {
        // Since we are in single page mode and at the top of the page, go to previous page.
        // setViewport() is more optimized than document->setPrevPage and then move view to bottom.
        Okular::DocumentViewport newViewport = d->document->viewport();
        newViewport.pageNumber -= viewColumns();
        if (newViewport.pageNumber < 0)
            newViewport.pageNumber = 0;
        newViewport.rePos.enabled = true;
        newViewport.rePos.normalizedY = 1.0;
        d->document->setViewport(newViewport);
    }
}

void PageView::slotScrollDown(int nSteps)
{
    if (verticalScrollBar()->value() < verticalScrollBar()->maximum()) {
        if (nSteps) {
            d->scroller->scrollTo(d->scroller->finalPosition() + QPoint(0, 100 * nSteps), d->currentShortScrollDuration);
        } else {
            if (d->scroller->finalPosition().y() < verticalScrollBar()->maximum())
                d->scroller->scrollTo(d->scroller->finalPosition() + QPoint(0, (1 - Okular::Settings::scrollOverlap() / 100.0) * viewport()->height()), d->currentLongScrollDuration);
        }
    } else if (!getContinuousMode() && (int)d->document->currentPage() < d->items.count() - 1) {
        // Since we are in single page mode and at the bottom of the page, go to next page.
        // setViewport() is more optimized than document->setNextPage and then move view to top
        Okular::DocumentViewport newViewport = d->document->viewport();
        newViewport.pageNumber += viewColumns();
        if (newViewport.pageNumber >= (int)d->items.count())
            newViewport.pageNumber = d->items.count() - 1;
        newViewport.rePos.enabled = true;
        newViewport.rePos.normalizedY = 0.0;
        d->document->setViewport(newViewport);
    }
}

void PageView::slotRotateClockwise()
{
    int id = ((int)d->document->rotation() + 1) % 4;
    d->document->setRotation(id);
}

void PageView::slotRotateCounterClockwise()
{
    int id = ((int)d->document->rotation() + 3) % 4;
    d->document->setRotation(id);
}

void PageView::slotRotateOriginal()
{
    d->document->setRotation(0);
}

// Enforce mutual-exclusion between trim modes
// Each mode is uniquely identified by a single value
// From Okular::Settings::EnumTrimMode
void PageView::updateTrimMode(int except_id)
{
    const QList<QAction *> trimModeActions = d->aTrimMode->menu()->actions();
    for (QAction *trimModeAction : trimModeActions) {
        if (trimModeAction->data().toInt() != except_id)
            trimModeAction->setChecked(false);
    }
}

bool PageView::mouseReleaseOverLink(const Okular::ObjectRect *rect) const
{
    if (rect) {
        // handle click over a link
        const Okular::Action *action = static_cast<const Okular::Action *>(rect->object());
        d->document->processAction(action);
        return true;
    }
    return false;
}

void PageView::slotTrimMarginsToggled(bool on)
{
    if (on) { // Turn off any other Trim modes
        updateTrimMode(d->aTrimMargins->data().toInt());
    }

    if (Okular::Settings::trimMargins() != on) {
        Okular::Settings::setTrimMargins(on);
        Okular::Settings::self()->save();
        if (d->document->pages() > 0) {
            slotRelayoutPages();
            slotRequestVisiblePixmaps(); // TODO: slotRelayoutPages() may have done this already!
        }
    }
}

void PageView::slotTrimToSelectionToggled(bool on)
{
    if (on) { // Turn off any other Trim modes
        updateTrimMode(d->aTrimToSelection->data().toInt());

        // Change the mouse mode
        d->mouseMode = Okular::Settings::EnumMouseMode::TrimSelect;
        d->aMouseNormal->setChecked(false);

        // change the text in messageWindow (and show it if hidden)
        d->messageWindow->display(i18n("Draw a rectangle around the page area you wish to keep visible"), QString(), PageViewMessage::Info, -1);
        // force an update of the cursor
        updateCursor();
    } else {
        // toggled off while making selection
        if (Okular::Settings::EnumMouseMode::TrimSelect == d->mouseMode) {
            // clear widget selection and invalidate rect
            selectionClear();

            // When Trim selection bbox interaction is over, we should switch to another mousemode.
            if (d->aPrevAction) {
                d->aPrevAction->trigger();
                d->aPrevAction = nullptr;
            } else {
                d->aMouseNormal->trigger();
            }
        }

        d->trimBoundingBox = Okular::NormalizedRect(); // invalidate box
        if (d->document->pages() > 0) {
            slotRelayoutPages();
            slotRequestVisiblePixmaps(); // TODO: slotRelayoutPages() may have done this already!
        }
    }
}

void PageView::slotToggleForms()
{
    toggleFormWidgets(!d->m_formsVisible);
}

void PageView::slotFormChanged(int pageNumber)
{
    if (!d->refreshTimer) {
        d->refreshTimer = new QTimer(this);
        d->refreshTimer->setSingleShot(true);
        connect(d->refreshTimer, &QTimer::timeout, this, &PageView::slotRefreshPage);
    }
    d->refreshPages << pageNumber;
    int delay = 0;
    if (d->m_formsVisible) {
        delay = 1000;
    }
    d->refreshTimer->start(delay);
}

void PageView::slotRefreshPage()
{
    for (int req : qAsConst(d->refreshPages)) {
        QTimer::singleShot(0, this, [this, req] { d->document->refreshPixmaps(req); });
    }
    d->refreshPages.clear();
}

#ifdef HAVE_SPEECH
void PageView::slotSpeakDocument()
{
    QString text;
    for (const PageViewItem *item : qAsConst(d->items)) {
        Okular::RegularAreaRect *area = textSelectionForItem(item);
        text.append(item->page()->text(area));
        text.append('\n');
        delete area;
    }

    d->tts()->say(text);
}

void PageView::slotSpeakCurrentPage()
{
    const int currentPage = d->document->viewport().pageNumber;

    PageViewItem *item = d->items.at(currentPage);
    Okular::RegularAreaRect *area = textSelectionForItem(item);
    const QString text = item->page()->text(area);
    delete area;

    d->tts()->say(text);
}

void PageView::slotStopSpeaks()
{
    if (!d->m_tts)
        return;

    d->m_tts->stopAllSpeechs();
}

void PageView::slotPauseResumeSpeech()
{
    if (!d->m_tts)
        return;

    d->m_tts->pauseResumeSpeech();
}

#endif

void PageView::slotAction(Okular::Action *action)
{
    d->document->processAction(action);
}

void PageView::externalKeyPressEvent(QKeyEvent *e)
{
    keyPressEvent(e);
}

void PageView::slotProcessMovieAction(const Okular::MovieAction *action)
{
    const Okular::MovieAnnotation *movieAnnotation = action->annotation();
    if (!movieAnnotation)
        return;

    Okular::Movie *movie = movieAnnotation->movie();
    if (!movie)
        return;

    const int currentPage = d->document->viewport().pageNumber;

    PageViewItem *item = d->items.at(currentPage);
    if (!item)
        return;

    VideoWidget *vw = item->videoWidgets().value(movie);
    if (!vw)
        return;

    vw->show();

    switch (action->operation()) {
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

void PageView::slotProcessRenditionAction(const Okular::RenditionAction *action)
{
    Okular::Movie *movie = action->movie();
    if (!movie)
        return;

    const int currentPage = d->document->viewport().pageNumber;

    PageViewItem *item = d->items.at(currentPage);
    if (!item)
        return;

    VideoWidget *vw = item->videoWidgets().value(movie);
    if (!vw)
        return;

    if (action->operation() == Okular::RenditionAction::None)
        return;

    vw->show();

    switch (action->operation()) {
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

void PageView::slotFitWindowToPage()
{
    const PageViewItem *currentPageItem = nullptr;
    QSize viewportSize = viewport()->size();
    for (const PageViewItem *pageItem : qAsConst(d->items)) {
        if (pageItem->isVisible()) {
            currentPageItem = pageItem;
            break;
        }
    }

    if (!currentPageItem)
        return;

    const QSize pageSize = QSize(currentPageItem->uncroppedWidth() + kcolWidthMargin, currentPageItem->uncroppedHeight() + krowHeightMargin);
    if (verticalScrollBar()->isVisible())
        viewportSize.setWidth(viewportSize.width() + verticalScrollBar()->width());
    if (horizontalScrollBar()->isVisible())
        viewportSize.setHeight(viewportSize.height() + horizontalScrollBar()->height());
    emit fitWindowToPage(viewportSize, pageSize);
}

void PageView::slotSelectPage()
{
    textSelectionClear();
    const int currentPage = d->document->viewport().pageNumber;
    PageViewItem *item = d->items.at(currentPage);

    if (item) {
        Okular::RegularAreaRect *area = textSelectionForItem(item);
        d->pagesWithTextSelection.insert(currentPage);
        d->document->setPageTextSelection(currentPage, area, palette().color(QPalette::Active, QPalette::Highlight));
    }
}

void PageView::highlightSignatureFormWidget(const Okular::FormFieldSignature *form)
{
    QVector<PageViewItem *>::const_iterator dIt = d->items.constBegin(), dEnd = d->items.constEnd();
    for (; dIt != dEnd; ++dIt) {
        const QSet<FormWidgetIface *> fwi = (*dIt)->formWidgets();
        for (FormWidgetIface *fw : fwi) {
            if (fw->formField() == form) {
                SignatureEdit *widget = static_cast<SignatureEdit *>(fw);
                widget->setDummyMode(true);
                QTimer::singleShot(250, this, [=] { widget->setDummyMode(false); });
                return;
            }
        }
    }
}

// END private SLOTS

/* kate: replace-tabs on; indent-width 4; */
