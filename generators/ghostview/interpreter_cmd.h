/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *   Copyright (C) 2007 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GSINTERPETERCMD_H_
#define _OKULAR_GSINTERPETERCMD_H_

#include <qthread.h>
#include <qsemaphore.h>

#include <kdebug.h>

#include "internaldocument.h"

class QImage;
class QString;

namespace Okular {
    class PixmapRequest;
}

class GSHandler;

class GSInterpreterCMD : public QThread
{
Q_OBJECT
    public:
        static GSInterpreterCMD *getCreateInterpreter();

        ~GSInterpreterCMD();

        void setFileName(const QString &filename);
        void setPlatformFonts(bool pfonts);
        void setAABits(int text, int graphics);
        void setMagnify(double magnify);
        void setMedia(const QString &media);
        void setStructure(const PsPosition &prolog, const PsPosition &setup);
        void setPosition(const PsPosition &pos);

        void fordwardImage(QImage *image);

        void startRequest(Okular::PixmapRequest *request);

    signals:
        void imageDone(QImage *image, Okular::PixmapRequest *request);

    private:
        GSInterpreterCMD();

        QSemaphore m_semaphore;

        static GSInterpreterCMD *theInterpreter;

        void run();

        PsPosition m_position;

        bool m_sendStructure;
        PsPosition m_structureData[2];

        double m_magnify;
        int m_aaText, m_aaGfx;
        bool m_pfonts;

        QString m_filename;
        QString m_media;

        GSHandler *m_handler;
        Okular::PixmapRequest *m_request;
};

#endif
