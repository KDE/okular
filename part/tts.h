/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _TTS_H_
#define _TTS_H_

#include <QObject>
#include <QTextToSpeech>

class OkularTTS : public QObject
{
    Q_OBJECT
public:
    explicit OkularTTS(QObject *parent = nullptr);
    ~OkularTTS() override;

    void say(const QString &text);
    void stopAllSpeechs();
    void pauseResumeSpeech();

public slots:
    void slotSpeechStateChanged(QTextToSpeech::State state);
    void slotConfigChanged();

signals:
    void isSpeaking(bool speaking);
    void canPauseOrResume(bool speakingOrPaused);

private:
    // private storage
    class Private;
    Private *d;
};

#endif
