/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// local includes
#include "pagesize.h"

using namespace Okular;

class PageSize::Private
{
    public:
        Private()
          : m_width( 0 ), m_height( 0 )
        {
        }

        double m_width;
        double m_height;
        QString m_name;
};


PageSize::PageSize()
    : d( new Private )
{
}

PageSize::PageSize( double width, double height, const QString &name )
    : d( new Private )
{
    d->m_width = width;
    d->m_height = height;
    d->m_name = name;
}

PageSize::PageSize( const PageSize &pageSize )
    : d( new Private( *pageSize.d ) )
{
}

PageSize::~PageSize()
{
    delete d;
}

double PageSize::width() const
{
    return d->m_width;
}

double PageSize::height() const
{
    return d->m_height;
}

QString PageSize::name() const
{
    return d->m_name;
}

bool PageSize::isNull() const
{
    return d->m_width == 0 && d->m_height == 0 && d->m_name.isEmpty();
}

bool PageSize::operator==( const PageSize &pageSize ) const
{
    return d->m_width == pageSize.d->m_width &&
           d->m_height == pageSize.d->m_height &&
           d->m_name == pageSize.d->m_name;
}

PageSize& PageSize::operator=( const PageSize &pageSize )
{
    if ( this == &pageSize )
        return *this;

    *d = *pageSize.d;
    return *this;
}

