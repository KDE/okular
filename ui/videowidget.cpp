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
#include <qdir.h>
#include <qevent.h>
#include <qlayout.h>

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

    void load();

    // slots
    void finished();

    VideoWidget *q;
    Okular::MovieAnnotation *anno;
    Okular::Document *document;
    Okular::NormalizedRect geom;
    Phonon::VideoPlayer *player;
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

void VideoWidget::Private::finished()
{
    switch ( anno->movie()->playMode() )
    {
        case Okular::Movie::PlayOnce:
            // TODO hide the control bar
            break;
        case Okular::Movie::PlayOpen:
            // playback has ended, nothing to do
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

VideoWidget::VideoWidget( Okular::MovieAnnotation *movieann, Okular::Document *document, QWidget *parent )
    : QWidget( parent ), d( new Private( movieann, document, this ) )
{
    // do not propagate the mouse events to the parent widget;
    // they should be tied to this widget, not spread around...
    setAttribute( Qt::WA_NoMousePropagation );

    QVBoxLayout *mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 0 );

    d->player = new Phonon::VideoPlayer( Phonon::NoCategory, this );
    mainlay->addWidget( d->player );

    connect( d->player, SIGNAL( finished() ), this, SLOT( finished() ) );

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
}

void VideoWidget::stop()
{
    d->player->stop();
}

void VideoWidget::pause()
{
    d->player->pause();
}

void VideoWidget::mousePressEvent( QMouseEvent * event )
{
    if ( event->button() == Qt::LeftButton )
    {
        if ( !d->player->isPlaying() )
        {
            play();
        }
        event->accept();
    }

    QWidget::mousePressEvent( event );
}

#include "videowidget.moc"
