/***************************************************************************
 *   Copyright (C) 2012 by Tobias Koening <tokoe@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef SNAPSHOTTAKER_H
#define SNAPSHOTTAKER_H

#include <phonon/videoplayer.h>

#include <QObject>

class QImage;

class SnapshotTaker : public QObject
{
    Q_OBJECT

public:
    explicit SnapshotTaker(const QUrl &url, QObject *parent = nullptr);
    ~SnapshotTaker() override;

Q_SIGNALS:
    void finished(const QImage &image);

private Q_SLOTS:
    void stateChanged(Phonon::State, Phonon::State);

private:
    Phonon::VideoPlayer *m_player;
};

#endif
