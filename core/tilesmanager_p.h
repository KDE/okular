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

#define TILES_MAXSIZE 2000000

class QPixmap;

namespace Okular {

/**
 * Tile is a class that stores the pixmap of a tile and its location on the
 * page
 */
class OKULAR_EXPORT Tile
{
    public:
        Tile();

        /**
         * True if the pixmap is available and updated
         */
        bool isValid() const;

        /**
         * Location of the tile
         */
        NormalizedRect rect;
        /**
         * Pixmap (may also be NULL)
         */
        QPixmap *pixmap;
        /**
         * Whether the tile needs to be repainted (after a zoom or rotation)
         * If a tile doesn't have a pixmap but all its children are updated
         * (dirty = false), the parent tile is also considered updated.
         */
        bool dirty : 1;

        /**
         * Children tiles
         */
        Tile *tiles;
        int nTiles;

        Tile *parent;

        /**
         * Hit/miss counter.
         * Increased whenever the tile is not referenced. Decreased whenever a
         * reference to the tile is made
         * Used by the ranking algorithm.
         */
        int miss;
};

/**
 * @short Tiles management
 *
 * This class has direct access to all tiles and handle how they should be
 * stored, deleted and retrieved.
 *
 * The tiles manager is a tree of tiles. At first the page is divided in 16
 * tiles. Then each one of these tiles can be split in more tiles (or merged
 * back to only one).
 */
class OKULAR_EXPORT TilesManager
{
    public:
        TilesManager( int width, int height, Rotation rotation = Rotation0 );
        virtual ~TilesManager();

        /**
         * Use @p pixmap to paint all tiles that are contained inside @p rect
         */
        void setPixmap( const QPixmap *pixmap, const NormalizedRect &rect );

        /**
         * Checks whether all tiles intersecting with @p rect are available.
         * Returns false if at least one tile needs to be repainted (the tile
         * is dirty).
         */
        bool hasPixmap( const NormalizedRect &rect );

        /**
         * Returns a list of all tiles intersecting with @p rect.
         *
         * @param allowEmpty If false only tiles with a non null pixmap are returned
         */
        QList<Tile> tilesAt( const NormalizedRect &rect, bool allowEmpty = true );

        /**
         * The total memory consumed by the tiles manager
         */
        long totalMemory() const;

        /**
         * Removes the least ranked tiles from memory
         */
        void cleanupPixmapMemory( qulonglong numberOfBytes = 1 );

        /**
         * Checks whether a given region has already been requested
         */
        bool isRequesting( const NormalizedRect &rect, int pageWidth, int pageHeight ) const;

        /**
         * Sets a region to be requested so the tiles manager knows which
         * pixmaps to expect and discard those not useful anymore (late pixmaps)
         */
        void setRequest( const NormalizedRect &rect, int pageWidth, int pageHeight );

        /**
         * Inform the new width of the page and mark all tiles to repaint
         */
        void setWidth( int width );
        int width() const;
        void setHeight( int height );
        int height() const;

        /**
         * Perform a rotation and mark all tiles to repaint.
         */
        void setRotation( Rotation rotation );
        Rotation rotation() const;

        /**
         * Sets the visible area of the page
         */
        void setVisibleRect( const NormalizedRect &rect );

        /**
         * Returns the visible area of the page
         */
        NormalizedRect visibleRect() const;

        /**
         * Returns a rotated NormalizedRect given a @p rotation
         */
        static NormalizedRect toRotatedRect( const NormalizedRect &rect, Rotation rotation );

        /**
         * Returns a non rotated version of @p rect, which is rotated by @p rotation
         */
        static NormalizedRect fromRotatedRect( const NormalizedRect &rect, Rotation rotation );

    private:
        class Private;
        Private * const d;
        friend class Private;
};

}

#endif // _OKULAR_TILES_MANAGER_H_
