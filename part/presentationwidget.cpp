/*
    SPDX-FileCopyrightText: 2004 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "presentationwidget.h"

// qt/kde includes
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QLoggingCategory>

#include <KActionCollection>
#include <KCursor>
#include <KLineEdit>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRandom>
#include <KSelectAction>
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QEvent>
#include <QFontMetrics>
#include <QGestureEvent>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QScreen>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QToolBar>
#include <QToolTip>
#include <QValidator>

#ifdef Q_OS_LINUX
#include <QDBusUnixFileDescriptor>
#include <unistd.h> // For ::close() for sleep inhibition
#endif

// system includes
#include <math.h>
#include <stdlib.h>

// local includes
#include "annotationtools.h"
#include "core/action.h"
#include "core/annotations.h"
#include "core/audioplayer.h"
#include "core/document.h"
#include "core/generator.h"
#include "core/movie.h"
#include "core/page.h"
#include "debug_ui.h"
#include "drawingtoolactions.h"
#include "guiutils.h"
#include "pagepainter.h"
#include "presentationsearchbar.h"
#include "priorities.h"
#include "settings.h"
#include "settings_core.h"
#include "videowidget.h"

// comment this to disable the top-right progress indicator
#define ENABLE_PROGRESS_OVERLAY

// a frame contains a pointer to the page object, its geometry and the
// transition effect to the next frame
struct PresentationFrame {
    PresentationFrame() = default;

    ~PresentationFrame()
    {
        qDeleteAll(videoWidgets);
    }

    PresentationFrame(const PresentationFrame &) = delete;
    PresentationFrame &operator=(const PresentationFrame &) = delete;

    void recalcGeometry(int width, int height, float screenRatio)
    {
        // calculate frame geometry keeping constant aspect ratio
        float pageRatio = page->ratio();
        int pageWidth = width, pageHeight = height;
        if (pageRatio > screenRatio)
            pageWidth = (int)((float)pageHeight / pageRatio);
        else
            pageHeight = (int)((float)pageWidth * pageRatio);
        geometry.setRect((width - pageWidth) / 2, (height - pageHeight) / 2, pageWidth, pageHeight);

        for (VideoWidget *vw : qAsConst(videoWidgets)) {
            const Okular::NormalizedRect r = vw->normGeometry();
            QRect vwgeom = r.geometry(geometry.width(), geometry.height());
            vw->resize(vwgeom.size());
            vw->move(geometry.topLeft() + vwgeom.topLeft());
        }
    }

    const Okular::Page *page;
    QRect geometry;
    QHash<Okular::Movie *, VideoWidget *> videoWidgets;
    QLinkedList<SmoothPath> drawings;
};

// a custom QToolBar that basically does not propagate the event if the widget
// background is not automatically filled
class PresentationToolBar : public QToolBar
{
    Q_OBJECT

public:
    PresentationToolBar(QWidget *parent = Q_NULLPTR)
        : QToolBar(parent)
    {
    }

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        QToolBar::mousePressEvent(e);
        e->accept();
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        QToolBar::mouseReleaseEvent(e);
        e->accept();
    }
};

PresentationWidget::PresentationWidget(QWidget *parent, Okular::Document *doc, DrawingToolActions *drawingToolActions, KActionCollection *collection)
    : QWidget(nullptr /* must be null, to have an independent widget */, Qt::FramelessWindowHint)
    , m_pressedLink(nullptr)
    , m_handCursor(false)
    , m_drawingEngine(nullptr)
    , m_screenInhibitCookie(0)
    , m_sleepInhibitFd(-1)
    , m_parentWidget(parent)
    , m_document(doc)
    , m_frameIndex(-1)
    , m_topBar(nullptr)
    , m_pagesEdit(nullptr)
    , m_searchBar(nullptr)
    , m_ac(collection)
    , m_screenSelect(nullptr)
    , m_isSetup(false)
    , m_blockNotifications(false)
    , m_inBlackScreenMode(false)
    , m_showSummaryView(Okular::Settings::slidesShowSummary())
    , m_advanceSlides(Okular::SettingsCore::slidesAdvance())
    , m_goToPreviousPageOnRelease(false)
    , m_goToNextPageOnRelease(false)
{
    Q_UNUSED(parent)
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setObjectName(QStringLiteral("presentationWidget"));
    QString caption = doc->metaData(QStringLiteral("DocumentTitle")).toString();
    if (caption.trimmed().isEmpty())
        caption = doc->currentDocument().fileName();
    caption = i18nc("[document title/filename] – Presentation", "%1 – Presentation", caption);
    setWindowTitle(caption);

    m_width = -1;

    // create top toolbar
    m_topBar = new PresentationToolBar(this);
    m_topBar->setObjectName(QStringLiteral("presentationBar"));
    m_topBar->setMovable(false);
    m_topBar->layout()->setContentsMargins(0, 0, 0, 0);
    m_topBar->addAction(QIcon::fromTheme(layoutDirection() == Qt::RightToLeft ? QStringLiteral("go-next") : QStringLiteral("go-previous")), i18n("Previous Page"), this, SLOT(slotPrevPage()));
    m_pagesEdit = new KLineEdit(m_topBar);
    QSizePolicy sp = m_pagesEdit->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Minimum);
    m_pagesEdit->setSizePolicy(sp);
    QFontMetrics fm(m_pagesEdit->font());
    QStyleOptionFrame option;
    option.initFrom(m_pagesEdit);
    m_pagesEdit->setMaximumWidth(fm.horizontalAdvance(QString::number(m_document->pages())) + 2 * style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, m_pagesEdit) +
                                 4); // the 4 comes from 2*horizontalMargin, horizontalMargin being a define in qlineedit.cpp
    QIntValidator *validator = new QIntValidator(1, m_document->pages(), m_pagesEdit);
    m_pagesEdit->setValidator(validator);
    m_topBar->addWidget(m_pagesEdit);
    QLabel *pagesLabel = new QLabel(m_topBar);
    pagesLabel->setText(QLatin1String(" / ") + QString::number(m_document->pages()) + QLatin1String(" "));
    m_topBar->addWidget(pagesLabel);
    connect(m_pagesEdit, &QLineEdit::returnPressed, this, &PresentationWidget::slotPageChanged);
    m_topBar->addAction(QIcon::fromTheme(layoutDirection() == Qt::RightToLeft ? QStringLiteral("go-previous") : QStringLiteral("go-next")), i18n("Next Page"), this, SLOT(slotNextPage()));
    m_topBar->addSeparator();
    QAction *playPauseAct = collection->action(QStringLiteral("presentation_play_pause"));
    playPauseAct->setEnabled(true);
    connect(playPauseAct, &QAction::triggered, this, &PresentationWidget::slotTogglePlayPause);
    m_topBar->addAction(playPauseAct);
    addAction(playPauseAct);
    m_topBar->addSeparator();

    const QList<QAction *> actionsList = drawingToolActions->actions();
    for (QAction *action : actionsList) {
        action->setEnabled(true);
        m_topBar->addAction(action);
        addAction(action);
    }
    connect(drawingToolActions, &DrawingToolActions::changeEngine, this, &PresentationWidget::slotChangeDrawingToolEngine);
    connect(drawingToolActions, &DrawingToolActions::actionsRecreated, this, &PresentationWidget::slotAddDrawingToolActions);

    QAction *eraseDrawingAct = collection->action(QStringLiteral("presentation_erase_drawings"));
    eraseDrawingAct->setEnabled(true);
    connect(eraseDrawingAct, &QAction::triggered, this, &PresentationWidget::clearDrawings);
    m_topBar->addAction(eraseDrawingAct);
    addAction(eraseDrawingAct);

    const int screenCount = QApplication::screens().count();
    if (screenCount > 1) {
        m_topBar->addSeparator();
        m_screenSelect = new KSelectAction(QIcon::fromTheme(QStringLiteral("video-display")), i18n("Switch Screen"), m_topBar);
        m_screenSelect->setToolBarMode(KSelectAction::MenuMode);
        m_screenSelect->setToolButtonPopupMode(QToolButton::InstantPopup);
        m_topBar->addAction(m_screenSelect);
        for (int i = 0; i < screenCount; ++i) {
            QAction *act = m_screenSelect->addAction(i18nc("%1 is the screen number (0, 1, ...)", "Screen %1", i));
            act->setData(QVariant::fromValue(i));
        }
    }
    QWidget *spacer = new QWidget(m_topBar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    m_topBar->addWidget(spacer);
    m_topBar->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18n("Exit Presentation Mode"), this, SLOT(close()));
    m_topBar->setAutoFillBackground(true);
    showTopBar(false);
    // change topbar background color
    QPalette p = m_topBar->palette();
    p.setColor(QPalette::Active, QPalette::Button, Qt::gray);
    p.setColor(QPalette::Active, QPalette::Window, Qt::darkGray);
    m_topBar->setPalette(p);

    // Grab swipe gestures to change pages
    grabGesture(Qt::SwipeGesture);

    // misc stuff
    setMouseTracking(true);
    setContextMenuPolicy(Qt::PreventContextMenu);
    m_transitionTimer = new QTimer(this);
    m_transitionTimer->setSingleShot(true);
    connect(m_transitionTimer, &QTimer::timeout, this, &PresentationWidget::slotTransitionStep);
    m_overlayHideTimer = new QTimer(this);
    m_overlayHideTimer->setSingleShot(true);
    connect(m_overlayHideTimer, &QTimer::timeout, this, &PresentationWidget::slotHideOverlay);
    m_nextPageTimer = new QTimer(this);
    m_nextPageTimer->setSingleShot(true);
    connect(m_nextPageTimer, &QTimer::timeout, this, &PresentationWidget::slotNextPage);
    setPlayPauseIcon();

    connect(m_document, &Okular::Document::processMovieAction, this, &PresentationWidget::slotProcessMovieAction);
    connect(m_document, &Okular::Document::processRenditionAction, this, &PresentationWidget::slotProcessRenditionAction);

    // handle cursor appearance as specified in configuration
    if (Okular::Settings::slidesCursor() == Okular::Settings::EnumSlidesCursor::HiddenDelay) {
        KCursor::setAutoHideCursor(this, true);
        KCursor::setHideCursorDelay(3000);
    } else if (Okular::Settings::slidesCursor() == Okular::Settings::EnumSlidesCursor::Hidden) {
        setCursor(QCursor(Qt::BlankCursor));
    }

    setupActions();

    // inhibit power management
    inhibitPowerManagement();

    QTimer::singleShot(0, this, &PresentationWidget::slotDelayedEvents);

    // setFocus() so KCursor::setAutoHideCursor() goes into effect if it's enabled
    setFocus(Qt::OtherFocusReason);

    // Catch TabletEnterProximity and TabletLeaveProximity events from the QApplication
    qApp->installEventFilter(this);
}

