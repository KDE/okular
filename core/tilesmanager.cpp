/***************************************************************************
 *   Copyright (C) 2012 by Mailson Menezes <mailson@gmail.com>             *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "tilesmanager_p.h"

#include <QPixmap>
#include <QtCore/qmath.h>
#include <QList>

#define TILES_MAXSIZE 2000000

using namespace Okular;

static bool rankedTilesLessThan( TileNode *t1, TileNode *t2 )
{
    // Order tiles by its dirty state and then by distance from the viewport.
    if ( t1->dirty == t2->dirty )
        return t1->distance < t2->distance;

    return !t1->dirty;
}

class TilesManager::Private
{
    public:
        Private();

        bool hasPixmap( const NormalizedRect &rect, const TileNode &tile ) const;
        void tilesAt( const NormalizedRect &rect, TileNode &tile, QList<Tile> &result, bool allowEmpty );
        void setPixmap( const QPixmap *pixmap, const NormalizedRect &rect, TileNode &tile );

        /**
         * Mark @p tile and all its children as dirty
         */
        static void markDirty( TileNode &tile );

        /**
         * Deletes all tiles, recursively
         */
        void deleteTiles( const TileNode &tile );

        void markParentDirty( const TileNode &tile );
        void rankTiles( TileNode &tile, QList<TileNode*> &rankedTiles, const NormalizedRect &visibleRect, int visiblePageNumber );
        /**
         * Since the tile can be large enough to occupy a significant amount of
         * space, they may be split in more tiles. This operation is performed
         * when the tiles of a certain region is requested and they are bigger
         * than an arbitrary value. Only tiles intersecting the desired region
         * are split. There's no need to do this for the entire page.
         */
        void split( TileNode &tile, const NormalizedRect &rect );

        /**
         * Checks whether the tile's size is bigger than an arbitrary value and
         * performs the split operation returning true.
         * Otherwise it just returns false, without performing any operation.
         */
        bool splitBigTiles( TileNode &tile, const NormalizedRect &rect );

        // The page is split in a 4x4 grid of tiles
        TileNode tiles[16];
        int width;
        int height;
        int pageNumber;
        qulonglong totalPixels;
        Rotation rotation;
        NormalizedRect visibleRect;
        NormalizedRect requestRect;
        int requestWidth;
        int requestHeight;
};

TilesManager::Private::Private()
    : width( 0 )
    , height( 0 )
    , pageNumber( 0 )
    , totalPixels( 0 )
    , rotation( Rotation0 )
    , requestRect( NormalizedRect() )
    , requestWidth( 0 )
    , requestHeight( 0 )
{
}

TilesManager::TilesManager( int pageNumber, int width, int height, Rotation rotation )
    : d( new Private )
{
    d->pageNumber = pageNumber;
    d->width = width;
    d->height = height;
    d->rotation = rotation;

    // The page is split in a 4x4 grid of tiles
    const double dim = 0.25;
    for ( int i = 0; i < 16; ++i )
    {
        int x = i % 4;
        int y = i / 4;
        d->tiles[ i ].rect = NormalizedRect( x*dim, y*dim, x*dim+dim, y*dim+dim );
    }
}

TilesManager::~TilesManager()
{
    for ( int i = 0; i < 16; ++i )
        d->deleteTiles( d->tiles[ i ] );

    delete d;
}

void TilesManager::Private::deleteTiles( const TileNode &tile )
{
    if ( tile.pixmap )
    {
        totalPixels -= tile.pixmap->width()*tile.pixmap->height();
        delete tile.pixmap;
    }

    if ( tile.nTiles > 0 )
    {
        for ( int i = 0; i < tile.nTiles; ++i )
            deleteTiles( tile.tiles[ i ] );

        delete [] tile.tiles;
    }
}

void TilesManager::setWidth( int width )
{
    if ( width == d->width )
        return;

    d->width = width;

    markDirty();
}

int TilesManager::width() const
{
    return d->width;
}

void TilesManager::setHeight( int height )
{
    if ( height == d->height )
        return;

    d->height = height;
}

int TilesManager::height() const
{
    return d->height;
}

void TilesManager::setRotation( Rotation rotation )
{
    if ( rotation == d->rotation )
        return;

    d->rotation = rotation;

    markDirty();
}

Rotation TilesManager::rotation() const
{
    return d->rotation;
}

void TilesManager::markDirty()
{
    for ( int i = 0; i < 16; ++i )
    {
        TilesManager::Private::markDirty( d->tiles[ i ] );
    }
}

void TilesManager::Private::markDirty( TileNode &tile )
{
    tile.dirty = true;

    for ( int i = 0; i < tile.nTiles; ++i )
    {
        markDirty( tile.tiles[ i ] );
    }
}

