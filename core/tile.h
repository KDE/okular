/***************************************************************************
 *   Copyright (C) 2012 by Fabio D'Urso <fabiodurso@hotmail.it>            *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TILE_H_
#define _OKULAR_TILE_H_

#include "area.h"

class QPixmap;

namespace Okular
{
/**
 * This class represents a rectangular portion of a page.
 *
 * It doesn't take ownership of pixmap
 *
 * @since 0.16 (KDE 4.10)
 */
class OKULARCORE_EXPORT Tile
{
public:
    Tile(const NormalizedRect &rect, QPixmap *pixmap, bool isValid);
    Tile(const Tile &t);
    ~Tile();

    /**
     * Location of the tile
     */
    NormalizedRect rect() const;

    /**
     * Pixmap (may also be NULL)
     */
    QPixmap *pixmap() const;

    /**
     * True if the pixmap is available and updated
     */
    bool isValid() const;

    Tile &operator=(const Tile &other);

private:
    class Private;
    Private *d;
};

}

#endif // _OKULAR_TILE_H_
