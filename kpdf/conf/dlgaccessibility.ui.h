/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "settings.h"


void DlgAccessibility::init()
{
    switch ( Settings::renderMode() )
    {
    case Settings::EnumRenderMode::Normal:
	radioNormal->setChecked( true );
	break;
    case Settings::EnumRenderMode::Inverted:
	radioInverted->setChecked( true );
	break;
    case Settings::EnumRenderMode::Recolor:
	radioRecolor->setChecked( true );
	break;
    case Settings::EnumRenderMode::BlackWhite:
	radioContrast->setChecked( true );	
	break;
    }
}

void DlgAccessibility::radioNormal_toggled( bool on )
{
    if ( on ) Settings::setRenderMode( Settings::EnumRenderMode::Normal );
}

void DlgAccessibility::radioInverted_toggled( bool on )
{
    if ( on ) Settings::setRenderMode( Settings::EnumRenderMode::Inverted );
}

void DlgAccessibility::radioRecolor_toggled( bool on )
{
    if ( on ) Settings::setRenderMode( Settings::EnumRenderMode::Recolor );
}

void DlgAccessibility::radioContrast_toggled( bool on )
{
    if ( on ) Settings::setRenderMode( Settings::EnumRenderMode::BlackWhite );
}