void TilesManager::setPixmap( const QPixmap *pixmap, const NormalizedRect &rect )
{
    if ( !d->requestRect.isNull() )
    {
        if ( !(d->requestRect == rect) || rect.geometry( width(), height() ).size() != pixmap->size() )
            return;

        d->requestRect = NormalizedRect();
    }

    for ( int i = 0; i < 16; ++i )
    {
        d->setPixmap( pixmap, fromRotatedRect( rect, d->rotation ), d->tiles[ i ] );
    }
}

void TilesManager::Private::setPixmap( const QPixmap *pixmap, const NormalizedRect &rect, TileNode &tile )
{
    QRect pixmapRect = TilesManager::toRotatedRect( rect, rotation ).geometry( width, height );

    // Exclude tiles outside the viewport
    if ( !tile.rect.intersects( rect ) )
        return;

    // if the tile is not entirely within the viewport (the tile intersects an
    // edged of the viewport), attempt to set the pixmap in the children tiles
    if ( !((tile.rect & rect) == tile.rect) )
    {
        // paint children tiles
        if ( tile.nTiles > 0 )
        {
            for ( int i = 0; i < tile.nTiles; ++i )
                setPixmap( pixmap, rect, tile.tiles[ i ] );

            delete tile.pixmap;
            tile.pixmap = 0;
        }

        return;
    }

    // the tile lies entirely within the viewport
    if ( tile.nTiles == 0 )
    {
        tile.dirty = false;

        // check whether the tile size is big and split it if necessary
        if ( !splitBigTiles( tile, rect ) )
        {
            if ( tile.pixmap )
            {
                totalPixels -= tile.pixmap->width()*tile.pixmap->height();
                delete tile.pixmap;
            }
            NormalizedRect rotatedRect = TilesManager::toRotatedRect( tile.rect, rotation );
            tile.pixmap = new QPixmap( pixmap->copy( rotatedRect.geometry( width, height ).translated( -pixmapRect.topLeft() ) ) );
            totalPixels += tile.pixmap->width()*tile.pixmap->height();
        }
        else
        {
            if ( tile.pixmap )
            {
                totalPixels -= tile.pixmap->width()*tile.pixmap->height();
                delete tile.pixmap;
                tile.pixmap = 0;
            }

            for ( int i = 0; i < tile.nTiles; ++i )
                setPixmap( pixmap, rect, tile.tiles[ i ] );
        }
    }
    else
    {
        QRect tileRect = tile.rect.geometry( width, height );
        // sets the pixmap of the children tiles. if the tile's size is too
        // small, discards the children tiles and use the current one
        if ( tileRect.width()*tileRect.height() >= TILES_MAXSIZE )
        {
            tile.dirty = false;
            if ( tile.pixmap )
            {
                totalPixels -= tile.pixmap->width()*tile.pixmap->height();
                delete tile.pixmap;
                tile.pixmap = 0;
            }

            for ( int i = 0; i < tile.nTiles; ++i )
                setPixmap( pixmap, rect, tile.tiles[ i ] );
        }
        else
        {
            // remove children tiles
            for ( int i = 0; i < tile.nTiles; ++i )
            {
                deleteTiles( tile.tiles[ i ] );
                tile.tiles[ i ].pixmap = 0;
            }

            delete [] tile.tiles;
            tile.tiles = 0;
            tile.nTiles = 0;

            // paint tile
            if ( tile.pixmap )
            {
                totalPixels -= tile.pixmap->width()*tile.pixmap->height();
                delete tile.pixmap;
            }
            tile.pixmap = new QPixmap( pixmap->copy( tile.rect.geometry( width, height ).translated( -pixmapRect.topLeft() ) ) );
            totalPixels += tile.pixmap->width()*tile.pixmap->height();
            tile.dirty = false;
        }
    }
}

bool TilesManager::hasPixmap( const NormalizedRect &rect )
{
    NormalizedRect rotatedRect = fromRotatedRect( rect, d->rotation );
    for ( int i = 0; i < 16; ++i )
    {
        if ( !d->hasPixmap( rotatedRect, d->tiles[ i ] ) )
            return false;
    }

    return true;
}

bool TilesManager::Private::hasPixmap( const NormalizedRect &rect, const TileNode &tile ) const
{
    if ( !tile.rect.intersects( rect ) )
        return true;

    if ( tile.nTiles == 0 )
        return tile.isValid();

    // all children tiles are clean. doesn't need to go deeper
    if ( !tile.dirty )
        return true;

    for ( int i = 0; i < tile.nTiles; ++i )
    {
        if ( !hasPixmap( rect, tile.tiles[ i ] ) )
            return false;
    }

    return true;
}

