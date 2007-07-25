/***************************************************************************
 *   Copyright (C) 2005 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// local includes
#include "pagetransition.h"

using namespace Okular;

/** class Okular::PageTransition **/

class PageTransition::Private
{
    public:
        Private( Type type )
            : m_type( type ),
              m_duration( 1 ),
              m_alignment( Horizontal ),
              m_direction( Inward ),
              m_angle( 0 ),
              m_scale( 1.0 ),
              m_rectangular( false )
        {
        }

        Type m_type;
        int m_duration;
        Alignment m_alignment;
        Direction m_direction;
        int m_angle;
        double m_scale;
        bool m_rectangular;
};

PageTransition::PageTransition( Type type )
    : d( new Private( type ) )
{
}

PageTransition::PageTransition( const PageTransition &other )
    : d( new Private( *other.d ) )
{
}

PageTransition& PageTransition::operator=( const PageTransition &other )
{
    if ( this == &other )
        return *this;

    *d = *other.d;

    return *this;
}

PageTransition::~PageTransition()
{
    delete d;
}

PageTransition::Type PageTransition::type() const
{
    return d->m_type;
}

int PageTransition::duration() const
{
    return d->m_duration;
}

PageTransition::Alignment PageTransition::alignment() const
{
    return d->m_alignment;
}

PageTransition::Direction PageTransition::direction() const
{
    return d->m_direction;
}

int PageTransition::angle() const
{
    return d->m_angle;
}

double PageTransition::scale() const
{
    return d->m_scale;
}

bool PageTransition::isRectangular() const
{
    return d->m_rectangular;
}

void PageTransition::setType( Type type )
{
    d->m_type = type;
}

void PageTransition::setDuration( int duration )
{
    d->m_duration = duration;
}

void PageTransition::setAlignment( Alignment alignment )
{
    d->m_alignment = alignment;
}

void PageTransition::setDirection( Direction direction )
{
    d->m_direction = direction;
}

void PageTransition::setAngle( int angle )
{
    d->m_angle = angle;
}

void PageTransition::setScale( double scale )
{
    d->m_scale = scale;
}

void PageTransition::setIsRectangular( bool rectangular )
{
    d->m_rectangular = rectangular;
}

