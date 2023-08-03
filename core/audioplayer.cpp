/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "audioplayer.h"

// qt/kde includes
#include <KLocalizedString>
#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QRandomGenerator>

#include "config-okular.h"

#if HAVE_PHONON
#include <phonon/abstractmediastream.h>
#include <phonon/audiooutput.h>
#include <phonon/mediaobject.h>
#include <phonon/path.h>
#endif

// local includes
#include "action.h"
#include "debug_p.h"
#include "document.h"
#include "sound.h"
#include <stdlib.h>

using namespace Okular;

#if HAVE_PHONON

class PlayData;
class SoundInfo;

namespace Okular
{
class AudioPlayerPrivate
{
public:
    explicit AudioPlayerPrivate(AudioPlayer *qq);

    ~AudioPlayerPrivate();

    int newId() const;
    bool play(const SoundInfo &si);
    void stopPlayings();

    void finished(int);

    AudioPlayer *q;

    QHash<int, PlayData *> m_playing;
    QUrl m_currentDocument;
    AudioPlayer::State m_state;
};
}

// helper class used to store info about a sound to be played
class SoundInfo
{
public:
    explicit SoundInfo(const Sound *s = nullptr, const SoundAction *ls = nullptr)
        : sound(s)
        , volume(0.5)
        , synchronous(false)
        , repeat(false)
        , mix(false)
    {
        if (ls) {
            volume = ls->volume();
            synchronous = ls->synchronous();
            repeat = ls->repeat();
            mix = ls->mix();
        }
    }

    const Sound *sound;
    double volume;
    bool synchronous;
    bool repeat;
    bool mix;
};

class PlayData
{
public:
    PlayData()
        : m_mediaobject(nullptr)
        , m_output(nullptr)
        , m_buffer(nullptr)
    {
    }

    void play()
    {
        if (m_buffer) {
            m_buffer->open(QIODevice::ReadOnly);
        }
        m_mediaobject->play();
    }

    ~PlayData()
    {
        m_mediaobject->stop();
        delete m_mediaobject;
        delete m_output;
        delete m_buffer;
    }

    PlayData(const PlayData &) = delete;
    PlayData &operator=(const PlayData &) = delete;

    Phonon::MediaObject *m_mediaobject;
    Phonon::AudioOutput *m_output;
    QBuffer *m_buffer;
    SoundInfo m_info;
};

AudioPlayerPrivate::AudioPlayerPrivate(AudioPlayer *qq)
    : q(qq)
    , m_state(AudioPlayer::StoppedState)
{
}

AudioPlayerPrivate::~AudioPlayerPrivate()
{
    stopPlayings();
}

int AudioPlayerPrivate::newId() const
{
    auto random = QRandomGenerator::global();
    int newid = 0;
    QHash<int, PlayData *>::const_iterator it;
    QHash<int, PlayData *>::const_iterator itEnd = m_playing.constEnd();
    do {
        newid = random->bounded(RAND_MAX);
        it = m_playing.constFind(newid);
    } while (it != itEnd);
    return newid;
}

bool AudioPlayerPrivate::play(const SoundInfo &si)
{
    qCDebug(OkularCoreDebug);
    PlayData *data = new PlayData();
    data->m_output = new Phonon::AudioOutput(Phonon::NotificationCategory);
    data->m_output->setVolume(si.volume);
    data->m_mediaobject = new Phonon::MediaObject();
    Phonon::createPath(data->m_mediaobject, data->m_output);
    data->m_info = si;
    bool valid = false;

    switch (si.sound->soundType()) {
    case Sound::External: {
        QString url = si.sound->url();
        qCDebug(OkularCoreDebug) << "External," << url;
        if (!url.isEmpty()) {
            int newid = newId();
            QObject::connect(data->m_mediaobject, &Phonon::MediaObject::finished, q, [this, newid]() { finished(newid); });
            const QUrl newurl = QUrl::fromUserInput(url, m_currentDocument.adjusted(QUrl::RemoveFilename).toLocalFile());
            data->m_mediaobject->setCurrentSource(newurl);
            m_playing.insert(newid, data);
            valid = true;
        }
        break;
    }
    case Sound::Embedded: {
        QByteArray filedata = si.sound->data();
        qCDebug(OkularCoreDebug) << "Embedded," << filedata.length();
        if (!filedata.isEmpty()) {
            qCDebug(OkularCoreDebug) << "Mediaobject:" << data->m_mediaobject;
            int newid = newId();
            QObject::connect(data->m_mediaobject, &Phonon::MediaObject::finished, q, [this, newid]() { finished(newid); });
            data->m_buffer = new QBuffer();
            data->m_buffer->setData(filedata);
            data->m_mediaobject->setCurrentSource(Phonon::MediaSource(data->m_buffer));
            m_playing.insert(newid, data);
            valid = true;
        }
        break;
    }
    }
    if (!valid) {
        delete data;
        data = nullptr;
    }
    if (data) {
        qCDebug(OkularCoreDebug) << "PLAY";
        data->play();
        m_state = AudioPlayer::PlayingState;
    }
    return valid;
}