QList<Tile> TilesManager::tilesAt( const NormalizedRect &rect, bool allowEmpty )
{
    QList<Tile> result;

    NormalizedRect rotatedRect = fromRotatedRect( rect, d->rotation );
    for ( int i = 0; i < 16; ++i )
    {
        d->tilesAt( rotatedRect, d->tiles[ i ], result, allowEmpty );
    }

    return result;
}

void TilesManager::Private::tilesAt( const NormalizedRect &rect, TileNode &tile, QList<Tile> &result, bool allowEmpty )
{
    if ( !tile.rect.intersects( rect ) )
        return;

    // split big tiles before the requests are made, otherwise we would end up
    // requesting huge areas unnecessarily
    splitBigTiles( tile, rect );

    if ( ( allowEmpty && tile.nTiles == 0 ) || ( !allowEmpty && tile.pixmap ) )
    {
        NormalizedRect rotatedRect;
        if ( rotation != Rotation0 )
            rotatedRect = TilesManager::toRotatedRect( tile.rect, rotation );
        else
            rotatedRect = tile.rect;
        result.append( Tile( rotatedRect, tile.pixmap, tile.isValid() ) );
    }
    else
    {
        for ( int i = 0; i < tile.nTiles; ++i )
            tilesAt( rect, tile.tiles[ i ], result, allowEmpty );
    }
}

qulonglong TilesManager::totalMemory() const
{
    return 4*d->totalPixels;
}

void TilesManager::cleanupPixmapMemory( qulonglong numberOfBytes, const NormalizedRect &visibleRect, int visiblePageNumber )
{
    QList<TileNode*> rankedTiles;
    for ( int i = 0; i < 16; ++i )
    {
        d->rankTiles( d->tiles[ i ], rankedTiles, visibleRect, visiblePageNumber );
    }
    qSort( rankedTiles.begin(), rankedTiles.end(), rankedTilesLessThan );

    while ( numberOfBytes > 0 && !rankedTiles.isEmpty() )
    {
        TileNode *tile = rankedTiles.takeLast();
        if ( !tile->pixmap )
            continue;

        // do not evict visible pixmaps
        if ( tile->rect.intersects( visibleRect ) )
            continue;

        qulonglong pixels = tile->pixmap->width()*tile->pixmap->height();
        d->totalPixels -= pixels;
        if ( numberOfBytes < 4*pixels )
            numberOfBytes = 0;
        else
            numberOfBytes -= 4*pixels;

        delete tile->pixmap;
        tile->pixmap = 0;

        d->markParentDirty( *tile );
    }
}

void TilesManager::Private::markParentDirty( const TileNode &tile )
{
    if ( !tile.parent )
        return;

    if ( !tile.parent->dirty )
    {
        tile.parent->dirty = true;
        markParentDirty( *tile.parent );
    }
}

void TilesManager::Private::rankTiles( TileNode &tile, QList<TileNode*> &rankedTiles, const NormalizedRect &visibleRect, int visiblePageNumber )
{
    // If the page is visible, visibleRect is not null.
    // Otherwise we use the number of one of the visible pages to calculate the
    // distance.
    // Note that the current page may be visible and yet its pageNumber is
    // different from visiblePageNumber. Since we only use this value on hidden
    // pages, any visible page number will fit.
    if ( visibleRect.isNull() && visiblePageNumber < 0 )
        return;

    if ( tile.pixmap )
    {
        // Update distance
        if ( !visibleRect.isNull() )
        {
            NormalizedPoint viewportCenter = visibleRect.center();
            NormalizedPoint tileCenter = tile.rect.center();
            // Manhattan distance. It's a good and fast approximation.
            tile.distance = qAbs(viewportCenter.x - tileCenter.x) + qAbs(viewportCenter.y - tileCenter.y);
        }
        else
        {
            // For non visible pages only the vertical distance is used
            if ( pageNumber < visiblePageNumber )
                tile.distance = 1 - tile.rect.bottom;
            else
                tile.distance = tile.rect.top;
        }
        rankedTiles.append( &tile );
    }
    else
    {
        for ( int i = 0; i < tile.nTiles; ++i )
        {
            rankTiles( tile.tiles[ i ], rankedTiles, visibleRect, visiblePageNumber );
        }
    }
}

bool TilesManager::isRequesting( const NormalizedRect &rect, int pageWidth, int pageHeight ) const
{
    return rect == d->requestRect && pageWidth == d->requestWidth && pageHeight == d->requestHeight;
}

