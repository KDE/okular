/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qcheckbox.h>
#include <qlayout.h>

#include "dlgdebug.h"

DlgDebug::DlgDebug( QWidget * parent )
    : QWidget( parent )
{
    QVBoxLayout * lay = new QVBoxLayout( this );
    lay->setMargin( 0 );

    QCheckBox * drawBoundaries = new QCheckBox( "DebugDrawBoundaries", this );
    drawBoundaries->setObjectName( "kcfg_DebugDrawBoundaries" );
    lay->addWidget( drawBoundaries );

    QCheckBox * drawAnnRect = new QCheckBox( "DebugDrawAnnotationRect", this );
    drawAnnRect->setObjectName( "kcfg_DebugDrawAnnotationRect" );
    lay->addWidget( drawAnnRect );

    lay->addItem( new QSpacerItem( 5, 5, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding ) );
}