PresentationWidget::~PresentationWidget()
{
    // allow power management saver again
    allowPowerManagement();

    // stop the audio playbacks
    Okular::AudioPlayer::instance()->stopPlaybacks();

    // remove our highlights
    if (m_searchBar) {
        m_document->resetSearch(PRESENTATION_SEARCH_ID);
    }

    // remove this widget from document observer
    m_document->removeObserver(this);

    const QList<QAction *> actionsList = actions();
    for (QAction *action : actionsList) {
        action->setChecked(false);
        action->setEnabled(false);
    }

    delete m_drawingEngine;

    // delete frames
    qDeleteAll(m_frames);

    qApp->removeEventFilter(this);
}

void PresentationWidget::notifySetup(const QVector<Okular::Page *> &pageSet, int setupFlags)
{
    // same document, nothing to change - here we assume the document sets up
    // us with the whole document set as first notifySetup()
    if (!(setupFlags & Okular::DocumentObserver::DocumentChanged))
        return;

    // delete previous frames (if any (shouldn't be))
    qDeleteAll(m_frames);
    if (!m_frames.isEmpty())
        qCWarning(OkularUiDebug) << "Frames setup changed while a Presentation is in progress.";
    m_frames.clear();

    // create the new frames
    float screenRatio = (float)m_height / (float)m_width;
    for (const Okular::Page *page : pageSet) {
        PresentationFrame *frame = new PresentationFrame();
        frame->page = page;
        const QLinkedList<Okular::Annotation *> annotations = page->annotations();
        for (Okular::Annotation *a : annotations) {
            if (a->subType() == Okular::Annotation::AMovie) {
                Okular::MovieAnnotation *movieAnn = static_cast<Okular::MovieAnnotation *>(a);
                VideoWidget *vw = new VideoWidget(movieAnn, movieAnn->movie(), m_document, this);
                frame->videoWidgets.insert(movieAnn->movie(), vw);
                vw->pageInitialized();
            } else if (a->subType() == Okular::Annotation::ARichMedia) {
                Okular::RichMediaAnnotation *richMediaAnn = static_cast<Okular::RichMediaAnnotation *>(a);
                if (richMediaAnn->movie()) {
                    VideoWidget *vw = new VideoWidget(richMediaAnn, richMediaAnn->movie(), m_document, this);
                    frame->videoWidgets.insert(richMediaAnn->movie(), vw);
                    vw->pageInitialized();
                }
            } else if (a->subType() == Okular::Annotation::AScreen) {
                const Okular::ScreenAnnotation *screenAnn = static_cast<Okular::ScreenAnnotation *>(a);
                Okular::Movie *movie = GuiUtils::renditionMovieFromScreenAnnotation(screenAnn);
                if (movie) {
                    VideoWidget *vw = new VideoWidget(screenAnn, movie, m_document, this);
                    frame->videoWidgets.insert(movie, vw);
                    vw->pageInitialized();
                }
            }
        }
        frame->recalcGeometry(m_width, m_height, screenRatio);
        // add the frame to the vector
        m_frames.push_back(frame);
    }

    // get metadata from the document
    m_metaStrings.clear();
    const Okular::DocumentInfo info = m_document->documentInfo(QSet<Okular::DocumentInfo::Key>() << Okular::DocumentInfo::Title << Okular::DocumentInfo::Author);
    if (!info.get(Okular::DocumentInfo::Title).isNull())
        m_metaStrings += i18n("Title: %1", info.get(Okular::DocumentInfo::Title));
    if (!info.get(Okular::DocumentInfo::Author).isNull())
        m_metaStrings += i18n("Author: %1", info.get(Okular::DocumentInfo::Author));
    m_metaStrings += i18n("Pages: %1", m_document->pages());
    m_metaStrings += i18n("Click to begin");

    m_isSetup = true;
}

void PresentationWidget::notifyViewportChanged(bool /*smoothMove*/)
{
    // display the current page
    changePage(m_document->viewport().pageNumber);

    // auto advance to the next page if set
    startAutoChangeTimer();
}

void PresentationWidget::notifyPageChanged(int pageNumber, int changedFlags)
{
    // if we are blocking the notifications, do nothing
    if (m_blockNotifications)
        return;

    // check if it's the last requested pixmap. if so update the widget.
    if ((changedFlags & (DocumentObserver::Pixmap | DocumentObserver::Annotations | DocumentObserver::Highlights)) && pageNumber == m_frameIndex)
        generatePage(changedFlags & (DocumentObserver::Annotations | DocumentObserver::Highlights));
}

void PresentationWidget::notifyCurrentPageChanged(int previousPage, int currentPage)
{
    if (previousPage != -1) {
        // stop video playback
        for (VideoWidget *vw : qAsConst(m_frames[previousPage]->videoWidgets)) {
            vw->stop();
            vw->pageLeft();
        }

        // stop audio playback, if any
        Okular::AudioPlayer::instance()->stopPlaybacks();

        // perform the page closing action, if any
        if (m_document->page(previousPage)->pageAction(Okular::Page::Closing))
            m_document->processAction(m_document->page(previousPage)->pageAction(Okular::Page::Closing));

        // perform the additional actions of the page's annotations, if any
        const QLinkedList<Okular::Annotation *> annotationsList = m_document->page(previousPage)->annotations();
        for (const Okular::Annotation *annotation : annotationsList) {
            Okular::Action *action = nullptr;

            if (annotation->subType() == Okular::Annotation::AScreen)
                action = static_cast<const Okular::ScreenAnnotation *>(annotation)->additionalAction(Okular::Annotation::PageClosing);
            else if (annotation->subType() == Okular::Annotation::AWidget)
                action = static_cast<const Okular::WidgetAnnotation *>(annotation)->additionalAction(Okular::Annotation::PageClosing);

            if (action)
                m_document->processAction(action);
        }
    }

    if (currentPage != -1) {
        m_frameIndex = currentPage;

        // check if pixmap exists or else request it
        PresentationFrame *frame = m_frames[m_frameIndex];
        int pixW = frame->geometry.width();
        int pixH = frame->geometry.height();

        bool signalsBlocked = m_pagesEdit->signalsBlocked();
        m_pagesEdit->blockSignals(true);
        m_pagesEdit->setText(QString::number(m_frameIndex + 1));
        m_pagesEdit->blockSignals(signalsBlocked);

        // if pixmap not inside the Okular::Page we request it and wait for
        // notifyPixmapChanged call or else we can proceed to pixmap generation
        if (!frame->page->hasPixmap(this, ceil(pixW * devicePixelRatioF()), ceil(pixH * devicePixelRatioF()))) {
            requestPixmaps();
        } else {
            // make the background pixmap
            generatePage();
        }

        // perform the page opening action, if any
        if (m_document->page(m_frameIndex)->pageAction(Okular::Page::Opening))
            m_document->processAction(m_document->page(m_frameIndex)->pageAction(Okular::Page::Opening));

        // perform the additional actions of the page's annotations, if any
        const QLinkedList<Okular::Annotation *> annotationsList = m_document->page(m_frameIndex)->annotations();
        for (const Okular::Annotation *annotation : annotationsList) {
            Okular::Action *action = nullptr;

            if (annotation->subType() == Okular::Annotation::AScreen)
                action = static_cast<const Okular::ScreenAnnotation *>(annotation)->additionalAction(Okular::Annotation::PageOpening);
            else if (annotation->subType() == Okular::Annotation::AWidget)
                action = static_cast<const Okular::WidgetAnnotation *>(annotation)->additionalAction(Okular::Annotation::PageOpening);

            if (action)
                m_document->processAction(action);
        }

        // start autoplay video playback
        for (VideoWidget *vw : qAsConst(m_frames[m_frameIndex]->videoWidgets)) {
            vw->pageEntered();
        }
    }
}

bool PresentationWidget::canUnloadPixmap(int pageNumber) const
{
    if (Okular::SettingsCore::memoryLevel() == Okular::SettingsCore::EnumMemoryLevel::Low || Okular::SettingsCore::memoryLevel() == Okular::SettingsCore::EnumMemoryLevel::Normal) {
        // can unload all pixmaps except for the currently visible one
        return pageNumber != m_frameIndex;
    } else {
        // can unload all pixmaps except for the currently visible one, previous and next
        return qAbs(pageNumber - m_frameIndex) <= 1;
    }
}

void PresentationWidget::setupActions()
{
    addAction(m_ac->action(QStringLiteral("first_page")));
    addAction(m_ac->action(QStringLiteral("last_page")));
    addAction(m_ac->action(QString::fromLocal8Bit(KStandardAction::name(KStandardAction::Prior))));
    addAction(m_ac->action(QString::fromLocal8Bit(KStandardAction::name(KStandardAction::Next))));
    addAction(m_ac->action(QString::fromLocal8Bit(KStandardAction::name(KStandardAction::DocumentBack))));
    addAction(m_ac->action(QString::fromLocal8Bit(KStandardAction::name(KStandardAction::DocumentForward))));

    QAction *action = m_ac->action(QStringLiteral("switch_blackscreen_mode"));
    connect(action, &QAction::toggled, this, &PresentationWidget::toggleBlackScreenMode);
    action->setEnabled(true);
    addAction(action);
}

void PresentationWidget::setPlayPauseIcon()
{
    QAction *playPauseAction = m_ac->action(QStringLiteral("presentation_play_pause"));
    if (m_nextPageTimer->isActive()) {
        playPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
        playPauseAction->setToolTip(i18nc("For Presentation", "Pause"));
    } else {
        playPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
        playPauseAction->setToolTip(i18nc("For Presentation", "Play"));
    }
}

