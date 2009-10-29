/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_VERSION_H_
#define _OKULAR_VERSION_H_

#define OKULAR_VERSION_STRING "0.9.3"
#define OKULAR_VERSION_MAJOR 0
#define OKULAR_VERSION_MINOR 9
#define OKULAR_VERSION_RELEASE 3
#define OKULAR_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))

#define OKULAR_VERSION \
  OKULAR_MAKE_VERSION(OKULAR_VERSION_MAJOR,OKULAR_VERSION_MINOR,OKULAR_VERSION_RELEASE)

#define OKULAR_IS_VERSION(a,b,c) ( OKULAR_VERSION >= OKULAR_MAKE_VERSION(a,b,c) )

#endif
