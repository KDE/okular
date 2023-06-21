/*
    SPDX-FileCopyrightText: 2012 Tobias Koening <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SNAPSHOTTAKER_H
#define SNAPSHOTTAKER_H
#include "config-okular.h"

#if HAVE_PHONON

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
#endif
