/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <klocale.h>

// The purpose of this file is only to display a sort of descriptive text
// when the user clicks on each memory profile.

void DlgPerformance::init()
{
    QFont labelFont = descLabel->font();
    labelFont.setBold( true );
    descLabel->setFont( labelFont );
}

void DlgPerformance::lowRadio_toggled( bool on )
{
    if ( on )
        descLabel->setText( i18n("Keeps used memory as low as possible. Do not reuse anything. (For systems with low memory.)") );
}

void DlgPerformance::normalRadio_toggled( bool on )
{
    if ( on )
        descLabel->setText( i18n("A good compromise between memory usage and speed gain. Preload next page and boost searches. (For systems with 256MB of memory, typically.)") );
}

void DlgPerformance::aggressiveRadio_toggled( bool on )
{
    if ( on )
        descLabel->setText( i18n("Keeps everything in memory. Preload next pages. Boost searches. (For systems with more than 512MB of memory.)") );
}
