/***************************************************************************
 *   Copyright (C) 2007 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GSRENDERERTHREAD_H_
#define _OKULAR_GSRENDERERTHREAD_H_

#include <qsemaphore.h>
#include <qstring.h>
#include <qthread.h>

#include <libspectre/spectre.h>

class QImage;

namespace Okular
{
   class PixmapRequest;
}

class GSRendererThread : public QThread
{
Q_OBJECT
    public:
        static GSRendererThread *getCreateRenderer();

        ~GSRendererThread();

        void setPlatformFonts(bool pfonts);
        void setAABits(int text, int graphics);
        void setMagnify(double magnify);
        void setRotation(int rotation);

        void startRequest(Okular::PixmapRequest *request, SpectrePage *page);

    signals:
        void imageDone(QImage *image, Okular::PixmapRequest *request);

    private:
        GSRendererThread();

        QSemaphore m_semaphore;

        static GSRendererThread *theRenderer;

        void run();

        SpectreRenderContext *m_renderContext;
        Okular::PixmapRequest *m_currentRequest, *m_nextRequest;
        SpectrePage *m_currentPage, *m_nextPage;
};

#endif
