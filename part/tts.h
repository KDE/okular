/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
