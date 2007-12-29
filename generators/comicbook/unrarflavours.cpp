/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "unrarflavours.h"

#include <QtCore/QRegExp>
#include <QtCore/QStringList>

UnrarFlavour::UnrarFlavour()
{
}

UnrarFlavour::~UnrarFlavour()
{
}

void UnrarFlavour::setFileName( const QString &fileName )
{
    mFileName = fileName;
}

QString UnrarFlavour::fileName() const
{
    return mFileName;
}


NonFreeUnrarFlavour::NonFreeUnrarFlavour()
    : UnrarFlavour()
{
}

QStringList NonFreeUnrarFlavour::processListing( const QStringList &data )
{
    // unrar-nonfree just lists the files
    return data;
}

QString NonFreeUnrarFlavour::name() const
{
    return "unrar-nonfree";
}


FreeUnrarFlavour::FreeUnrarFlavour()
    : UnrarFlavour()
{
}

QStringList FreeUnrarFlavour::processListing( const QStringList &data )
{
    QRegExp re( "^ ([^/]+/([^\\s]+))$" );

    QStringList newdata;
    foreach ( const QString &line, data )
    {
        if ( re.exactMatch( line ) )
            newdata.append( re.cap( 1 ) );
    }
    return newdata;
}

QString FreeUnrarFlavour::name() const
{
    return "unrar-free";
}

