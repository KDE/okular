/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "tts.h"

#include <QSet>

#include <KLocalizedString>

#include "settings.h"

/* Private storage. */
class OkularTTS::Private
{
public:
    explicit Private(OkularTTS *qq)
        : q(qq)
        , speech(new QTextToSpeech(Okular::Settings::ttsEngine()))
    {
        const QVector<QVoice> voices = speech->availableVoices();
        QString voiceName = Okular::Settings::ttsVoice();
        for (const QVoice &voice : voices) {
            if (voice.name() == voiceName) {
                speech->setVoice(voice);
            }
        }
    }

    ~Private()
    {
        delete speech;
        speech = nullptr;
    }

    OkularTTS *q;
    QTextToSpeech *speech;
    // Which speech engine was used when above object was created.
    // When the setting changes, we need to stop speaking and recreate.
    QString speechEngine;
};

OkularTTS::OkularTTS(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
    // Initialize speechEngine so we can reinitialize if it changes.
    d->speechEngine = Okular::Settings::ttsEngine();
    connect(d->speech, &QTextToSpeech::stateChanged, this, &OkularTTS::slotSpeechStateChanged);
    connect(Okular::Settings::self(), &KCoreConfigSkeleton::configChanged, this, &OkularTTS::slotConfigChanged);
}

OkularTTS::~OkularTTS()
{
    delete d;
}

void OkularTTS::say(const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    d->speech->say(text);
}

void OkularTTS::stopAllSpeechs()
{
    if (!d->speech) {
        return;
    }

    d->speech->stop();
}

void OkularTTS::pauseResumeSpeech()
{
    if (!d->speech) {
        return;
    }

    if (d->speech->state() == QTextToSpeech::Speaking) {
        d->speech->pause();
    } else {
        d->speech->resume();
    }
}

void OkularTTS::slotSpeechStateChanged(QTextToSpeech::State state)
{
    if (state == QTextToSpeech::Speaking) {
        Q_EMIT isSpeaking(true);
        Q_EMIT canPauseOrResume(true);
    } else {
        Q_EMIT isSpeaking(false);
        if (state == QTextToSpeech::Paused) {
            Q_EMIT canPauseOrResume(true);
        } else {
            Q_EMIT canPauseOrResume(false);
        }
    }
}

void OkularTTS::slotConfigChanged()
{
    const QString engine = Okular::Settings::ttsEngine();
    const QString voiceName = Okular::Settings::ttsVoice();
    if (engine != d->speechEngine) {
        d->speech->stop();
        delete d->speech;
        d->speech = new QTextToSpeech(engine);
        connect(d->speech, &QTextToSpeech::stateChanged, this, &OkularTTS::slotSpeechStateChanged);
        d->speechEngine = engine;
    }

    const QVector<QVoice> voices = d->speech->availableVoices();
    for (const QVoice &voice : voices) {
        if (voice.name() == voiceName) {
            d->speech->setVoice(voice);
            break;
        }
    }
}
