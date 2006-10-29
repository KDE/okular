/***************************************************************************
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_INTERPETER_H_
#define _OKULAR_INTERPETER_H_
#include <qgs.h>
#include <QX11Info>

namespace DPIMod
{
        const float X = QX11Info::appDpiX()/72.0;
        const float Y = QX11Info::appDpiY()/72.0;
}

#endif
