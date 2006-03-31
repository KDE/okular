/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgperformancebase.h"

#include "dlgperformance.h"

DlgPerformance::DlgPerformance( QWidget * parent )
    : QWidget( parent )
{
    m_dlg = new Ui_DlgPerformanceBase();
    m_dlg->setupUi( this );

    connect( m_dlg->lowRadio, SIGNAL( toggled( bool ) ), this, SLOT( lowRadio_toggled( bool ) ) );
    connect( m_dlg->normalRadio, SIGNAL( toggled( bool ) ), this, SLOT( normalRadio_toggled( bool ) ) );
    connect( m_dlg->aggressiveRadio, SIGNAL( toggled( bool ) ), this, SLOT( aggressiveRadio_toggled( bool ) ) );
}

void DlgPerformance::lowRadio_toggled( bool on )
{
    if ( on )
        m_dlg->descLabel->setText( i18n("Keeps used memory as low as possible. Do not reuse anything. (For systems with low memory.)") );
}

void DlgPerformance::normalRadio_toggled( bool on )
{
    if ( on )
        m_dlg->descLabel->setText( i18n("A good compromise between memory usage and speed gain. Preload next page and boost searches. (For systems with 256MB of memory, typically.)") );
}

void DlgPerformance::aggressiveRadio_toggled( bool on )
{
    if ( on )
        m_dlg->descLabel->setText( i18n("Keeps everything in memory. Preload next pages. Boost searches. (For systems with more than 512MB of memory.)") );
}

#include "dlgperformance.moc"