bool PresentationWidget::eventFilter(QObject *o, QEvent *e)
{
    if (o == qApp) {
        if (e->type() == QTabletEvent::TabletEnterProximity) {
            setCursor(QCursor(Qt::CrossCursor));
        } else if (e->type() == QTabletEvent::TabletLeaveProximity) {
            setCursor(QCursor(Okular::Settings::slidesCursor() == Okular::Settings::EnumSlidesCursor::Hidden ? Qt::BlankCursor : Qt::ArrowCursor));
            if (Okular::Settings::slidesCursor() == Okular::Settings::EnumSlidesCursor::HiddenDelay) {
                // Trick KCursor to hide the cursor if needed by sending an "unknown" key press event
                // Send also the key release to make the world happy even it's probably not needed
                QKeyEvent kp(QEvent::KeyPress, 0, Qt::NoModifier);
                qApp->sendEvent(this, &kp);
                QKeyEvent kr(QEvent::KeyRelease, 0, Qt::NoModifier);
                qApp->sendEvent(this, &kr);
            }
        }
    }
    return false;
}

// <widget events>
bool PresentationWidget::event(QEvent *e)
{
    if (e->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent *>(e));

    if (e->type() == QEvent::ToolTip) {
        QHelpEvent *he = (QHelpEvent *)e;

        QRect r;
        const Okular::Action *link = getLink(he->x(), he->y(), &r);

        if (link) {
            QString tip = link->actionTip();
            if (!tip.isEmpty())
                QToolTip::showText(he->globalPos(), tip, this, r);
        }
        e->accept();
        return true;
    } else
        // do not stop the event
        return QWidget::event(e);
}

bool PresentationWidget::gestureEvent(QGestureEvent *event)
{
    // Swiping left or right on a touch screen will go to the previous or next slide, respectively.
    // The precise gesture is the standard Qt swipe: with three(!) fingers.
    if (QGesture *swipe = event->gesture(Qt::SwipeGesture)) {
        QSwipeGesture *swipeEvent = static_cast<QSwipeGesture *>(swipe);

        if (swipeEvent->state() == Qt::GestureFinished) {
            if (swipeEvent->horizontalDirection() == QSwipeGesture::Left) {
                slotPrevPage();
                event->accept();
                return true;
            }
            if (swipeEvent->horizontalDirection() == QSwipeGesture::Right) {
                slotNextPage();
                event->accept();
                return true;
            }
        }
    }

    return false;
}
void PresentationWidget::keyPressEvent(QKeyEvent *e)
{
    if (!m_isSetup)
        return;

    switch (e->key()) {
    case Qt::Key_Left:
    case Qt::Key_Backspace:
    case Qt::Key_PageUp:
    case Qt::Key_Up:
        slotPrevPage();
        break;
    case Qt::Key_Right:
    case Qt::Key_Space:
    case Qt::Key_PageDown:
    case Qt::Key_Down:
        slotNextPage();
        break;
    case Qt::Key_Home:
        slotFirstPage();
        break;
    case Qt::Key_End:
        slotLastPage();
        break;
    case Qt::Key_Escape:
        if (!m_topBar->isHidden())
            showTopBar(false);
        else
            close();
        break;
    }
}

void PresentationWidget::wheelEvent(QWheelEvent *e)
{
    if (!m_isSetup)
        return;

    // performance note: don't remove the clipping
    int div = e->angleDelta().y() / 120;
    if (div > 0) {
        if (div > 3)
            div = 3;
        while (div--)
            slotPrevPage();
    } else if (div < 0) {
        if (div < -3)
            div = -3;
        while (div++)
            slotNextPage();
    }
}

void PresentationWidget::mousePressEvent(QMouseEvent *e)
{
    if (!m_isSetup)
        return;

    if (m_drawingEngine) {
        QRect r = routeMouseDrawingEvent(e);
        if (r.isValid()) {
            m_drawingRect |= r.translated(m_frames[m_frameIndex]->geometry.topLeft());
            update(m_drawingRect);
        }
        return;
    }

    // pressing left button
    if (e->button() == Qt::LeftButton) {
        // if pressing on a link, skip other checks
        if ((m_pressedLink = getLink(e->x(), e->y())))
            return;

        const Okular::Annotation *annotation = getAnnotation(e->x(), e->y());
        if (annotation) {
            if (annotation->subType() == Okular::Annotation::AMovie) {
                const Okular::MovieAnnotation *movieAnnotation = static_cast<const Okular::MovieAnnotation *>(annotation);

                VideoWidget *vw = m_frames[m_frameIndex]->videoWidgets.value(movieAnnotation->movie());
                vw->show();
                vw->play();
                return;
            } else if (annotation->subType() == Okular::Annotation::ARichMedia) {
                const Okular::RichMediaAnnotation *richMediaAnnotation = static_cast<const Okular::RichMediaAnnotation *>(annotation);

                VideoWidget *vw = m_frames[m_frameIndex]->videoWidgets.value(richMediaAnnotation->movie());
                vw->show();
                vw->play();
                return;
            } else if (annotation->subType() == Okular::Annotation::AScreen) {
                m_document->processAction(static_cast<const Okular::ScreenAnnotation *>(annotation)->action());
                return;
            }
        }

        // handle clicking on top-right overlay
        if (!(Okular::Settings::slidesCursor() == Okular::Settings::EnumSlidesCursor::Hidden) && m_overlayGeometry.contains(e->pos())) {
            overlayClick(e->pos());
            return;
        }

        // Actual mouse press events always lead to the next page page
        if (e->source() == Qt::MouseEventNotSynthesized) {
            m_goToNextPageOnRelease = true;
        }
        // Touch events may lead to the previous or next page
        else if (Okular::Settings::slidesTapNavigation() != Okular::Settings::EnumSlidesTapNavigation::Disabled) {
            switch (Okular::Settings::slidesTapNavigation()) {
            case Okular::Settings::EnumSlidesTapNavigation::ForwardBackward: {
                if (e->x() < (geometry().width() / 2)) {
                    m_goToPreviousPageOnRelease = true;
                } else {
                    m_goToNextPageOnRelease = true;
                }
                break;
            }
            case Okular::Settings::EnumSlidesTapNavigation::Forward: {
                m_goToNextPageOnRelease = true;
                break;
            }
            case Okular::Settings::EnumSlidesTapNavigation::Disabled: {
                // Do Nothing
            }
            }
        }
    }
    // pressing forward button
    else if (e->button() == Qt::ForwardButton) {
        m_goToNextPageOnRelease = true;
    }
    // pressing right or backward button
    else if (e->button() == Qt::RightButton || e->button() == Qt::BackButton)
        m_goToPreviousPageOnRelease = true;
}

void PresentationWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_drawingEngine) {
        routeMouseDrawingEvent(e);
        return;
    }

    // if releasing on the same link we pressed over, execute it
    if (m_pressedLink && e->button() == Qt::LeftButton) {
        const Okular::Action *link = getLink(e->x(), e->y());
        if (link == m_pressedLink)
            m_document->processAction(link);
        m_pressedLink = nullptr;
    }

    if (m_goToPreviousPageOnRelease) {
        slotPrevPage();
        m_goToPreviousPageOnRelease = false;
    }

    if (m_goToNextPageOnRelease) {
        slotNextPage();
        m_goToNextPageOnRelease = false;
    }
}

void PresentationWidget::mouseMoveEvent(QMouseEvent *e)
{
    // safety check
    if (!m_isSetup)
        return;

    // update cursor and tooltip if hovering a link
    if (!m_drawingEngine && Okular::Settings::slidesCursor() != Okular::Settings::EnumSlidesCursor::Hidden)
        testCursorOnLink(e->x(), e->y());

    if (!m_topBar->isHidden()) {
        // hide a shown bar when exiting the area
        if (e->y() > (m_topBar->height() + 1)) {
            showTopBar(false);
            setFocus(Qt::OtherFocusReason);
        }
    } else {
        if (m_drawingEngine && e->buttons() != Qt::NoButton) {
            QRect r = routeMouseDrawingEvent(e);
            if (r.isValid()) {
                m_drawingRect |= r.translated(m_frames[m_frameIndex]->geometry.topLeft());
                update(m_drawingRect);
            }
        } else {
            // show the bar if reaching top 2 pixels
            if (e->y() <= 1)
                showTopBar(true);
            // handle "dragging the wheel" if clicking on its geometry
            else if ((QApplication::mouseButtons() & Qt::LeftButton) && m_overlayGeometry.contains(e->pos()))
                overlayClick(e->pos());
        }
    }
}

void PresentationWidget::paintEvent(QPaintEvent *pe)
{
    qreal dpr = devicePixelRatioF();

    if (m_inBlackScreenMode) {
        QPainter painter(this);
        painter.fillRect(pe->rect(), Qt::black);
        return;
    }

    if (!m_isSetup) {
        m_width = width();
        m_height = height();

        connect(m_document, &Okular::Document::linkFind, this, &PresentationWidget::slotFind);

        // register this observer in document. events will come immediately
        m_document->addObserver(this);

        // show summary if requested
        if (Okular::Settings::slidesShowSummary())
            generatePage();
    }

    // check painting rect consistency
    QRect r = pe->rect().intersected(QRect(QPoint(0, 0), geometry().size()));
    if (r.isNull())
        return;

    if (m_lastRenderedPixmap.isNull()) {
        QPainter painter(this);
        painter.fillRect(pe->rect(), Okular::Settings::slidesBackgroundColor());
        return;
    }

    // blit the pixmap to the screen
    QPainter painter(this);
    for (const QRect &r : pe->region()) {
        if (!r.isValid())
            continue;
#ifdef ENABLE_PROGRESS_OVERLAY
        const QRect dR(QRectF(r.x() * dpr, r.y() * dpr, r.width() * dpr, r.height() * dpr).toAlignedRect());
        if (Okular::Settings::slidesShowProgress() && r.intersects(m_overlayGeometry)) {
            // backbuffer the overlay operation
            QPixmap backPixmap(dR.size());
            backPixmap.setDevicePixelRatio(dpr);
            QPainter pixPainter(&backPixmap);

            // first draw the background on the backbuffer
            pixPainter.drawPixmap(QPoint(0, 0), m_lastRenderedPixmap, dR);

            // then blend the overlay (a piece of) over the background
            QRect ovr = m_overlayGeometry.intersected(r);
            pixPainter.drawPixmap((ovr.left() - r.left()), (ovr.top() - r.top()), m_lastRenderedOverlay, (ovr.left() - m_overlayGeometry.left()) * dpr, (ovr.top() - m_overlayGeometry.top()) * dpr, ovr.width() * dpr, ovr.height() * dpr);

            // finally blit the pixmap to the screen
            pixPainter.end();
            const QRect backPixmapRect = backPixmap.rect();
            const QRect dBackPixmapRect(QRectF(backPixmapRect.x() * dpr, backPixmapRect.y() * dpr, backPixmapRect.width() * dpr, backPixmapRect.height() * dpr).toAlignedRect());
            painter.drawPixmap(r.topLeft(), backPixmap, dBackPixmapRect);
        } else
#endif
            // copy the rendered pixmap to the screen
            painter.drawPixmap(r.topLeft(), m_lastRenderedPixmap, dR);
    }

    // paint drawings
    if (m_frameIndex != -1) {
        painter.save();

        const QRect &geom = m_frames[m_frameIndex]->geometry;

        const QSize pmSize(geom.width() * dpr, geom.height() * dpr);
        QPixmap pm(pmSize);
        pm.fill(Qt::transparent);
        QPainter pmPainter(&pm);

        pm.setDevicePixelRatio(dpr);
        pmPainter.setRenderHints(QPainter::Antialiasing);

        // Paint old paths
        for (const SmoothPath &drawing : qAsConst(m_frames[m_frameIndex]->drawings)) {
            drawing.paint(&pmPainter, pmSize.width(), pmSize.height());
        }

        // Paint the path that is currently being drawn by the user
        if (m_drawingEngine && m_drawingRect.intersects(pe->rect()))
            m_drawingEngine->paint(&pmPainter, pmSize.width(), pmSize.height(), m_drawingRect.intersected(pe->rect()));

        painter.setRenderHints(QPainter::Antialiasing);
        painter.drawPixmap(geom.topLeft(), pm);

        painter.restore();
    }
    painter.end();
}

