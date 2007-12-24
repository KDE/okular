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

void GSRendererThread::setMagnify(double magnify)
{
    spectre_render_context_set_scale(m_renderContext, magnify);
}

void GSRendererThread::setPlatformFonts(bool pfonts)
{
    spectre_render_context_set_use_platform_fonts(m_renderContext, pfonts);
}

void GSRendererThread::setAABits(int text, int graphics)
{
    spectre_render_context_set_antialias_bits(m_renderContext, graphics, text);
}

void GSRendererThread::setRotation(int rotation)
{
    spectre_render_context_set_rotation(m_renderContext, rotation);
}

void GSRendererThread::startRequest(Okular::PixmapRequest *request, SpectrePage *page)
{
    m_nextRequest = request;
    m_nextPage = page;
    m_semaphore.release();
}

void GSRendererThread::run()
{
    while(1)
    {
        m_semaphore.acquire();
        m_currentRequest = m_nextRequest;
        m_currentPage = m_nextPage;

        unsigned char   *data = NULL;
        int              row_length;

        int wantedWidth = m_currentRequest->width();
        int wantedHeight = m_currentRequest->height();

        spectre_page_render(m_currentPage, m_renderContext, &data, &row_length);

        QImage img;
        if (row_length == wantedWidth * 4)
        {
            img = QImage(data, wantedWidth, wantedHeight, QImage::Format_RGB32 );
        }
        else
        {
            // In case this ends up beign very slow we can try with some memmove
            QImage aux(data, row_length / 4, wantedHeight, QImage::Format_RGB32 );
            img = QImage(aux.copy(0, 0, wantedWidth, wantedHeight));
        }
        QImage *image = new QImage(img.copy());
        free(data);

        if (image->width() != wantedWidth || image->height() != wantedHeight)
        {
            kWarning(4711) << "Generated image does not match wanted size " << image->width() << " " << m_currentRequest->width() << " " << image->height() << " " << m_currentRequest->height() ;
            QImage aux = image->scaled(wantedWidth, wantedHeight);
            delete image;
            image = new QImage(aux);
        }
        emit imageDone(image, m_currentRequest);

        spectre_page_free(m_currentPage);
    }
}

#include "rendererthread.moc"
