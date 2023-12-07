/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "config-okular.h"

#include "videowidget.h"

// qt/kde includes
#include <qaction.h>
#include <qcoreapplication.h>
#include <qdir.h>
#include <qevent.h>
#include <qlabel.h>
#include <qlayout.h>
#if HAVE_MULTIMEDIA
#include <qmediaplayer.h>
#endif
#include <qmenu.h>
#include <qstackedlayout.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#if HAVE_MULTIMEDIA
#include <qvideowidget.h>
#endif
#include <qwidgetaction.h>

#include <KLocalizedString>
#include <QIcon>

#include "core/annotations.h"
#include "core/area.h"
#include "core/document.h"
#include "core/movie.h"

const int kVideoPage = 0;
const int kPosterPage = 1;

#if HAVE_MULTIMEDIA

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
        if (player) {
            player->stop();

            // To handle https://bugreports.qt.io/browse/QTBUG-87325 do these in this order...
            // FIXME: Remove once we port to Qt6 and/or the bug is fixed.
            player->disconnect();
            videoWidget->disconnect();
            videoWidget->setParent(nullptr);
            player->setParent(nullptr);
            player->deleteLater();
            videoWidget->deleteLater();
        }
    }

    enum PlayPauseMode { PlayMode, PauseMode };

    void load();
    void setupPlayPauseAction(PlayPauseMode mode);
    void setPosterImage(const QImage &);
    void takeSnapshot();
    void videoStopped();
    void stateChanged(QMediaPlayer::State newState);

    // slots
    void mediaStatusChanged(QMediaPlayer::MediaStatus);
    void playOrPause();

    VideoWidget *q;
    Okular::Movie *movie;
    Okular::Document *document;
    Okular::NormalizedRect geom;
    QMediaPlayer *player;
    QVideoWidget *videoWidget;
    QToolBar *controlBar;
    QAction *playPauseAction;
    QAction *stopAction;
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
    if (loaded) {
        return;
    }

    loaded = true;

    player->setMedia(urlFromUrlString(movie->url(), document));

    connect(player, &QMediaPlayer::stateChanged, q, [this](QMediaPlayer::State s) { stateChanged(s); });
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
    QPixmap pixmap = videoWidget->grab();
    QImage image = pixmap.toImage();
    setPosterImage(image);
}

void VideoWidget::Private::videoStopped()
{
    if (movie->showPosterImage()) {
        pageLayout->setCurrentIndex(kPosterPage);
    } else {
        q->hide();
    }
}

void VideoWidget::Private::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    // For now we only care about when the media finished since this used to be
    // a finished() slot.
    if (status == QMediaPlayer::EndOfMedia) {
        switch (movie->playMode()) {
        case Okular::Movie::PlayLimited:
        case Okular::Movie::PlayOpen:
            repetitionsLeft -= 1.0;
            if (repetitionsLeft < 1e-5) { // allow for some calculation error
                // playback has ended
                stopAction->setEnabled(false);
                setupPlayPauseAction(PlayMode);
                if (movie->playMode() == Okular::Movie::PlayLimited) {
                    controlBar->setVisible(false);
                }
                videoStopped();
            } else {
                // not done yet, repeat
                // if repetitionsLeft is less than 1, we are supposed to stop midway, but not even Adobe reader does this
                player->play();
            }
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
}

void VideoWidget::Private::playOrPause()
{
    if (player->state() == QMediaPlayer::PlayingState) {
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

void VideoWidget::Private::stateChanged(QMediaPlayer::State newState)
{
    if (newState == QMediaPlayer::PlayingState) {
        pageLayout->setCurrentIndex(kVideoPage);
    }
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

    d->player = new QMediaPlayer();
    d->videoWidget = new QVideoWidget(playerPage);
    d->player->setVideoOutput(d->videoWidget);
    mainlay->addWidget(d->videoWidget);

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

    d->controlBar->setVisible(movie->showControls());

    connect(d->player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) { d->mediaStatusChanged(status); });
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
        d->pageLayout->setCurrentIndex(kPosterPage);

        const QImage posterImage = movie->posterImage();
        if (posterImage.isNull()) {
            d->takeSnapshot();
        } else {
            d->setPosterImage(posterImage);
        }
    } else {
        d->pageLayout->setCurrentIndex(kVideoPage);
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
    return d->player->state() == QMediaPlayer::PlayingState;
}

void VideoWidget::pageInitialized()
{
    hide();
}

void VideoWidget::pageEntered()
{
    if (d->movie->showPosterImage()) {
        d->pageLayout->setCurrentIndex(kPosterPage);
        show();
    }

    if (d->movie->autoPlay()) {
        show();
        QMetaObject::invokeMethod(this, "play", Qt::QueuedConnection);
        if (d->movie->startPaused()) {
            QMetaObject::invokeMethod(this, "pause", Qt::QueuedConnection);
        }
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
                if (d->player->state() != QMediaPlayer::PlayingState) {
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

#else

class VideoWidget::Private
{
public:
    Okular::NormalizedRect geom;
};

bool VideoWidget::event(QEvent *event)
{
    return QWidget::event(event);
}

bool VideoWidget::eventFilter(QObject *object, QEvent *event)
{
    return QWidget::eventFilter(object, event);
}

bool VideoWidget::isPlaying() const
{
    return false;
}

Okular::NormalizedRect VideoWidget::normGeometry() const
{
    return d->geom;
}

void VideoWidget::pageEntered()
{
    show();
}

void VideoWidget::pageInitialized()
{
}

void VideoWidget::pageLeft()
{
}
void VideoWidget::pause()
{
}
void VideoWidget::play()
{
}

void VideoWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void VideoWidget::setNormGeometry(const Okular::NormalizedRect &rect)
{
    d->geom = rect;
}

void VideoWidget::stop()
{
}

VideoWidget::VideoWidget(const Okular::Annotation *annotation, Okular::Movie *movie, Okular::Document *document, QWidget *parent)
    : QWidget(parent)
    , d(new VideoWidget::Private)
{
    auto layout = new QVBoxLayout();
    d->geom = annotation->transformedBoundingRectangle();
    auto poster = new QLabel;
    if (movie->showPosterImage()) {
        auto posterImage = movie->posterImage();
        if (!posterImage.isNull()) {
            poster->setPixmap(QPixmap::fromImage(posterImage));
        }
    }
    Q_EMIT document->warning(i18n("Videos not supported in this okular"), 5000);
    poster->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    layout->addWidget(poster, 2);

    auto label = new QLabel(i18n("Videos not supported in this Okular"));
    label->setAutoFillBackground(true);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addWidget(label, 1, Qt::AlignCenter);
    setLayout(layout);
}

VideoWidget::~VideoWidget() noexcept
{
}

#endif
#include "moc_videowidget.cpp"
