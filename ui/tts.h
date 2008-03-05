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

#include <qobject.h>

class PageViewMessage;

class OkularTTS : public QObject
{
    Q_OBJECT
    public:
        OkularTTS( PageViewMessage *messageWindow, QObject *parent = 0 );
        ~OkularTTS();

        void say( const QString &text );

    private slots:
        void slotServiceUnregistered( const QString& );
        void slotServiceOwnerChanged( const QString&, const QString&, const QString& );

    private:
        // private storage
        class Private;
        Private *d;
};

#endif
