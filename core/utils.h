/***************************************************************************
 *   Copyright (C) 2006 by Luigi Toscano <luigi.toscano@tiscali.it>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_UTILS_H_
#define _OKULAR_UTILS_H_

#include "okular_export.h"

class QRect;

namespace Okular
{

/**
 * @short General utility functions.
 *
 * This class contains some static functions of general utility.
 */
class OKULAR_EXPORT Utils
{
  public:
    /**
     * Rotate the rect \p source in the area \p width x \p height with the
     * specified \p orientation .
     */
    static QRect rotateRect( const QRect & source, int width, int height, int orientation );

    /**
     * Return the horizontal DPI of the main display
     */
    static double dpiX();

    /**
     * Return the vertical DPI of the main display
     */
    static double dpiY();
};

}

#endif
