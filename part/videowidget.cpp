/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "videowidget.h"

// qt/kde includes
#include <qaction.h>
#include <qcoreapplication.h>
#include <qdir.h>
#include <qevent.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmenu.h>
#include <qstackedlayout.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qwidgetaction.h>

#include <KLocalizedString>
#include <QIcon>

#include <phonon/mediaobject.h>
#include <phonon/seekslider.h>
#include <phonon/videoplayer.h>

#include "core/annotations.h"
#include "core/area.h"
#include "core/document.h"
#include "core/movie.h"
#include "snapshottaker.h"

static QAction *createToolBarButtonWithWidgetPopup(QToolBar *toolBar, QWidget *widget, const QIcon &icon)
{
    QToolButton *button = new QToolButton(toolBar);
    QAction *action = toolBar->addWidget(button);
    button->setAutoRaise(true);
    button->setIcon(icon);
    button->setPopupMode(QToolButton::InstantPopup);
    QMenu *menu = new QMenu(button);
    button->setMenu(menu);
    QWidgetAction *widgetAction = new QWidgetAction(menu);
    QWidget *dummy = new QWidget(menu);
    widgetAction->setDefaultWidget(dummy);
    QVBoxLayout *dummyLayout = new QVBoxLayout(dummy);
    dummyLayout->setContentsMargins(5, 5, 5, 5);
    dummyLayout->addWidget(widget);
    menu->addAction(widgetAction);
    return action;
}

/* Private storage. */
class VideoWidget::Private
{
public:
    Private(Okular::Movie *m, Okular::Document *doc, VideoWidget *qq)
        : q(qq)
        , movie(m)
        , document(doc)
        , player(nullptr)
        , loaded(false)
    {
    }

    ~Private()
    {
        if (player)
            player->stop();
    }

    enum PlayPauseMode { PlayMode, PauseMode };

    void load();
    void setupPlayPauseAction(PlayPauseMode mode);
    void setPosterImage(const QImage &);
    void takeSnapshot();
    void videoStopped();
    void stateChanged(Phonon::State newState);

    // slots
    void finished();
    void playOrPause();

    VideoWidget *q;
    Okular::Movie *movie;
    Okular::Document *document;
    Okular::NormalizedRect geom;
    Phonon::VideoPlayer *player;
    Phonon::SeekSlider *seekSlider;
    QToolBar *controlBar;
    QAction *playPauseAction;
    QAction *stopAction;
    QAction *seekSliderAction;
    QAction *seekSliderMenuAction;
    QStackedLayout *pageLayout;
    QLabel *posterImagePage;
    bool loaded : 1;
    double repetitionsLeft;
};

static QUrl urlFromUrlString(const QString &url, Okular::Document *document)
{
    QUrl newurl;
    if (url.startsWith(QLatin1Char('/'))) {
        newurl = QUrl::fromLocalFile(url);
    } else {
        newurl = QUrl(url);
        if (newurl.isRelative()) {
            newurl = document->currentDocument().adjusted(QUrl::RemoveFilename);
            newurl.setPath(newurl.path() + url);
        }
    }
    return newurl;
}

void VideoWidget::Private::load()
{
    repetitionsLeft = movie->playRepetitions();
    if (loaded)
        return;

    loaded = true;

    player->load(urlFromUrlString(movie->url(), document));

    connect(player->mediaObject(), &Phonon::MediaObject::stateChanged, q, [this](Phonon::State s) { stateChanged(s); });

    seekSlider->setEnabled(true);
}

void VideoWidget::Private::setupPlayPauseAction(PlayPauseMode mode)
{
    if (mode == PlayMode) {
        playPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
        playPauseAction->setText(i18nc("start the movie playback", "Play"));
    } else if (mode == PauseMode) {
        playPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
        playPauseAction->setText(i18nc("pause the movie playback", "Pause"));
    }
}

void VideoWidget::Private::takeSnapshot()
{
    const QUrl url = urlFromUrlString(movie->url(), document);
    SnapshotTaker *taker = new SnapshotTaker(url, q);

    q->connect(taker, &SnapshotTaker::finished, q, [this](const QImage &image) { setPosterImage(image); });
}

