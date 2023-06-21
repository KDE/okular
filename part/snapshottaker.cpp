/*
    SPDX-FileCopyrightText: 2012 Tobias Koening <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "snapshottaker.h"

#if HAVE_PHONON

#include <phonon/mediaobject.h>
#include <phonon/videowidget.h>

#include <QImage>

SnapshotTaker::SnapshotTaker(const QUrl &url, QObject *parent)
    : QObject(parent)
    , m_player(new Phonon::VideoPlayer(Phonon::NoCategory, nullptr))
{
    m_player->load(url);
    m_player->hide();

    connect(m_player->mediaObject(), &Phonon::MediaObject::stateChanged, this, &SnapshotTaker::stateChanged);

    m_player->play();
}

SnapshotTaker::~SnapshotTaker()
{
    m_player->stop();
    delete m_player;
}

void SnapshotTaker::stateChanged(Phonon::State newState, Phonon::State)
{
    if (newState == Phonon::PlayingState) {
        const QImage image = m_player->videoWidget()->snapshot();
        if (!image.isNull()) {
            Q_EMIT finished(image);
        }

        m_player->stop();
        deleteLater();
    }
}
#endif
