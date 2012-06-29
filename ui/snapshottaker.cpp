#include "snapshottaker.h"

#include <phonon/mediaobject.h>
#include <phonon/videowidget.h>

#include <QtGui/QImage>

SnapshotTaker::SnapshotTaker( const QString &url, QObject *parent )
    : QObject( parent )
    , m_player( new Phonon::VideoPlayer( Phonon::NoCategory, 0 ) )
{
    m_player->load( url );
    m_player->hide();

    connect(m_player->mediaObject(), SIGNAL(stateChanged(Phonon::State, Phonon::State)),
            this, SLOT(stateChanged(Phonon::State, Phonon::State)));

    m_player->play();
}

void SnapshotTaker::stateChanged(Phonon::State newState, Phonon::State)
{
    if (newState == Phonon::PlayingState) {
        const QImage image = m_player->videoWidget()->snapshot();
        if (!image.isNull())
            emit finished( image );

        m_player->stop();
        deleteLater();
    }
}