void VideoWidget::Private::videoStopped()
{
    if (movie->showPosterImage())
        pageLayout->setCurrentIndex(1);
    else
        q->hide();
}

void VideoWidget::Private::finished()
{
    switch (movie->playMode()) {
    case Okular::Movie::PlayLimited:
    case Okular::Movie::PlayOpen:
        repetitionsLeft -= 1.0;
        if (repetitionsLeft < 1e-5) { // allow for some calculation error
            // playback has ended
            stopAction->setEnabled(false);
            setupPlayPauseAction(PlayMode);
            if (movie->playMode() == Okular::Movie::PlayLimited)
                controlBar->setVisible(false);
            videoStopped();
        } else
            // not done yet, repeat
            // if repetitionsLeft is less than 1, we are supposed to stop midway, but not even Adobe reader does this
            player->play();
        break;
    case Okular::Movie::PlayRepeat:
        // repeat the playback
        player->play();
        break;
    case Okular::Movie::PlayPalindrome:
        // FIXME we should play backward, but we cannot
        player->play();
        break;
    }
}

void VideoWidget::Private::playOrPause()
{
    if (player->isPlaying()) {
        player->pause();
        setupPlayPauseAction(PlayMode);
    } else {
        q->play();
    }
}

void VideoWidget::Private::setPosterImage(const QImage &image)
{
    if (!image.isNull()) {
        // cache the snapshot image
        movie->setPosterImage(image);
    }

    posterImagePage->setPixmap(QPixmap::fromImage(image));
}

void VideoWidget::Private::stateChanged(Phonon::State newState)
{
    if (newState == Phonon::PlayingState)
        pageLayout->setCurrentIndex(0);
}

VideoWidget::VideoWidget(const Okular::Annotation *annotation, Okular::Movie *movie, Okular::Document *document, QWidget *parent)
    : QWidget(parent)
    , d(new Private(movie, document, this))
{
    // do not propagate the mouse events to the parent widget;
    // they should be tied to this widget, not spread around...
    setAttribute(Qt::WA_NoMousePropagation);

    // Setup player page
    QWidget *playerPage = new QWidget(this);

    QVBoxLayout *mainlay = new QVBoxLayout(playerPage);
    mainlay->setContentsMargins(0, 0, 0, 0);
    mainlay->setSpacing(0);

    d->player = new Phonon::VideoPlayer(Phonon::NoCategory, playerPage);
    d->player->installEventFilter(playerPage);
    mainlay->addWidget(d->player);

    d->controlBar = new QToolBar(playerPage);
    d->controlBar->setIconSize(QSize(16, 16));
    d->controlBar->setAutoFillBackground(true);
    mainlay->addWidget(d->controlBar);

    d->playPauseAction = new QAction(d->controlBar);
    d->controlBar->addAction(d->playPauseAction);
    d->setupPlayPauseAction(Private::PlayMode);
    d->stopAction = d->controlBar->addAction(QIcon::fromTheme(QStringLiteral("media-playback-stop")), i18nc("stop the movie playback", "Stop"), this, SLOT(stop()));
    d->stopAction->setEnabled(false);
    d->controlBar->addSeparator();
    d->seekSlider = new Phonon::SeekSlider(d->player->mediaObject(), d->controlBar);
    d->seekSliderAction = d->controlBar->addWidget(d->seekSlider);
    d->seekSlider->setEnabled(false);

    Phonon::SeekSlider *verticalSeekSlider = new Phonon::SeekSlider(d->player->mediaObject(), nullptr);
    verticalSeekSlider->setMaximumHeight(100);
    d->seekSliderMenuAction = createToolBarButtonWithWidgetPopup(d->controlBar, verticalSeekSlider, QIcon::fromTheme(QStringLiteral("player-time")));
    d->seekSliderMenuAction->setVisible(false);

    d->controlBar->setVisible(movie->showControls());

    connect(d->player, &Phonon::VideoPlayer::finished, this, [this] { d->finished(); });
    connect(d->playPauseAction, &QAction::triggered, this, [this] { d->playOrPause(); });

    d->geom = annotation->transformedBoundingRectangle();

    // Setup poster image page
    d->posterImagePage = new QLabel;
    d->posterImagePage->setScaledContents(true);
    d->posterImagePage->installEventFilter(this);
    d->posterImagePage->setCursor(Qt::PointingHandCursor);

    d->pageLayout = new QStackedLayout(this);
    d->pageLayout->setMargin(0);
    d->pageLayout->setSpacing(0);
    d->pageLayout->addWidget(playerPage);
    d->pageLayout->addWidget(d->posterImagePage);

    if (movie->showPosterImage()) {
        d->pageLayout->setCurrentIndex(1);

        const QImage posterImage = movie->posterImage();
        if (posterImage.isNull()) {
            d->takeSnapshot();
        } else {
            d->setPosterImage(posterImage);
        }
    } else {
        d->pageLayout->setCurrentIndex(0);
    }
}

