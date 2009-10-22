/***************************************************************************
 *   Copyright (C) 2007 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "rendererthread.h"

#include <qimage.h>

#include <kdebug.h>

#include "core/generator.h"
#include "core/page.h"
#include "core/utils.h"

GSRendererThread *GSRendererThread::theRenderer = 0;

GSRendererThread *GSRendererThread::getCreateRenderer()
{
    if (!theRenderer) theRenderer = new GSRendererThread();
    return theRenderer;
}

GSRendererThread::GSRendererThread()
{
    m_renderContext = spectre_render_context_new();
}

GSRendererThread::~GSRendererThread()
{
    spectre_render_context_free(m_renderContext);
}

void GSRendererThread::addRequest(const GSRendererThreadRequest &req)
{
    m_queueMutex.lock();
    m_queue.enqueue(req);
    m_queueMutex.unlock();
    m_semaphore.release();
}

void GSRendererThread::run()
{
    while(1)
    {
        m_semaphore.acquire();
        bool goAhead = true;
        do
        {
            m_queueMutex.lock();
            GSRendererThreadRequest req = m_queue.dequeue();
            goAhead = !m_queue.isEmpty();
            m_queueMutex.unlock();

            spectre_render_context_set_scale(m_renderContext, req.magnify, req.magnify);
            spectre_render_context_set_use_platform_fonts(m_renderContext, req.platformFonts);
            spectre_render_context_set_antialias_bits(m_renderContext, req.graphicsAAbits, req.textAAbits);
            spectre_render_context_set_rotation(m_renderContext, req.rotation);

            unsigned char *data = NULL;
            int row_length = 0;
            int wantedWidth = req.request->width();
            int wantedHeight = req.request->height();

            spectre_page_render(req.spectrePage, m_renderContext, &data, &row_length);

            QImage img;
            if (row_length == wantedWidth * 4)
            {
                img = QImage(data, wantedWidth, wantedHeight, QImage::Format_RGB32);
            }
            else
            {
                // In case this ends up beign very slow we can try with some memmove
                QImage aux(data, row_length / 4, wantedHeight, QImage::Format_RGB32);
                img = QImage(aux.copy(0, 0, wantedWidth, wantedHeight));
            }
            QImage *image = new QImage(img.copy());
            free(data);

            if (image->width() != wantedWidth || image->height() != wantedHeight)
            {
                kWarning(4711).nospace() << "Generated image does not match wanted size: "
                    << "[" << image->width() << "x" << image->height() << "] vs requested "
                    << "[" << req.request->width() << "x" << req.request->height() << "]";
                QImage aux = image->scaled(wantedWidth, wantedHeight);
                delete image;
                image = new QImage(aux);
            }
            emit imageDone(image, req.request);

            spectre_page_free(req.spectrePage);

        } while (goAhead);
    }
}

#include "rendererthread.moc"
