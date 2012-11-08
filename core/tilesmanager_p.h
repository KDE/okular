/***************************************************************************
 *   Copyright (C) 2012 by Mailson Menezes <mailson@gmail.com>             *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TILES_MANAGER_P_H_
#define _OKULAR_TILES_MANAGER_P_H_

#include "okular_export.h"
#include "area.h"

class QPixmap;

namespace Okular {

/**
 * This class is a node on the tree structure of tiles on tiles manager.
 * Each node stores the pixmap of the tile and its location on the page. Each
 * node may have children and together they cover the same area of the node
 * itself. Leaf tiles can have children if their size is bigger than an
 * arbitrary value.
 *
 * eg: Say the page is divided into 4 tiles: A, B, C and D. If the user apply a
 * zoom to part of the page covered by A, this tile may get too big and
 * eventually will be split into 4 different tiles: A1, A2, A3 and A4. In the
 * tree structure these are all children of the node A.
 *
 * @since 0.16 (KDE 4.10)
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
        bool dirty;
        /**
         * Distance between the tile and the viewport.
         * This is used by the evicting algorithm.
         */
        double distance;

        /**
         * Children tiles
         * When a tile is split into multiple tiles it doesn't cease to exist.
         * It actually adds children nodes to the data structure.
         * Since each tile is a node on the tree structure, it's easier to
         * consider if a large area should be evaluated without visiting all
         * its tiles (eg: when we need to list all tiles from an small area)
         */
        Tile *tiles;
        int nTiles;

        Tile *parent;
};

/**
 * @short Tiles management
 *
 * This class has direct access to all tiles and handle how they should be
 * stored, deleted and retrieved. Each tiles manager only handles one page.
 *
 * The tiles manager is a tree of tiles. At first the page is divided in 16
 * tiles. Then each one of these tiles can be split in more tiles (or merged
 * back to only one) so we keep the size of each pixmap of the tile inside a
 * safe interval.
 *
 * @since 0.16 (KDE 4.10)
 */
class OKULAR_EXPORT TilesManager
{
    public:
        TilesManager( int pageNumber, int width, int height, Rotation rotation = Rotation0 );
        virtual ~TilesManager();

        /**
         * Sets the pixmap of the tiles covered by @p rect (which represents
         * the location of @p pixmap on the page).
         * @p pixmap may cover an area which contains multiple tiles. So each
         * tile we get a cropped part of the @p pixmap.
         *
         * Also it checks the dimensions of the given parameters against the
         * current request as to avoid setting pixmaps of late requests.
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
         * As to avoid requests of big areas, each traversed tile is checked
         * for its size and split if necessary.
         *
         * @param allowEmpty If false only tiles with a non null pixmap are returned
         */
        QList<Tile> tilesAt( const NormalizedRect &rect, bool allowEmpty = true );

        /**
         * The total memory consumed by the tiles manager
         */
        long totalMemory() const;

        /**
         * Removes at least @p numberOfBytes bytes worth of tiles (least ranked
         * tiles are removed first).
         * Set @p visibleRect to the visible region of the page. Set a
         * @p visiblePageNumber if the current page is not visible.
         * Visible tiles are not discarded.
         */
        void cleanupPixmapMemory( qulonglong numberOfBytes, const NormalizedRect &visibleRect, int visiblePageNumber );

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
        /**
         * Gets the width of the page in tiles manager
         */
        int width() const;
        /**
         * Inform the new height of the page
         */
        void setHeight( int height );
        /**
         * Gets the height of the page in tiles manager
         */
        int height() const;

        /**
         * Inform the new rotation of the page and mark all tiles to repaint.
         */
        void setRotation( Rotation rotation );
        Rotation rotation() const;

        /**
         * Mark all tiles as dirty
         */
        void markDirty();

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

#endif // _OKULAR_TILES_MANAGER_P_H_