VideoWidget::~VideoWidget()
{
    delete d;
}

void VideoWidget::setNormGeometry(const Okular::NormalizedRect &rect)
{
    d->geom = rect;
}

Okular::NormalizedRect VideoWidget::normGeometry() const
{
    return d->geom;
}

bool VideoWidget::isPlaying() const
{
    return d->player->isPlaying();
}

void VideoWidget::pageInitialized()
{
    hide();
}

void VideoWidget::pageEntered()
{
    if (d->movie->showPosterImage()) {
        d->pageLayout->setCurrentIndex(1);
        show();
    }

    if (d->movie->autoPlay()) {
        show();
        QMetaObject::invokeMethod(this, "play", Qt::QueuedConnection);
    }
}

void VideoWidget::pageLeft()
{
    d->player->stop();
    d->videoStopped();

    hide();
}

void VideoWidget::play()
{
    d->controlBar->setVisible(d->movie->showControls());
    d->load();
    // if d->repetitionsLeft is less than 1, we are supposed to stop midway, but not even Adobe reader does this
    d->player->play();
    d->stopAction->setEnabled(true);
    d->setupPlayPauseAction(Private::PauseMode);
}

void VideoWidget::stop()
{
    d->player->stop();
    d->stopAction->setEnabled(false);
    d->setupPlayPauseAction(Private::PlayMode);
}

void VideoWidget::pause()
{
    d->player->pause();
    d->setupPlayPauseAction(Private::PlayMode);
}

bool VideoWidget::eventFilter(QObject *object, QEvent *event)
{
    if (object == d->player || object == d->posterImagePage) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                if (!d->player->isPlaying()) {
                    play();
                }
                event->accept();
            }
            break;
        }
        case QEvent::Wheel: {
            if (object == d->posterImagePage) {
                QWheelEvent *we = static_cast<QWheelEvent *>(event);

                // forward wheel events to parent widget
                QWheelEvent *copy = new QWheelEvent(we->pos(), we->globalPos(), we->angleDelta().y(), we->buttons(), we->modifiers(), we->orientation());
                QCoreApplication::postEvent(parentWidget(), copy);
            }
            break;
        }
        default:;
        }
    }

    return false;
}

bool VideoWidget::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ToolTip:
        // "eat" the help events (= tooltips), avoid parent widgets receiving them
        event->accept();
        return true;
        break;
    default:;
    }

    return QWidget::event(event);
}

void VideoWidget::resizeEvent(QResizeEvent *event)
{
    const QSize &s = event->size();
    int usedSpace = d->seekSlider->geometry().left() + d->seekSlider->iconSize().width();
    // try to give the slider at least 30px of space
    if (s.width() < (usedSpace + 30)) {
        d->seekSliderAction->setVisible(false);
        d->seekSliderMenuAction->setVisible(true);
    } else {
        d->seekSliderAction->setVisible(true);
        d->seekSliderMenuAction->setVisible(false);
    }
}

#include "moc_videowidget.cpp"
