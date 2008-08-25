/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "videowidget.h"

// qt/kde includes
#include <qaction.h>
#include <qdir.h>
#include <qevent.h>
#include <qlayout.h>
#include <qtoolbar.h>

#include <kicon.h>
#include <klocale.h>

#include <phonon/videoplayer.h>

#include "core/area.h"
#include "core/annotations.h"
#include "core/document.h"
#include "core/movie.h"

/* Private storage. */
class VideoWidget::Private
{
public:
    Private( Okular::MovieAnnotation *ma, Okular::Document *doc, VideoWidget *qq )
        : q( qq ), anno( ma ), document( doc ), loaded( false )
    {
    }

    enum PlayPauseMode { PlayMode, PauseMode };

    void load();
    void setupPlayPauseAction( PlayPauseMode mode );

    // slots
    void finished();
    void playOrPause();

    VideoWidget *q;
    Okular::MovieAnnotation *anno;
    Okular::Document *document;
    Okular::NormalizedRect geom;
    Phonon::VideoPlayer *player;
    QToolBar *controlBar;
    QAction *playPauseAction;
    QAction *stopAction;
    bool loaded : 1;
};

void VideoWidget::Private::load()
{
    if ( loaded )
        return;

    loaded = true;

    QString url = anno->movie()->url();
    KUrl newurl;
    if ( QDir::isRelativePath( url ) )
    {
        newurl = document->currentDocument();
        newurl.setFileName( url );
    }
    else
    {
        newurl = url;
    }
    if ( newurl.isLocalFile() )
        player->load( newurl.toLocalFile() );
    else
        player->load( newurl );
}

void VideoWidget::Private::setupPlayPauseAction( PlayPauseMode mode )
{
    if ( mode == PlayMode )
    {
        playPauseAction->setIcon( KIcon( "media-playback-start" ) );
        playPauseAction->setText( i18nc( "start the movie playback", "Play" ) );
    }
    else if ( mode == PauseMode )
    {
        playPauseAction->setIcon( KIcon( "media-playback-pause" ) );
        playPauseAction->setText( i18nc( "pause the movie playback", "Pause" ) );
    }
}

void VideoWidget::Private::finished()
{
    switch ( anno->movie()->playMode() )
    {
        case Okular::Movie::PlayOnce:
        case Okular::Movie::PlayOpen:
            // playback has ended, nothing to do
            stopAction->setEnabled( false );
            setupPlayPauseAction( PlayMode );
            if ( anno->movie()->playMode() == Okular::Movie::PlayOnce )
                controlBar->setVisible( false );
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
    if ( player->isPlaying() )
    {
        player->pause();
        setupPlayPauseAction( PlayMode );
    }
    else
    {
        q->play();
    }
}

VideoWidget::VideoWidget( Okular::MovieAnnotation *movieann, Okular::Document *document, QWidget *parent )
    : QWidget( parent ), d( new Private( movieann, document, this ) )
{
    // do not propagate the mouse events to the parent widget;
    // they should be tied to this widget, not spread around...
    setAttribute( Qt::WA_NoMousePropagation );

    QVBoxLayout *mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 0 );

    d->player = new Phonon::VideoPlayer( Phonon::NoCategory, this );
    d->player->installEventFilter( this );
    mainlay->addWidget( d->player );

    d->controlBar = new QToolBar( this );
    d->controlBar->setIconSize( QSize( 16, 16 ) );
    d->controlBar->setAutoFillBackground( true );
    mainlay->addWidget( d->controlBar );

    d->playPauseAction = new QAction( d->controlBar );
    d->controlBar->addAction( d->playPauseAction );
    d->setupPlayPauseAction( Private::PlayMode );
    d->stopAction = d->controlBar->addAction(
        KIcon( "media-playback-stop" ),
        i18nc( "stop the movie playback", "Stop" ),
        this, SLOT( stop() ) );
    d->stopAction->setEnabled( false );

    d->controlBar->setVisible( movieann->movie()->showControls() );

    connect( d->player, SIGNAL( finished() ), this, SLOT( finished() ) );
    connect( d->playPauseAction, SIGNAL( triggered() ), this, SLOT( playOrPause() ) );

    d->geom = movieann->transformedBoundingRectangle();
}

VideoWidget::~VideoWidget()
{
    delete d;
}

void VideoWidget::setNormGeometry( const Okular::NormalizedRect &rect )
{
    d->geom = rect;
}

Okular::NormalizedRect VideoWidget::normGeometry() const
{
    return d->geom;
}

void VideoWidget::play()
{
    d->load();
    d->player->play();
    d->stopAction->setEnabled( true );
    d->setupPlayPauseAction( Private::PauseMode );
}

void VideoWidget::stop()
{
    d->player->stop();
    d->stopAction->setEnabled( false );
    d->setupPlayPauseAction( Private::PlayMode );
}

void VideoWidget::pause()
{
    d->player->pause();
    d->setupPlayPauseAction( Private::PlayMode );
}

bool VideoWidget::eventFilter( QObject * object, QEvent * event )
{
    if ( object == d->player )
    {
        QMouseEvent * me = static_cast< QMouseEvent * >( event );
        if ( me->button() == Qt::LeftButton )
        {
            if ( !d->player->isPlaying() )
            {
                play();
            }
            event->accept();
        }
    }

    return false;
}

#include "videowidget.moc"
