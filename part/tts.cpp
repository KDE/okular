/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "tts.h"

#include <QDBusServiceWatcher>
#include <QSet>

#include <KLocalizedString>

#include "settings.h"

/* Private storage. */
class OkularTTS::Private
{
public:
    Private(OkularTTS *qq)
        : q(qq)
        , speech(new QTextToSpeech(Okular::Settings::ttsEngine()))
    {
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
    connect(Okular::Settings::self(), &KConfigSkeleton::configChanged, this, &OkularTTS::slotConfigChanged);
}

OkularTTS::~OkularTTS()
{
    delete d;
}

void OkularTTS::say(const QString &text)
{
    if (text.isEmpty())
        return;

    d->speech->say(text);
}

void OkularTTS::stopAllSpeechs()
{
    if (!d->speech)
        return;

    d->speech->stop();
}

void OkularTTS::pauseResumeSpeech()
{
    if (!d->speech)
        return;

    if (d->speech->state() == QTextToSpeech::Speaking)
        d->speech->pause();
    else
        d->speech->resume();
}

void OkularTTS::slotSpeechStateChanged(QTextToSpeech::State state)
{
    if (state == QTextToSpeech::Speaking) {
        emit isSpeaking(true);
        emit canPauseOrResume(true);
    } else {
        emit isSpeaking(false);
        if (state == QTextToSpeech::Paused)
            emit canPauseOrResume(true);
        else
            emit canPauseOrResume(false);
    }
}

void OkularTTS::slotConfigChanged()
{
    const QString engine = Okular::Settings::ttsEngine();
    if (engine != d->speechEngine) {
        d->speech->stop();
        delete d->speech;
        d->speech = new QTextToSpeech(engine);
        connect(d->speech, &QTextToSpeech::stateChanged, this, &OkularTTS::slotSpeechStateChanged);
        d->speechEngine = engine;
    }
}