void AudioPlayerPrivate::stopPlayings()
{
    qDeleteAll(m_playing);
    m_playing.clear();
    m_state = AudioPlayer::StoppedState;
}

void AudioPlayerPrivate::finished(int id)
{
    QHash<int, PlayData *>::iterator it = m_playing.find(id);
    if (it == m_playing.end()) {
        return;
    }

    SoundInfo si = it.value()->m_info;
    // if the sound must be repeated indefinitely, then start the playback
    // again, otherwise destroy the PlayData as it's no more useful
    if (si.repeat) {
        it.value()->play();
    } else {
        delete it.value();
        m_playing.erase(it);
        m_state = AudioPlayer::StoppedState;
    }
    qCDebug(OkularCoreDebug) << "finished," << m_playing.count();
}

AudioPlayer::AudioPlayer()
    : QObject()
    , d(new AudioPlayerPrivate(this))
{
}

AudioPlayer::~AudioPlayer()
{
    delete d;
}

AudioPlayer *AudioPlayer::instance()
{
    static AudioPlayer ap;
    return &ap;
}

void AudioPlayer::playSound(const Sound *sound, const SoundAction *linksound)
{
    // we can't play null pointers ;)
    if (!sound) {
        return;
    }

    // we don't play external sounds for remote documents
    if (sound->soundType() == Sound::External && !d->m_currentDocument.isLocalFile()) {
        return;
    }

    qCDebug(OkularCoreDebug);
    SoundInfo si(sound, linksound);

    // if the mix flag of the new sound is false, then the currently playing
    // sounds must be stopped.
    if (!si.mix) {
        d->stopPlayings();
    }

    d->play(si);
}

void AudioPlayer::stopPlaybacks()
{
    d->stopPlayings();
}

AudioPlayer::State AudioPlayer::state() const
{
    return d->m_state;
}

void AudioPlayer::resetDocument()
{
    d->m_currentDocument = {};
}

void AudioPlayer::setDocument(const QUrl &url, Okular::Document *document)
{
    Q_UNUSED(document);
    d->m_currentDocument = url;
}

#else

namespace Okular
{
class AudioPlayerPrivate
{
public:
    Document *document;
};
}

AudioPlayer::AudioPlayer()
    : d(new AudioPlayerPrivate())
{
}

AudioPlayer *AudioPlayer::instance()
{
    static AudioPlayer ap;
    return &ap;
}

void AudioPlayer::playSound(const Sound *sound, const SoundAction *linksound)
{
    Q_UNUSED(sound);
    Q_UNUSED(linksound);
    Q_EMIT d->document->warning(i18n("This Okular is built without audio support"), 2000);
}

AudioPlayer::State Okular::AudioPlayer::state() const
{
    return State::StoppedState;
}

void AudioPlayer::stopPlaybacks()
{
}

AudioPlayer::~AudioPlayer() noexcept
{
}

void AudioPlayer::resetDocument()
{
    d->document = nullptr;
}

void AudioPlayer::setDocument(const QUrl &url, Okular::Document *document)
{
    Q_UNUSED(url);
    d->document = document;
}

#endif

#include "moc_audioplayer.cpp"
