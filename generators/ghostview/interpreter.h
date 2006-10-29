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

#include "core/utils.h"

namespace DPIMod
{
        const float X = Okular::Utils::getDpiX() / 72.0;
        const float Y = Okular::Utils::getDpiY() / 72.0;
}

#endif
