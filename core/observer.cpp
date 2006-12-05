/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2005 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "observer.h"

using namespace Okular;

DocumentObserver::~DocumentObserver()
{
}

void DocumentObserver::notifySetup( const QVector< Okular::Page * >&, bool )
{
}

void DocumentObserver::notifyViewportChanged( bool )
{
}

void DocumentObserver::notifyPageChanged( int, int )
{
}

void DocumentObserver::notifyContentsCleared( int )
{
}

void DocumentObserver::notifyVisibleRectsChanged()
{
}

bool DocumentObserver::canUnloadPixmap( int ) const
{
    return true;
}