void PresentationWidget::resizeEvent(QResizeEvent *re)
{
    m_width = width();
    m_height = height();

    // if by chance the new size equals the old, do not invalidate pixmaps and such..
    if (size() == re->oldSize())
        return;

    // BEGIN Top toolbar
    // tool bar height in pixels, make it large enough to hold the text fields with the page numbers
    const int toolBarHeight = m_pagesEdit->height() * 1.5;

    m_topBar->setGeometry(0, 0, width(), toolBarHeight);
    m_topBar->setIconSize(QSize(toolBarHeight * 0.75, toolBarHeight * 0.75));
    // END Top toolbar

    // BEGIN Content area
    // update the frames
    const float screenRatio = (float)m_height / (float)m_width;
    for (PresentationFrame *frame : qAsConst(m_frames)) {
        frame->recalcGeometry(m_width, m_height, screenRatio);
    }

    if (m_frameIndex != -1) {
        // ugliness alarm!
        const_cast<Okular::Page *>(m_frames[m_frameIndex]->page)->deletePixmap(this);
        // force the regeneration of the pixmap
        m_lastRenderedPixmap = QPixmap();
        m_blockNotifications = true;
        requestPixmaps();
        m_blockNotifications = false;
    }

    if (m_transitionTimer->isActive()) {
        m_transitionTimer->stop();
    }

    generatePage(true /* no transitions */);
    // END Content area
}

void PresentationWidget::leaveEvent(QEvent *e)
{
    Q_UNUSED(e)

    if (!m_topBar->isHidden()) {
        showTopBar(false);
    }
}
// </widget events>

const void *PresentationWidget::getObjectRect(Okular::ObjectRect::ObjectType type, int x, int y, QRect *geometry) const
{
    // no links on invalid pages
    if (geometry && !geometry->isNull())
        geometry->setRect(0, 0, 0, 0);
    if (m_frameIndex < 0 || m_frameIndex >= (int)m_frames.size())
        return nullptr;

    // get frame, page and geometry
    const PresentationFrame *frame = m_frames[m_frameIndex];
    const Okular::Page *page = frame->page;
    const QRect &frameGeometry = frame->geometry;

    // compute normalized x and y
    double nx = (double)(x - frameGeometry.left()) / (double)frameGeometry.width();
    double ny = (double)(y - frameGeometry.top()) / (double)frameGeometry.height();

    // no links outside the pages
    if (nx < 0 || nx > 1 || ny < 0 || ny > 1)
        return nullptr;

    // check if 1) there is an object and 2) it's a link
    const QRect screenRect = oldQt_screenOf(this)->geometry();
    const Okular::ObjectRect *object = page->objectRect(type, nx, ny, screenRect.width(), screenRect.height());
    if (!object)
        return nullptr;

    // compute link geometry if destination rect present
    if (geometry) {
        *geometry = object->boundingRect(frameGeometry.width(), frameGeometry.height());
        geometry->translate(frameGeometry.left(), frameGeometry.top());
    }

    // return the link pointer
    return object->object();
}

const Okular::Action *PresentationWidget::getLink(int x, int y, QRect *geometry) const
{
    return reinterpret_cast<const Okular::Action *>(getObjectRect(Okular::ObjectRect::Action, x, y, geometry));
}

const Okular::Annotation *PresentationWidget::getAnnotation(int x, int y, QRect *geometry) const
{
    return reinterpret_cast<const Okular::Annotation *>(getObjectRect(Okular::ObjectRect::OAnnotation, x, y, geometry));
}

void PresentationWidget::testCursorOnLink(int x, int y)
{
    const Okular::Action *link = getLink(x, y, nullptr);
    const Okular::Annotation *annotation = getAnnotation(x, y, nullptr);

    const bool needsHandCursor = ((link != nullptr) || ((annotation != nullptr) && (annotation->subType() == Okular::Annotation::AMovie)) || ((annotation != nullptr) && (annotation->subType() == Okular::Annotation::ARichMedia)) ||
                                  ((annotation != nullptr) && (annotation->subType() == Okular::Annotation::AScreen) && (GuiUtils::renditionMovieFromScreenAnnotation(static_cast<const Okular::ScreenAnnotation *>(annotation)) != nullptr)));

    // only react on changes (in/out from a link)
    if ((needsHandCursor && !m_handCursor) || (!needsHandCursor && m_handCursor)) {
        // change cursor shape
        m_handCursor = needsHandCursor;
        setCursor(QCursor(m_handCursor ? Qt::PointingHandCursor : Qt::ArrowCursor));
    }
}

void PresentationWidget::overlayClick(const QPoint position)
{
    // clicking the progress indicator
    int xPos = position.x() - m_overlayGeometry.x() - m_overlayGeometry.width() / 2, yPos = m_overlayGeometry.height() / 2 - position.y();
    if (!xPos && !yPos)
        return;

    // compute angle relative to indicator (note coord transformation)
    float angle = 0.5 + 0.5 * atan2((double)-xPos, (double)-yPos) / M_PI;
    int pageIndex = (int)(angle * (m_frames.count() - 1) + 0.5);

    // go to selected page
    changePage(pageIndex);
}

void PresentationWidget::changePage(int newPage)
{
    if (m_showSummaryView) {
        m_showSummaryView = false;
        m_frameIndex = -1;
        return;
    }

    if (m_frameIndex == newPage)
        return;

    // switch to newPage
    m_document->setViewportPage(newPage, this);

    if ((Okular::Settings::slidesShowSummary() && !m_showSummaryView) || m_frameIndex == -1)
        notifyCurrentPageChanged(-1, newPage);
}

void PresentationWidget::generatePage(bool disableTransition)
{
    if (m_lastRenderedPixmap.isNull()) {
        qreal dpr = devicePixelRatioF();
        m_lastRenderedPixmap = QPixmap(m_width * dpr, m_height * dpr);
        m_lastRenderedPixmap.setDevicePixelRatio(dpr);

        m_previousPagePixmap = QPixmap();
    } else {
        m_previousPagePixmap = m_lastRenderedPixmap;
    }

    // opens the painter over the pixmap
    QPainter pixmapPainter;
    pixmapPainter.begin(&m_lastRenderedPixmap);
    // generate welcome page
    if (m_frameIndex == -1)
        generateIntroPage(pixmapPainter);
    // generate a normal pixmap with extended margin filling
    if (m_frameIndex >= 0 && m_frameIndex < (int)m_document->pages())
        generateContentsPage(m_frameIndex, pixmapPainter);
    pixmapPainter.end();

    // generate the top-right corner overlay
#ifdef ENABLE_PROGRESS_OVERLAY
    if (Okular::Settings::slidesShowProgress() && m_frameIndex != -1)
        generateOverlay();
#endif

    // start transition on pages that have one
    disableTransition |= (Okular::Settings::slidesTransition() == Okular::Settings::EnumSlidesTransition::NoTransitions);
    if (!disableTransition) {
        const Okular::PageTransition *transition = m_frameIndex != -1 ? m_frames[m_frameIndex]->page->transition() : nullptr;
        if (transition)
            initTransition(transition);
        else {
            Okular::PageTransition trans = defaultTransition();
            initTransition(&trans);
        }
    } else {
        Okular::PageTransition trans = defaultTransition(Okular::Settings::EnumSlidesTransition::Replace);
        initTransition(&trans);
    }

    // update cursor + tooltip
    if (!m_drawingEngine && Okular::Settings::slidesCursor() != Okular::Settings::EnumSlidesCursor::Hidden) {
        QPoint p = mapFromGlobal(QCursor::pos());
        testCursorOnLink(p.x(), p.y());
    }
}

void PresentationWidget::generateIntroPage(QPainter &p)
{
    qreal dpr = devicePixelRatioF();

    // use a vertical gray gradient background
    int blend1 = m_height / 10, blend2 = 9 * m_height / 10;
    int baseTint = QColor(Qt::gray).red();
    for (int i = 0; i < m_height; i++) {
        int k = baseTint;
        if (i < blend1)
            k -= (int)(baseTint * (i - blend1) * (i - blend1) / (float)(blend1 * blend1));
        if (i > blend2)
            k += (int)((255 - baseTint) * (i - blend2) * (i - blend2) / (float)(blend1 * blend1));
        p.fillRect(0, i, m_width, 1, QColor(k, k, k));
    }

    // draw okular logo in the four corners
    QPixmap logo = QIcon::fromTheme(QStringLiteral("okular")).pixmap(64 * dpr);
    logo.setDevicePixelRatio(dpr);
    if (!logo.isNull()) {
        p.drawPixmap(5, 5, logo);
        p.drawPixmap(m_width - 5 - logo.width(), 5, logo);
        p.drawPixmap(m_width - 5 - logo.width(), m_height - 5 - logo.height(), logo);
        p.drawPixmap(5, m_height - 5 - logo.height(), logo);
    }

    // draw metadata text (the last line is 'click to begin')
    int strNum = m_metaStrings.count(), strHeight = m_height / (strNum + 4), fontHeight = 2 * strHeight / 3;
    QFont font(p.font());
    font.setPixelSize(fontHeight);
    QFontMetrics metrics(font);
    for (int i = 0; i < strNum; i++) {
        // set a font to fit text width
        float wScale = (float)metrics.boundingRect(m_metaStrings[i]).width() / (float)m_width;
        QFont f(font);
        if (wScale > 1.0)
            f.setPixelSize((int)((float)fontHeight / (float)wScale));
        p.setFont(f);

        // text shadow
        p.setPen(Qt::darkGray);
        p.drawText(2, m_height / 4 + strHeight * i + 2, m_width, strHeight, Qt::AlignHCenter | Qt::AlignVCenter, m_metaStrings[i]);
        // text body
        p.setPen(128 + (127 * i) / strNum);
        p.drawText(0, m_height / 4 + strHeight * i, m_width, strHeight, Qt::AlignHCenter | Qt::AlignVCenter, m_metaStrings[i]);
    }
}

