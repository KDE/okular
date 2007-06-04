/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *   Copyright (C) 2007 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "interpreter_cmd.h"

#include <qimage.h>

#include <kdebug.h>

#include "gshandler.h"

#include "core/generator.h"

GSInterpreterCMD *GSInterpreterCMD::theInterpreter = 0;

GSInterpreterCMD *GSInterpreterCMD::getCreateInterpreter()
{
    if (!theInterpreter)
    {
        theInterpreter = new GSInterpreterCMD();
    }

    return theInterpreter;
}

GSInterpreterCMD::GSInterpreterCMD() :
    m_sendStructure   ( false ),
    m_magnify         ( 1 ),
    m_aaText          ( 1 ),
    m_aaGfx           ( 1 ),
    m_pfonts          ( false ),
    m_handler         ( 0 )
{
}

GSInterpreterCMD::~GSInterpreterCMD()
{
}

void GSInterpreterCMD::setFileName(const QString &filename)
{
    m_filename = filename;
}

void GSInterpreterCMD::setStructure(const PsPosition &prolog, const PsPosition &setup)
{
    m_sendStructure = true;
    m_structureData[0] = prolog;
    m_structureData[1] = setup;
}

void GSInterpreterCMD::setMagnify( double magnify )
{
    m_magnify = magnify;
}

void GSInterpreterCMD::setMedia( const QString &media )
{
    m_media = media;
}

void GSInterpreterCMD::setPlatformFonts(bool pfonts)
{
    m_pfonts = pfonts;
}

void GSInterpreterCMD::setAABits(int text, int graphics)
{
    m_aaText = text;
    m_aaGfx = graphics;
}

void GSInterpreterCMD::setPosition(const PsPosition &pos)
{
    m_position = pos;
}

void GSInterpreterCMD::fordwardImage(QImage *image)
{
    if (image->width() != m_request->width() || image->height() != m_request->height())
    {
        kDebug(4656) << "Generated image does not match wanted size " << image->width() << " " << m_request->width() << " " << image->height() << " " << m_request->height() << endl;
        Qt::TransformationMode transformMode;
        if (m_aaText == 4 && m_aaGfx == 2) transformMode = Qt::SmoothTransformation;
        else transformMode = Qt::FastTransformation;
        QImage aux = image->scaled(m_request->width(), m_request->height(), Qt::IgnoreAspectRatio, transformMode);
        delete image;
        image = new QImage(aux);
    }

    emit imageDone(image, m_request);
}

void GSInterpreterCMD::startRequest(Okular::PixmapRequest *request)
{
    m_request = request;
    m_lock.unlock();
}

void GSInterpreterCMD::run()
{
    m_handler = new GSHandler();

    m_lock.lock();

    while(1)
    {
        m_lock.lock();
        m_handler->init(m_media, m_magnify, m_pfonts, m_aaText, m_aaGfx, this);

        // send structural information
        if (m_sendStructure)
        {
            m_handler->process(m_filename, m_structureData[0]);
            m_handler->process(m_filename, m_structureData[1]);
        }
        m_handler->process(m_filename, m_position);
    }
}

#include "interpreter_cmd.moc"
