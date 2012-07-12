/***************************************************************************
 *   Copyright (C) 2012 by Mailson Menezes <mailson@gmail.com>             *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TILES_MANAGER_H_
#define _OKULAR_TILES_MANAGER_H_

#include "okular_export.h"
#include "area.h"

class QPixmap;

namespace Okular {

class OKULAR_EXPORT Tile
{
    public:
        Tile();

        bool isValid() const;

        NormalizedRect rect;
        QPixmap *pixmap;
        uint dirty : 1;
};

class OKULAR_EXPORT TilesManager
{
    public:
        TilesManager( int width, int height );
        virtual ~TilesManager();

        void setPixmap( const QPixmap *pixmap, const NormalizedRect &rect );
        bool hasPixmap( const NormalizedRect &rect );
        QList<Tile> tilesAt( const NormalizedRect &rect ) const;

        inline int width() const { return m_width; }
        void setWidth( int width );
        inline int height() const { return m_height; }
        void setHeight( int height );

    private:
        Tile m_tiles[16];
        int m_width;
        int m_height;
};

}

#endif // _OKULAR_TILES_MANAGER_H_
