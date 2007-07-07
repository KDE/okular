/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// local includes
#include "fontinfo.h"

using namespace Okular;

class Okular::FontInfoPrivate
    : public QSharedData
{
    public:
        FontInfoPrivate()
          : name( 0 ), type( FontInfo::Unknown ), embedded( true )
        {
        }

        bool operator==( const FontInfoPrivate &rhs ) const
        {
            return name == rhs.name &&
                   type == rhs.type &&
                   embedded == rhs.embedded &&
                   file == rhs.file;
        }

        QString name;
        FontInfo::FontType type;
        bool embedded;
        QString file;
};


FontInfo::FontInfo()
    : d( new FontInfoPrivate )
{
}

FontInfo::FontInfo( const FontInfo &fi )
    : d( fi.d )
{
}

FontInfo::~FontInfo()
{
}

QString FontInfo::name() const
{
    return d->name;
}

void FontInfo::setName( const QString& name )
{
    d->name = name;
}

FontInfo::FontType FontInfo::type() const
{
    return d->type;
}

void FontInfo::setType( FontInfo::FontType type )
{
    d->type = type;
}

bool FontInfo::isEmbedded() const
{
    return d->embedded;
}

void FontInfo::setEmbedded( bool embedded )
{
    d->embedded = embedded;
}

QString FontInfo::file() const
{
    return d->file;
}

void FontInfo::setFile( const QString& file )
{
    d->file = file;
}

bool FontInfo::operator==( const FontInfo &fi ) const
{
    return *d == *fi.d;
}

bool FontInfo::operator!=( const FontInfo &fi ) const
{
    return !operator==( fi );
}

FontInfo& FontInfo::operator=( const FontInfo &fi )
{
    if ( this == &fi )
        return *this;

    d = fi.d;
    return *this;
}

