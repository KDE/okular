/***************************************************************************
 *   Copyright (C) 2013 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_PART_EXPORT_H
#define OKULAR_PART_EXPORT_H

/* needed for KDE_EXPORT macros */
#include <kdemacros.h>


#if defined _WIN32 || defined _WIN64
#ifndef OKULAR_PART_EXPORT
# ifdef MAKE_OKULARPART_LIB
#  define OKULAR_PART_EXPORT KDE_EXPORT
# else
#  define OKULAR_PART_EXPORT KDE_IMPORT
# endif
#endif

#else /* UNIX*/


/* export statements for unix */
#define OKULAR_PART_EXPORT KDE_EXPORT
#endif

#endif
