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

#include "area.h"
#include "okularcore_export.h"

class QRect;
class QImage;
class QWidget;

namespace Okular
{
/**
 * @short General utility functions.
 *
 * This class contains some static functions of general utility.
 */
class OKULARCORE_EXPORT Utils
{
public:
    /**
     * Rotate the rect \p source in the area \p width x \p height with the
     * specified \p orientation .
     */
    static QRect rotateRect(const QRect &source, int width, int height, int orientation); // TODO remove the & when we do a BIC change elsewhere

    /**
     * Return the real DPI of the display containing given widget
     *
     * On X11, it can indicate the real horizontal DPI value without any Xrdb
     * setting. Otherwise, returns the same as realDpiX/Y(),
     *
     * @since 0.19 (KDE 4.13)
     */
    static QSizeF realDpi(QWidget *widgetOnScreen);

    /**
     * Compute the smallest rectangle that contains all non-white pixels in image),
     * in normalized [0,1] coordinates.
     *
     * @since 0.7 (KDE 4.1)
     */
    static NormalizedRect imageBoundingBox(const QImage *image);
};

}

#endif

/* kate: replace-tabs on; indent-width 4; */
