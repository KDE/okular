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

/** class KPDFPageTransition **/

KPDFPageTransition::KPDFPageTransition( Type type )
  : m_type( type ),
    m_duration( 1 ),
    m_alignment( Horizontal ),
    m_direction( Inward ),
    m_angle( 0 ),
    m_scale( 1.0 ),
    m_rectangular( false )
{
}

KPDFPageTransition::~KPDFPageTransition()
{
}
