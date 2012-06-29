#ifndef SNAPSHOTTAKER_H
#define SNAPSHOTTAKER_H

#include <phonon/videoplayer.h>

#include <QtCore/QObject>

class QImage;

class SnapshotTaker : public QObject
{
    Q_OBJECT

    public:
        SnapshotTaker( const QString &url, QObject *parent = 0 );

    Q_SIGNALS:
        void finished( const QImage &image );

    private Q_SLOTS:
        void stateChanged(Phonon::State, Phonon::State);

    private:
        Phonon::VideoPlayer *m_player;
};

#endif