void TilesManager::setRequest( const NormalizedRect &rect, int pageWidth, int pageHeight )
{
    d->requestRect = rect;
    d->requestWidth = pageWidth;
    d->requestHeight = pageHeight;
}

bool TilesManager::Private::splitBigTiles( TileNode &tile, const NormalizedRect &rect )
{
    QRect tileRect = tile.rect.geometry( width, height );
    if ( tileRect.width()*tileRect.height() < TILES_MAXSIZE )
        return false;

    split( tile, rect );
    return true;
}

void TilesManager::Private::split( TileNode &tile, const NormalizedRect &rect )
{
    if ( tile.nTiles != 0 )
        return;

    if ( rect.isNull() || !tile.rect.intersects( rect ) )
        return;

    tile.nTiles = 4;
    tile.tiles = new TileNode[4];
    double hCenter = (tile.rect.left + tile.rect.right)/2;
    double vCenter = (tile.rect.top + tile.rect.bottom)/2;

    tile.tiles[0].rect = NormalizedRect( tile.rect.left, tile.rect.top, hCenter, vCenter );
    tile.tiles[1].rect = NormalizedRect( hCenter, tile.rect.top, tile.rect.right, vCenter );
    tile.tiles[2].rect = NormalizedRect( tile.rect.left, vCenter, hCenter, tile.rect.bottom );
    tile.tiles[3].rect = NormalizedRect( hCenter, vCenter, tile.rect.right, tile.rect.bottom );

    for ( int i = 0; i < tile.nTiles; ++i )
    {
        tile.tiles[ i ].parent = &tile;
        splitBigTiles( tile.tiles[ i ], rect );
    }
}

NormalizedRect TilesManager::fromRotatedRect( const NormalizedRect &rect, Rotation rotation )
{
    if ( rotation == Rotation0 )
        return rect;

    NormalizedRect newRect;
    switch ( rotation )
    {
        case Rotation90:
            newRect = NormalizedRect( rect.top, 1 - rect.right, rect.bottom, 1 - rect.left );
            break;
        case Rotation180:
            newRect = NormalizedRect( 1 - rect.right, 1 - rect.bottom, 1 - rect.left, 1 - rect.top );
            break;
        case Rotation270:
            newRect = NormalizedRect( 1 - rect.bottom, rect.left, 1 - rect.top, rect.right );
            break;
        default:
            newRect = rect;
            break;
    }

    return newRect;
}

NormalizedRect TilesManager::toRotatedRect( const NormalizedRect &rect, Rotation rotation )
{
    if ( rotation == Rotation0 )
        return rect;

    NormalizedRect newRect;
    switch ( rotation )
    {
        case Rotation90:
            newRect = NormalizedRect( 1 - rect.bottom, rect.left, 1 - rect.top, rect.right );
            break;
        case Rotation180:
            newRect = NormalizedRect( 1 - rect.right, 1 - rect.bottom, 1 - rect.left, 1 - rect.top );
            break;
        case Rotation270:
            newRect = NormalizedRect( rect.top, 1 - rect.right, rect.bottom, 1 - rect.left );
            break;
        default:
            newRect = rect;
            break;
    }

    return newRect;
}

TileNode::TileNode()
    : pixmap( 0 )
    , dirty ( true )
    , distance( -1 )
    , tiles( 0 )
    , nTiles( 0 )
    , parent( 0 )
{
}

bool TileNode::isValid() const
{
    return pixmap && !dirty;
}

class Tile::Private
{
    public:
        Private();

        NormalizedRect rect;
        QPixmap *pixmap;
        bool isValid;
};

Tile::Private::Private()
    : pixmap( 0 )
    , isValid( false )
{
}

Tile::Tile( const NormalizedRect &rect, QPixmap *pixmap, bool isValid )
    : d( new Tile::Private )
{
    d->rect = rect;
    d->pixmap = pixmap;
    d->isValid = isValid;
}

Tile::Tile( const Tile &t )
    : d( new Tile::Private )
{
    d->rect = t.d->rect;
    d->pixmap = t.d->pixmap;
    d->isValid = t.d->isValid;
}

Tile & Tile::operator=( const Tile &t )
{
    if ( this == &t )
        return *this;

    d = new Tile::Private;
    d->rect = t.d->rect;
    d->pixmap = t.d->pixmap;
    d->isValid = t.d->isValid;

    return *this;
}

Tile::~Tile()
{
    delete d;
}

NormalizedRect Tile::rect() const
{
    return d->rect;
}

QPixmap * Tile::pixmap() const
{
    return d->pixmap;
}

bool Tile::isValid() const
{
    return d->isValid;
}