void PresentationWidget::generateContentsPage(int pageNum, QPainter &p)
{
    PresentationFrame *frame = m_frames[pageNum];

    // translate painter and contents rect
    QRect geom(frame->geometry);
    p.translate(geom.left(), geom.top());
    geom.translate(-geom.left(), -geom.top());

    // draw the page using the shared PagePainter class
    int flags = PagePainter::Accessibility | PagePainter::Highlights | PagePainter::Annotations;

    PagePainter::paintPageOnPainter(&p, frame->page, this, flags, geom.width(), geom.height(), geom);

    // restore painter
    p.translate(-frame->geometry.left(), -frame->geometry.top());

    // fill unpainted areas with background color
    const QRegion unpainted(QRect(0, 0, m_width, m_height));
    const QRegion rgn = unpainted.subtracted(frame->geometry);
    for (const QRect &r : rgn) {
        p.fillRect(r, Okular::Settings::slidesBackgroundColor());
    }
}

// from Arthur - Qt4 - (is defined elsewhere as 'qt_div_255' to not break final compilation)
inline int qt_div255(int x)
{
    return (x + (x >> 8) + 0x80) >> 8;
}
void PresentationWidget::generateOverlay()
{
#ifdef ENABLE_PROGRESS_OVERLAY
    qreal dpr = devicePixelRatioF();

    // calculate overlay geometry and resize pixmap if needed
    double side = m_width / 16.0;
    m_overlayGeometry.setRect(m_width - side - 4, 4, side, side);

    // note: to get a sort of antialiasing, we render the pixmap double sized
    // and the resulting image is smoothly scaled down. So here we open a
    // painter on the double sized pixmap.
    side *= 2;

    QPixmap doublePixmap(side * dpr, side * dpr);
    doublePixmap.setDevicePixelRatio(dpr);
    doublePixmap.fill(Qt::black);
    QPainter pixmapPainter(&doublePixmap);
    pixmapPainter.setRenderHints(QPainter::Antialiasing);

    // draw PIE SLICES in blue levels (the levels will then be the alpha component)
    int pages = m_document->pages();
    if (pages > 28) { // draw continuous slices
        int degrees = (int)(360 * (float)(m_frameIndex + 1) / (float)pages);
        pixmapPainter.setPen(0x05);
        pixmapPainter.setBrush(QColor(0x40));
        pixmapPainter.drawPie(2, 2, side - 4, side - 4, 90 * 16, (360 - degrees) * 16);
        pixmapPainter.setPen(0x40);
        pixmapPainter.setBrush(QColor(0xF0));
        pixmapPainter.drawPie(2, 2, side - 4, side - 4, 90 * 16, -degrees * 16);
    } else { // draw discrete slices
        float oldCoord = -90;
        for (int i = 0; i < pages; i++) {
            float newCoord = -90 + 360 * (float)(i + 1) / (float)pages;
            pixmapPainter.setPen(i <= m_frameIndex ? 0x40 : 0x05);
            pixmapPainter.setBrush(QColor(i <= m_frameIndex ? 0xF0 : 0x40));
            pixmapPainter.drawPie(2, 2, side - 4, side - 4, (int)(-16 * (oldCoord + 1)), (int)(-16 * (newCoord - (oldCoord + 2))));
            oldCoord = newCoord;
        }
    }
    int circleOut = side / 4;
    pixmapPainter.setPen(Qt::black);
    pixmapPainter.setBrush(Qt::black);
    pixmapPainter.drawEllipse(circleOut, circleOut, side - 2 * circleOut, side - 2 * circleOut);

    // draw TEXT using maximum opacity
    QFont f(pixmapPainter.font());
    f.setPixelSize(side / 4);
    pixmapPainter.setFont(f);
    pixmapPainter.setPen(0xFF);
    // use a little offset to prettify output
    pixmapPainter.drawText(2, 2, side, side, Qt::AlignCenter, QString::number(m_frameIndex + 1));

    // end drawing pixmap and halve image
    pixmapPainter.end();
    QImage image(doublePixmap.toImage().scaled((side / 2) * dpr, (side / 2) * dpr, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    image.setDevicePixelRatio(dpr);
    image = image.convertToFormat(QImage::Format_ARGB32);
    image.setDevicePixelRatio(dpr);

    // draw circular shadow using the same technique
    doublePixmap.fill(Qt::black);
    pixmapPainter.begin(&doublePixmap);
    pixmapPainter.setPen(0x40);
    pixmapPainter.setBrush(QColor(0x80));
    pixmapPainter.drawEllipse(0, 0, side, side);
    pixmapPainter.end();
    QImage shadow(doublePixmap.toImage().scaled((side / 2) * dpr, (side / 2) * dpr, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    shadow.setDevicePixelRatio(dpr);

    // generate a 2 colors pixmap using mixing shadow (made with highlight color)
    // and image (made with highlightedText color)
    QPalette pal = palette();
    QColor color = pal.color(QPalette::Active, QPalette::HighlightedText);
    int red = color.red(), green = color.green(), blue = color.blue();
    color = pal.color(QPalette::Active, QPalette::Highlight);
    int sRed = color.red(), sGreen = color.green(), sBlue = color.blue();
    // pointers
    unsigned int *data = reinterpret_cast<unsigned int *>(image.bits()), *shadowData = reinterpret_cast<unsigned int *>(shadow.bits()), pixels = image.width() * image.height();
    // cache data (reduce computation time to 26%!)
    int c1 = -1, c2 = -1, cR = 0, cG = 0, cB = 0, cA = 0;
    // foreach pixel
    for (unsigned int i = 0; i < pixels; ++i) {
        // alpha for shadow and image
        int shadowAlpha = shadowData[i] & 0xFF, srcAlpha = data[i] & 0xFF;
        // cache values
        if (srcAlpha != c1 || shadowAlpha != c2) {
            c1 = srcAlpha;
            c2 = shadowAlpha;
            // fuse color components and alpha value of image over shadow
            data[i] = qRgba(cR = qt_div255(srcAlpha * red + (255 - srcAlpha) * sRed),
                            cG = qt_div255(srcAlpha * green + (255 - srcAlpha) * sGreen),
                            cB = qt_div255(srcAlpha * blue + (255 - srcAlpha) * sBlue),
                            cA = qt_div255(srcAlpha * srcAlpha + (255 - srcAlpha) * shadowAlpha));
        } else
            data[i] = qRgba(cR, cG, cB, cA);
    }
    m_lastRenderedOverlay = QPixmap::fromImage(image);
    m_lastRenderedOverlay.setDevicePixelRatio(dpr);

    // start the autohide timer
    // repaint( m_overlayGeometry ); // toggle with next line
    update(m_overlayGeometry);
    m_overlayHideTimer->start(2500);
#endif
}

QRect PresentationWidget::routeMouseDrawingEvent(QMouseEvent *e)
{
    if (m_frameIndex == -1) // Can't draw on the summary page
        return QRect();

    const QRect &geom = m_frames[m_frameIndex]->geometry;
    const Okular::Page *page = m_frames[m_frameIndex]->page;

    AnnotatorEngine::EventType eventType;
    AnnotatorEngine::Button button;
    AnnotatorEngine::Modifiers modifiers;

    // figure out the event type and button
    AnnotatorEngine::decodeEvent(e, &eventType, &button);

    static bool hasclicked = false;
    if (eventType == AnnotatorEngine::Press)
        hasclicked = true;

    QPointF mousePos = e->localPos();
    double nX = (mousePos.x() - (double)geom.left()) / (double)geom.width();
    double nY = (mousePos.y() - (double)geom.top()) / (double)geom.height();
    QRect ret;
    bool isInside = nX >= 0 && nX < 1 && nY >= 0 && nY < 1;

    if (hasclicked && !isInside) {
        // Fake a move to the last border pos
        nX = qBound(0., nX, 1.);
        nY = qBound(0., nY, 1.);
        m_drawingEngine->event(AnnotatorEngine::Move, button, modifiers, nX, nY, geom.width(), geom.height(), page);

        // Fake a release in the following lines
        eventType = AnnotatorEngine::Release;
        isInside = true;
    } else if (!hasclicked && isInside) {
        // we're coming from the outside, pretend we started clicking at the closest border
        if (nX < (1 - nX) && nX < nY && nX < (1 - nY))
            nX = 0;
        else if (nY < (1 - nY) && nY < nX && nY < (1 - nX))
            nY = 0;
        else if ((1 - nX) < nX && (1 - nX) < nY && (1 - nX) < (1 - nY))
            nX = 1;
        else
            nY = 1;

        hasclicked = true;
        eventType = AnnotatorEngine::Press;
    }

    if (hasclicked && isInside) {
        ret = m_drawingEngine->event(eventType, button, modifiers, nX, nY, geom.width(), geom.height(), page);
    }

    if (eventType == AnnotatorEngine::Release) {
        hasclicked = false;
    }

    if (m_drawingEngine->creationCompleted()) {
        // add drawing to current page
        m_frames[m_frameIndex]->drawings << m_drawingEngine->endSmoothPath();

        // remove the actual drawer and create a new one just after
        // that - that gives continuous drawing
        delete m_drawingEngine;
        m_drawingRect = QRect();
        m_drawingEngine = new SmoothPathEngine(m_currentDrawingToolElement);

        // schedule repaint
        update();
    }

    return ret;
}

void PresentationWidget::startAutoChangeTimer()
{
    double pageDuration = m_frameIndex >= 0 && m_frameIndex < (int)m_frames.count() ? m_frames[m_frameIndex]->page->duration() : -1;
    if (m_advanceSlides || pageDuration >= 0.0) {
        double secs;
        if (pageDuration < 0.0)
            secs = Okular::SettingsCore::slidesAdvanceTime();
        else if (m_advanceSlides)
            secs = qMin<double>(pageDuration, Okular::SettingsCore::slidesAdvanceTime());
        else
            secs = pageDuration;

        m_nextPageTimer->start((int)(secs * 1000));
    }
    setPlayPauseIcon();
}

QScreen *PresentationWidget::defaultScreen() const
{
    const int preferenceScreen = Okular::Settings::slidesScreen();

    if (preferenceScreen == -2) {
        return oldQt_screenOf(m_parentWidget);
    } else if (preferenceScreen == -1) {
        return QApplication::primaryScreen();
    } else if (preferenceScreen >= 0 && preferenceScreen < QApplication::screens().count()) {
        return QApplication::screens().at(preferenceScreen);
    } else {
        return oldQt_screenOf(m_parentWidget);
    }
}

void PresentationWidget::requestPixmaps()
{
    const qreal dpr = devicePixelRatioF();
    PresentationFrame *frame = m_frames[m_frameIndex];
    int pixW = frame->geometry.width();
    int pixH = frame->geometry.height();

    // operation will take long: set busy cursor
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    // request the pixmap
    QLinkedList<Okular::PixmapRequest *> requests;
    requests.push_back(new Okular::PixmapRequest(this, m_frameIndex, pixW, pixH, dpr, PRESENTATION_PRIO, Okular::PixmapRequest::NoFeature));
    // restore cursor
    QApplication::restoreOverrideCursor();
    // ask for next and previous page if not in low memory usage setting
    if (Okular::SettingsCore::memoryLevel() != Okular::SettingsCore::EnumMemoryLevel::Low) {
        int pagesToPreload = 1;

        // If greedy, preload everything
        if (Okular::SettingsCore::memoryLevel() == Okular::SettingsCore::EnumMemoryLevel::Greedy)
            pagesToPreload = (int)m_document->pages();

        Okular::PixmapRequest::PixmapRequestFeatures requestFeatures = Okular::PixmapRequest::Preload;
        requestFeatures |= Okular::PixmapRequest::Asynchronous;

        for (int j = 1; j <= pagesToPreload; j++) {
            int tailRequest = m_frameIndex + j;
            if (tailRequest < (int)m_document->pages()) {
                PresentationFrame *nextFrame = m_frames[tailRequest];
                pixW = nextFrame->geometry.width();
                pixH = nextFrame->geometry.height();
                if (!nextFrame->page->hasPixmap(this, pixW, pixH))
                    requests.push_back(new Okular::PixmapRequest(this, tailRequest, pixW, pixH, dpr, PRESENTATION_PRELOAD_PRIO, requestFeatures));
            }

            int headRequest = m_frameIndex - j;
            if (headRequest >= 0) {
                PresentationFrame *prevFrame = m_frames[headRequest];
                pixW = prevFrame->geometry.width();
                pixH = prevFrame->geometry.height();
                if (!prevFrame->page->hasPixmap(this, pixW, pixH))
                    requests.push_back(new Okular::PixmapRequest(this, headRequest, pixW, pixH, dpr, PRESENTATION_PRELOAD_PRIO, requestFeatures));
            }

            // stop if we've already reached both ends of the document
            if (headRequest < 0 && tailRequest >= (int)m_document->pages())
                break;
        }
    }
    m_document->requestPixmaps(requests);
}

void PresentationWidget::slotNextPage()
{
    int nextIndex = m_frameIndex + 1;

    // loop when configured
    if (nextIndex == m_frames.count() && Okular::Settings::slidesLoop())
        nextIndex = 0;

    if (nextIndex < m_frames.count()) {
        // go to next page
        changePage(nextIndex);
        // auto advance to the next page if set
        startAutoChangeTimer();
    } else {
#ifdef ENABLE_PROGRESS_OVERLAY
        if (Okular::Settings::slidesShowProgress())
            generateOverlay();
#endif
        if (m_transitionTimer->isActive()) {
            m_transitionTimer->stop();
            m_lastRenderedPixmap = m_currentPagePixmap;
            update();
        }
    }
    // we need the setFocus() call here to let KCursor::autoHide() work correctly
    setFocus();
}

void PresentationWidget::slotPrevPage()
{
    if (m_frameIndex > 0) {
        // go to previous page
        changePage(m_frameIndex - 1);

        // auto advance to the next page if set
        startAutoChangeTimer();
    } else {
#ifdef ENABLE_PROGRESS_OVERLAY
        if (Okular::Settings::slidesShowProgress())
            generateOverlay();
#endif
        if (m_transitionTimer->isActive()) {
            m_transitionTimer->stop();
            m_lastRenderedPixmap = m_currentPagePixmap;
            update();
        }
    }
}

void PresentationWidget::slotFirstPage()
{
    changePage(0);
}

void PresentationWidget::slotLastPage()
{
    changePage((int)m_frames.count() - 1);
}

void PresentationWidget::slotHideOverlay()
{
    QRect geom(m_overlayGeometry);
    m_overlayGeometry.setCoords(0, 0, -1, -1);
    update(geom);
}

void PresentationWidget::slotTransitionStep()
{
    switch (m_currentTransition.type()) {
    case Okular::PageTransition::Fade: {
        QPainter pixmapPainter;
        m_currentPixmapOpacity += 1.0 / m_transitionSteps;
        m_lastRenderedPixmap = QPixmap(m_lastRenderedPixmap.size());
        m_lastRenderedPixmap.setDevicePixelRatio(devicePixelRatioF());
        m_lastRenderedPixmap.fill(Qt::transparent);
        pixmapPainter.begin(&m_lastRenderedPixmap);
        pixmapPainter.setCompositionMode(QPainter::CompositionMode_Source);
        pixmapPainter.setOpacity(1 - m_currentPixmapOpacity);
        pixmapPainter.drawPixmap(0, 0, m_previousPagePixmap);
        pixmapPainter.setOpacity(m_currentPixmapOpacity);
        pixmapPainter.drawPixmap(0, 0, m_currentPagePixmap);
        update();
        if (m_currentPixmapOpacity >= 1)
            return;
    } break;
    default: {
        if (m_transitionRects.empty()) {
            // it's better to fix the transition to cover the whole screen than
            // enabling the following line that wastes cpu for nothing
            // update();
            return;
        }

        for (int i = 0; i < m_transitionMul && !m_transitionRects.empty(); i++) {
            update(m_transitionRects.first());
            m_transitionRects.pop_front();
        }
    } break;
    }
    m_transitionTimer->start(m_transitionDelay);
}

void PresentationWidget::slotDelayedEvents()
{
    setScreen(defaultScreen());
    show();

    if (m_screenSelect) {
        m_screenSelect->setCurrentItem(QApplication::screens().indexOf(oldQt_screenOf(this)));
        connect(m_screenSelect->selectableActionGroup(), &QActionGroup::triggered, this, &PresentationWidget::chooseScreen);
    }

    // inform user on how to exit from presentation mode
    KMessageBox::information(
        this,
        i18n("There are two ways of exiting presentation mode, you can press either ESC key or click with the quit button that appears when placing the mouse in the top-right corner. Of course you can cycle windows (Alt+TAB by default)"),
        QString(),
        QStringLiteral("presentationInfo"));
}

void PresentationWidget::slotPageChanged()
{
    bool ok = true;
    int p = m_pagesEdit->text().toInt(&ok);
    if (!ok)
        return;

    changePage(p - 1);
}

void PresentationWidget::slotChangeDrawingToolEngine(const QDomElement &element)
{
    if (element.isNull()) {
        delete m_drawingEngine;
        m_drawingEngine = nullptr;
        m_drawingRect = QRect();
        setCursor(Qt::ArrowCursor);
    } else {
        m_drawingEngine = new SmoothPathEngine(element);
        setCursor(QCursor(QPixmap(QStringLiteral("pencil")), Qt::ArrowCursor));
        m_currentDrawingToolElement = element;
    }
}

void PresentationWidget::slotAddDrawingToolActions()
{
    DrawingToolActions *drawingToolActions = qobject_cast<DrawingToolActions *>(sender());

    const QList<QAction *> actionsList = drawingToolActions->actions();
    for (QAction *action : actionsList) {
        action->setEnabled(true);
        m_topBar->addAction(action);
        addAction(action);
    }
}

void PresentationWidget::clearDrawings()
{
    if (m_frameIndex != -1)
        m_frames[m_frameIndex]->drawings.clear();
    update();
}

void PresentationWidget::chooseScreen(QAction *act)
{
    if (!act || act->data().type() != QVariant::Int)
        return;

    const int newScreen = act->data().toInt();
    if (newScreen < QApplication::screens().count()) {
        setScreen(QApplication::screens().at(newScreen));
    }
}

void PresentationWidget::toggleBlackScreenMode(bool)
{
    m_inBlackScreenMode = !m_inBlackScreenMode;

    update();
}

void PresentationWidget::setScreen(const QScreen *newScreen)
{
    // To move to a new screen, need to disable fullscreen first:
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    if (newScreen != screen()) {
#else
    if (true) { // TODO Qt6 (oldQt_screenOf() doesn’t help here, because it has a default value.)
#endif
        setWindowState(windowState() & ~Qt::WindowFullScreen);
    }
    setGeometry(newScreen->geometry());
    setWindowState(windowState() | Qt::WindowFullScreen);
}

void PresentationWidget::inhibitPowerManagement()
{
#ifdef Q_OS_LINUX
    QString reason = i18nc("Reason for inhibiting the screensaver activation, when the presentation mode is active", "Giving a presentation");

    if (!m_screenInhibitCookie) {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"), QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("Inhibit"));
        message << QCoreApplication::applicationName();
        message << reason;

        QDBusPendingReply<uint> reply = QDBusConnection::sessionBus().asyncCall(message);
        reply.waitForFinished();
        if (reply.isValid()) {
            m_screenInhibitCookie = reply.value();
            qCDebug(OkularUiDebug) << "Screen inhibition cookie" << m_screenInhibitCookie;
        } else {
            qCWarning(OkularUiDebug) << "Unable to inhibit screensaver" << reply.error();
        }
    }

    if (m_sleepInhibitFd != -1) {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"), QStringLiteral("org.freedesktop.login1.Manager"), QStringLiteral("Inhibit"));
        message << QStringLiteral("sleep");
        message << QCoreApplication::applicationName();
        message << reason;
        message << QStringLiteral("block");

        QDBusPendingReply<QDBusUnixFileDescriptor> reply = QDBusConnection::systemBus().asyncCall(message);
        reply.waitForFinished();
        if (reply.isValid()) {
            m_sleepInhibitFd = dup(reply.value().fileDescriptor());
        } else {
            qCWarning(OkularUiDebug) << "Unable to inhibit sleep" << reply.error();
        }
    }
#endif
}

void PresentationWidget::allowPowerManagement()
{
#ifdef Q_OS_LINUX
    if (m_sleepInhibitFd != -1) {
        ::close(m_sleepInhibitFd);
        m_sleepInhibitFd = -1;
    }

    if (m_screenInhibitCookie) {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"), QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("UnInhibit"));
        message << m_screenInhibitCookie;

        QDBusPendingReply<uint> reply = QDBusConnection::sessionBus().asyncCall(message);
        reply.waitForFinished();

        m_screenInhibitCookie = 0;
    }
#endif
}

void PresentationWidget::showTopBar(bool show)
{
    if (show) {
        m_topBar->show();

        // Don't autohide the mouse cursor if it's over the toolbar
        if (Okular::Settings::slidesCursor() == Okular::Settings::EnumSlidesCursor::HiddenDelay) {
            KCursor::setAutoHideCursor(this, false);
        }

        // Always show a cursor when topBar is visible
        if (!m_drawingEngine) {
            setCursor(QCursor(Qt::ArrowCursor));
        }
    } else {
        m_topBar->hide();

        // Reenable autohide if need be when leaving the toolbar
        if (Okular::Settings::slidesCursor() == Okular::Settings::EnumSlidesCursor::HiddenDelay) {
            KCursor::setAutoHideCursor(this, true);
        }

        // Or hide the cursor again if hidden cursor is enabled
        else if (Okular::Settings::slidesCursor() == Okular::Settings::EnumSlidesCursor::Hidden) {
            // Don't hide the cursor if drawing mode is on
            if (!m_drawingEngine) {
                setCursor(QCursor(Qt::BlankCursor));
            }
        }
    }

    // Make sure mouse tracking isn't off after the KCursor::setAutoHideCursor() calls
    setMouseTracking(true);
}

void PresentationWidget::slotFind()
{
    if (!m_searchBar) {
        m_searchBar = new PresentationSearchBar(m_document, this, this);
        m_searchBar->forceSnap();
    }
    m_searchBar->focusOnSearchEdit();
    m_searchBar->show();
}

const Okular::PageTransition PresentationWidget::defaultTransition() const
{
    return defaultTransition(Okular::Settings::slidesTransition());
}

const Okular::PageTransition PresentationWidget::defaultTransition(int type) const
{
    switch (type) {
    case Okular::Settings::EnumSlidesTransition::BlindsHorizontal: {
        Okular::PageTransition transition(Okular::PageTransition::Blinds);
        transition.setAlignment(Okular::PageTransition::Horizontal);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::BlindsVertical: {
        Okular::PageTransition transition(Okular::PageTransition::Blinds);
        transition.setAlignment(Okular::PageTransition::Vertical);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::BoxIn: {
        Okular::PageTransition transition(Okular::PageTransition::Box);
        transition.setDirection(Okular::PageTransition::Inward);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::BoxOut: {
        Okular::PageTransition transition(Okular::PageTransition::Box);
        transition.setDirection(Okular::PageTransition::Outward);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::Dissolve: {
        return Okular::PageTransition(Okular::PageTransition::Dissolve);
        break;
    }
    case Okular::Settings::EnumSlidesTransition::GlitterDown: {
        Okular::PageTransition transition(Okular::PageTransition::Glitter);
        transition.setAngle(270);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::GlitterRight: {
        Okular::PageTransition transition(Okular::PageTransition::Glitter);
        transition.setAngle(0);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::GlitterRightDown: {
        Okular::PageTransition transition(Okular::PageTransition::Glitter);
        transition.setAngle(315);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::Random: {
        return defaultTransition(KRandom::random() % 18);
        break;
    }
    case Okular::Settings::EnumSlidesTransition::SplitHorizontalIn: {
        Okular::PageTransition transition(Okular::PageTransition::Split);
        transition.setAlignment(Okular::PageTransition::Horizontal);
        transition.setDirection(Okular::PageTransition::Inward);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::SplitHorizontalOut: {
        Okular::PageTransition transition(Okular::PageTransition::Split);
        transition.setAlignment(Okular::PageTransition::Horizontal);
        transition.setDirection(Okular::PageTransition::Outward);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::SplitVerticalIn: {
        Okular::PageTransition transition(Okular::PageTransition::Split);
        transition.setAlignment(Okular::PageTransition::Vertical);
        transition.setDirection(Okular::PageTransition::Inward);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::SplitVerticalOut: {
        Okular::PageTransition transition(Okular::PageTransition::Split);
        transition.setAlignment(Okular::PageTransition::Vertical);
        transition.setDirection(Okular::PageTransition::Outward);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::WipeDown: {
        Okular::PageTransition transition(Okular::PageTransition::Wipe);
        transition.setAngle(270);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::WipeRight: {
        Okular::PageTransition transition(Okular::PageTransition::Wipe);
        transition.setAngle(0);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::WipeLeft: {
        Okular::PageTransition transition(Okular::PageTransition::Wipe);
        transition.setAngle(180);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::WipeUp: {
        Okular::PageTransition transition(Okular::PageTransition::Wipe);
        transition.setAngle(90);
        return transition;
        break;
    }
    case Okular::Settings::EnumSlidesTransition::Fade: {
        return Okular::PageTransition(Okular::PageTransition::Fade);
        break;
    }
    case Okular::Settings::EnumSlidesTransition::NoTransitions:
    case Okular::Settings::EnumSlidesTransition::Replace:
    default:
        return Okular::PageTransition(Okular::PageTransition::Replace);
        break;
    }
    // should not happen, just make gcc happy
    return Okular::PageTransition();
}

/** ONLY the TRANSITIONS GENERATION function from here on **/
void PresentationWidget::initTransition(const Okular::PageTransition *transition)
{
    // if it's just a 'replace' transition, repaint the screen
    if (transition->type() == Okular::PageTransition::Replace) {
        update();
        return;
    }

    const bool isInward = transition->direction() == Okular::PageTransition::Inward;
    const bool isHorizontal = transition->alignment() == Okular::PageTransition::Horizontal;
    const float totalTime = transition->duration();

    m_transitionRects.clear();
    m_currentTransition = *transition;
    m_currentPagePixmap = m_lastRenderedPixmap;

    switch (transition->type()) {
        // split: horizontal / vertical and inward / outward
    case Okular::PageTransition::Split: {
        const int steps = isHorizontal ? 100 : 75;
        if (isHorizontal) {
            if (isInward) {
                int xPosition = 0;
                for (int i = 0; i < steps; i++) {
                    int xNext = ((i + 1) * m_width) / (2 * steps);
                    m_transitionRects.push_back(QRect(xPosition, 0, xNext - xPosition, m_height));
                    m_transitionRects.push_back(QRect(m_width - xNext, 0, xNext - xPosition, m_height));
                    xPosition = xNext;
                }
            } else {
                int xPosition = m_width / 2;
                for (int i = 0; i < steps; i++) {
                    int xNext = ((steps - (i + 1)) * m_width) / (2 * steps);
                    m_transitionRects.push_back(QRect(xNext, 0, xPosition - xNext, m_height));
                    m_transitionRects.push_back(QRect(m_width - xPosition, 0, xPosition - xNext, m_height));
                    xPosition = xNext;
                }
            }
        } else {
            if (isInward) {
                int yPosition = 0;
                for (int i = 0; i < steps; i++) {
                    int yNext = ((i + 1) * m_height) / (2 * steps);
                    m_transitionRects.push_back(QRect(0, yPosition, m_width, yNext - yPosition));
                    m_transitionRects.push_back(QRect(0, m_height - yNext, m_width, yNext - yPosition));
                    yPosition = yNext;
                }
            } else {
                int yPosition = m_height / 2;
                for (int i = 0; i < steps; i++) {
                    int yNext = ((steps - (i + 1)) * m_height) / (2 * steps);
                    m_transitionRects.push_back(QRect(0, yNext, m_width, yPosition - yNext));
                    m_transitionRects.push_back(QRect(0, m_height - yPosition, m_width, yPosition - yNext));
                    yPosition = yNext;
                }
            }
        }
        m_transitionMul = 2;
        m_transitionDelay = (int)((totalTime * 1000) / steps);
    } break;

        // blinds: horizontal(l-to-r) / vertical(t-to-b)
    case Okular::PageTransition::Blinds: {
        const int blinds = isHorizontal ? 8 : 6;
        const int steps = m_width / (4 * blinds);
        if (isHorizontal) {
            int xPosition[8];
            for (int b = 0; b < blinds; b++)
                xPosition[b] = (b * m_width) / blinds;

            for (int i = 0; i < steps; i++) {
                int stepOffset = (int)(((float)i * (float)m_width) / ((float)blinds * (float)steps));
                for (int b = 0; b < blinds; b++) {
                    m_transitionRects.push_back(QRect(xPosition[b], 0, stepOffset, m_height));
                    xPosition[b] = stepOffset + (b * m_width) / blinds;
                }
            }
        } else {
            int yPosition[6];
            for (int b = 0; b < blinds; b++)
                yPosition[b] = (b * m_height) / blinds;

            for (int i = 0; i < steps; i++) {
                int stepOffset = (int)(((float)i * (float)m_height) / ((float)blinds * (float)steps));
                for (int b = 0; b < blinds; b++) {
                    m_transitionRects.push_back(QRect(0, yPosition[b], m_width, stepOffset));
                    yPosition[b] = stepOffset + (b * m_height) / blinds;
                }
            }
        }
        m_transitionMul = blinds;
        m_transitionDelay = (int)((totalTime * 1000) / steps);
    } break;

        // box: inward / outward
    case Okular::PageTransition::Box: {
        const int steps = m_width / 10;
        if (isInward) {
            int L = 0, T = 0, R = m_width, B = m_height;
            for (int i = 0; i < steps; i++) {
                // compute shrunk box coords
                int newL = ((i + 1) * m_width) / (2 * steps);
                int newT = ((i + 1) * m_height) / (2 * steps);
                int newR = m_width - newL;
                int newB = m_height - newT;
                // add left, right, topcenter, bottomcenter rects
                m_transitionRects.push_back(QRect(L, T, newL - L, B - T));
                m_transitionRects.push_back(QRect(newR, T, R - newR, B - T));
                m_transitionRects.push_back(QRect(newL, T, newR - newL, newT - T));
                m_transitionRects.push_back(QRect(newL, newB, newR - newL, B - newB));
                L = newL;
                T = newT;
                R = newR, B = newB;
            }
        } else {
            int L = m_width / 2, T = m_height / 2, R = L, B = T;
            for (int i = 0; i < steps; i++) {
                // compute shrunk box coords
                int newL = ((steps - (i + 1)) * m_width) / (2 * steps);
                int newT = ((steps - (i + 1)) * m_height) / (2 * steps);
                int newR = m_width - newL;
                int newB = m_height - newT;
                // add left, right, topcenter, bottomcenter rects
                m_transitionRects.push_back(QRect(newL, newT, L - newL, newB - newT));
                m_transitionRects.push_back(QRect(R, newT, newR - R, newB - newT));
                m_transitionRects.push_back(QRect(L, newT, R - L, T - newT));
                m_transitionRects.push_back(QRect(L, B, R - L, newB - B));
                L = newL;
                T = newT;
                R = newR, B = newB;
            }
        }
        m_transitionMul = 4;
        m_transitionDelay = (int)((totalTime * 1000) / steps);
    } break;

        // wipe: implemented for 4 canonical angles
    case Okular::PageTransition::Wipe: {
        const int angle = transition->angle();
        const int steps = (angle == 0) || (angle == 180) ? m_width / 8 : m_height / 8;
        if (angle == 0) {
            int xPosition = 0;
            for (int i = 0; i < steps; i++) {
                int xNext = ((i + 1) * m_width) / steps;
                m_transitionRects.push_back(QRect(xPosition, 0, xNext - xPosition, m_height));
                xPosition = xNext;
            }
        } else if (angle == 90) {
            int yPosition = m_height;
            for (int i = 0; i < steps; i++) {
                int yNext = ((steps - (i + 1)) * m_height) / steps;
                m_transitionRects.push_back(QRect(0, yNext, m_width, yPosition - yNext));
                yPosition = yNext;
            }
        } else if (angle == 180) {
            int xPosition = m_width;
            for (int i = 0; i < steps; i++) {
                int xNext = ((steps - (i + 1)) * m_width) / steps;
                m_transitionRects.push_back(QRect(xNext, 0, xPosition - xNext, m_height));
                xPosition = xNext;
            }
        } else if (angle == 270) {
            int yPosition = 0;
            for (int i = 0; i < steps; i++) {
                int yNext = ((i + 1) * m_height) / steps;
                m_transitionRects.push_back(QRect(0, yPosition, m_width, yNext - yPosition));
                yPosition = yNext;
            }
        } else {
            update();
            return;
        }
        m_transitionMul = 1;
        m_transitionDelay = (int)((totalTime * 1000) / steps);
    } break;

        // dissolve: replace 'random' rects
    case Okular::PageTransition::Dissolve: {
        const int gridXsteps = 50;
        const int gridYsteps = 38;
        const int steps = gridXsteps * gridYsteps;
        int oldX = 0;
        int oldY = 0;
        // create a grid of gridXstep by gridYstep QRects
        for (int y = 0; y < gridYsteps; y++) {
            int newY = (int)(m_height * ((float)(y + 1) / (float)gridYsteps));
            for (int x = 0; x < gridXsteps; x++) {
                int newX = (int)(m_width * ((float)(x + 1) / (float)gridXsteps));
                m_transitionRects.push_back(QRect(oldX, oldY, newX - oldX, newY - oldY));
                oldX = newX;
            }
            oldX = 0;
            oldY = newY;
        }
        // randomize the grid
        for (int i = 0; i < steps; i++) {
#ifndef Q_OS_WIN
            int n1 = (int)(steps * drand48());
            int n2 = (int)(steps * drand48());
#else
            int n1 = (int)(steps * (std::rand() / RAND_MAX));
            int n2 = (int)(steps * (std::rand() / RAND_MAX));
#endif
            // swap items if index differs
            if (n1 != n2) {
                QRect r = m_transitionRects[n2];
                m_transitionRects[n2] = m_transitionRects[n1];
                m_transitionRects[n1] = r;
            }
        }
        // set global transition parameters
        m_transitionMul = 40;
        m_transitionDelay = (int)((m_transitionMul * 1000 * totalTime) / steps);
    } break;

        // glitter: similar to dissolve but has a direction
    case Okular::PageTransition::Glitter: {
        const int gridXsteps = 50;
        const int gridYsteps = 38;
        const int steps = gridXsteps * gridYsteps;
        const int angle = transition->angle();
        // generate boxes using a given direction
        if (angle == 90) {
            int yPosition = m_height;
            for (int i = 0; i < gridYsteps; i++) {
                int yNext = ((gridYsteps - (i + 1)) * m_height) / gridYsteps;
                int xPosition = 0;
                for (int j = 0; j < gridXsteps; j++) {
                    int xNext = ((j + 1) * m_width) / gridXsteps;
                    m_transitionRects.push_back(QRect(xPosition, yNext, xNext - xPosition, yPosition - yNext));
                    xPosition = xNext;
                }
                yPosition = yNext;
            }
        } else if (angle == 180) {
            int xPosition = m_width;
            for (int i = 0; i < gridXsteps; i++) {
                int xNext = ((gridXsteps - (i + 1)) * m_width) / gridXsteps;
                int yPosition = 0;
                for (int j = 0; j < gridYsteps; j++) {
                    int yNext = ((j + 1) * m_height) / gridYsteps;
                    m_transitionRects.push_back(QRect(xNext, yPosition, xPosition - xNext, yNext - yPosition));
                    yPosition = yNext;
                }
                xPosition = xNext;
            }
        } else if (angle == 270) {
            int yPosition = 0;
            for (int i = 0; i < gridYsteps; i++) {
                int yNext = ((i + 1) * m_height) / gridYsteps;
                int xPosition = 0;
                for (int j = 0; j < gridXsteps; j++) {
                    int xNext = ((j + 1) * m_width) / gridXsteps;
                    m_transitionRects.push_back(QRect(xPosition, yPosition, xNext - xPosition, yNext - yPosition));
                    xPosition = xNext;
                }
                yPosition = yNext;
            }
        } else // if angle is 0 or 315
        {
            int xPosition = 0;
            for (int i = 0; i < gridXsteps; i++) {
                int xNext = ((i + 1) * m_width) / gridXsteps;
                int yPosition = 0;
                for (int j = 0; j < gridYsteps; j++) {
                    int yNext = ((j + 1) * m_height) / gridYsteps;
                    m_transitionRects.push_back(QRect(xPosition, yPosition, xNext - xPosition, yNext - yPosition));
                    yPosition = yNext;
                }
                xPosition = xNext;
            }
        }
        // add a 'glitter' (1 over 10 pieces is randomized)
        int randomSteps = steps / 20;
        for (int i = 0; i < randomSteps; i++) {
#ifndef Q_OS_WIN
            int n1 = (int)(steps * drand48());
            int n2 = (int)(steps * drand48());
#else
            int n1 = (int)(steps * (std::rand() / RAND_MAX));
            int n2 = (int)(steps * (std::rand() / RAND_MAX));
#endif
            // swap items if index differs
            if (n1 != n2) {
                QRect r = m_transitionRects[n2];
                m_transitionRects[n2] = m_transitionRects[n1];
                m_transitionRects[n1] = r;
            }
        }
        // set global transition parameters
        m_transitionMul = (angle == 90) || (angle == 270) ? gridYsteps : gridXsteps;
        m_transitionMul /= 2;
        m_transitionDelay = (int)((m_transitionMul * 1000 * totalTime) / steps);
    } break;

    case Okular::PageTransition::Fade: {
        enum { FADE_TRANSITION_FPS = 20 };
        const int steps = totalTime * FADE_TRANSITION_FPS;
        m_transitionSteps = steps;
        QPainter pixmapPainter;
        m_currentPixmapOpacity = (double)1 / steps;
        m_transitionDelay = (int)(totalTime * 1000) / steps;
        m_lastRenderedPixmap = QPixmap(m_lastRenderedPixmap.size());
        m_lastRenderedPixmap.fill(Qt::transparent);
        pixmapPainter.begin(&m_lastRenderedPixmap);
        pixmapPainter.setCompositionMode(QPainter::CompositionMode_Source);
        pixmapPainter.setOpacity(1 - m_currentPixmapOpacity);
        pixmapPainter.drawPixmap(0, 0, m_previousPagePixmap);
        pixmapPainter.setOpacity(m_currentPixmapOpacity);
        pixmapPainter.drawPixmap(0, 0, m_currentPagePixmap);
        pixmapPainter.end();
        update();
    } break;
    // implement missing transitions (a binary raster engine needed here)
    case Okular::PageTransition::Fly:

    case Okular::PageTransition::Push:

    case Okular::PageTransition::Cover:

    case Okular::PageTransition::Uncover:

    default:
        update();
        return;
    }

    // send the first start to the timer
    m_transitionTimer->start(0);
}

void PresentationWidget::slotProcessMovieAction(const Okular::MovieAction *action)
{
    const Okular::MovieAnnotation *movieAnnotation = action->annotation();
    if (!movieAnnotation)
        return;

    Okular::Movie *movie = movieAnnotation->movie();
    if (!movie)
        return;

    VideoWidget *vw = m_frames[m_frameIndex]->videoWidgets.value(movieAnnotation->movie());
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

void PresentationWidget::slotProcessRenditionAction(const Okular::RenditionAction *action)
{
    Okular::Movie *movie = action->movie();
    if (!movie)
        return;

    VideoWidget *vw = m_frames[m_frameIndex]->videoWidgets.value(movie);
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

void PresentationWidget::slotTogglePlayPause()
{
    if (!m_nextPageTimer->isActive()) {
        m_advanceSlides = true;
        startAutoChangeTimer();
    } else {
        m_nextPageTimer->stop();
        m_advanceSlides = false;
        setPlayPauseIcon();
    }
}

#include "presentationwidget.moc"
