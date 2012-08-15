/***************************************************************************
 *   Copyright (C) 2012 by Mailson Menezes <mailson@gmail.com>             *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "tilesmanager.h"

#include <QPixmap>
#include <QtCore/qmath.h>
#include <QList>

#ifdef TILES_DEBUG
#include <QPainter>
#endif

#define RANGE_MAX 1073741823
#define RANGE_MIN -1073741824

using namespace Okular;

static bool rankedTilesLessThan( Tile *t1, Tile *t2 )
{
    if ( t1->dirty ^ t2->dirty )
        return t1->miss < t2->miss;

    return !t1->dirty;
}

class TilesManager::Private
{
    public:
        Private();

        bool hasPixmap( const NormalizedRect &rect, const Tile &tile ) const;
        void tilesAt( const NormalizedRect &rect, Tile &tile, QList<Tile> &result );
        void setPixmap( const QPixmap *pixmap, const NormalizedRect &rect, Tile &tile );
        void markDirty( Tile &tile );
        void deleteTiles( const Tile &tile );

        void onClearPixmap( const Tile &tile );
        void rankTiles( Tile &tile );

        Tile tiles[16];
        int width;
        int height;
        long totalPixels;

        QList<Tile*> rankedTiles;
        QSize tileSize;
};

TilesManager::Private::Private()
    : width( 0 )
    , height( 0 )
    , totalPixels( 0 )
    , tileSize( QSize() )
{
}

TilesManager::TilesManager( int width, int height )
    : d( new Private )
{
    d->width = width;
    d->height = height;

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
}

void TilesManager::Private::deleteTiles( const Tile &tile )
{
    if ( tile.pixmap )
        delete tile.pixmap;

    for ( int i = 0; i < tile.nTiles; ++i )
        deleteTiles( tile.tiles[ i ] );
}

void TilesManager::setWidth( int width )
{
    if ( width == d->width )
        return;

    d->width = width;

    for ( int i = 0; i < 16; ++i )
    {
        d->markDirty( d->tiles[ i ] );
    }
}

int TilesManager::width() const {
    return d->width;
}

void TilesManager::setHeight( int height )
{
    if ( height == d->height )
        return;

    d->height = height;
}

int TilesManager::height() const {
    return d->height;
}

void TilesManager::Private::markDirty( Tile &tile )
{
    tile.dirty = true;

    if ( tile.nTiles > 0 && tile.tiles )
    {
        for ( int i = 0; i < tile.nTiles; ++i )
        {
            markDirty( tile.tiles[ i ] );
        }
    }
}

void TilesManager::setPixmap( const QPixmap *pixmap, const NormalizedRect &rect )
{
    for ( int i = 0; i < 16; ++i )
    {
        d->setPixmap( pixmap, rect, d->tiles[ i ] );
    }
}

void TilesManager::Private::setPixmap( const QPixmap *pixmap, const NormalizedRect &rect, Tile &tile )
{
    QRect pixmapRect = rect.geometry( width, height );

    if ( !tile.rect.intersects( rect ) )
        return;

    // the tile intersects an edge of the viewport
    if ( !((tile.rect & rect) == tile.rect) )
    {
        // paint subtiles if applied
        for ( int i = 0; i < tile.nTiles; ++i )
            setPixmap( pixmap, rect, tile.tiles[ i ] );

        return;
    }

    if ( tile.nTiles == 0 )
    {
        tile.dirty = false;

        QRect tileRect = tile.rect.geometry( width, height );

        if ( tileRect.width()*tileRect.height() < TILES_MAXSIZE ) // size ok
        {
            if ( tile.pixmap )
            {
                totalPixels -= tile.pixmap->width()*tile.pixmap->height();
                delete tile.pixmap;
            }

            tile.pixmap = new QPixmap( pixmap->copy( tile.rect.geometry( width, height ).translated( -pixmapRect.topLeft() ) ) );
            totalPixels += tile.pixmap->width()*tile.pixmap->height();

#ifdef TILES_DEBUG
            QRect pixRect = tile.pixmap->rect();
            QPainter p(tile.pixmap);
            p.drawLine(0,0,pixRect.right(),0);
            p.drawLine(pixRect.right(), 0, pixRect.right(), pixRect.bottom());
            p.drawLine(pixRect.right(), pixRect.bottom(), pixRect.left(), pixRect.bottom());
            p.drawLine(pixRect.left(), pixRect.bottom(), 0,0);
#endif // TILES_DEBUG
        }
        else
        {
            tile.nTiles = 4;
            tile.tiles = new Tile[4];
            double hCenter = (tile.rect.left + tile.rect.right)/2;
            double vCenter = (tile.rect.top + tile.rect.bottom)/2;

            tile.tiles[0].rect = NormalizedRect( tile.rect.left, tile.rect.top, hCenter, vCenter );
            tile.tiles[1].rect = NormalizedRect( hCenter, tile.rect.top, tile.rect.right, vCenter );
            tile.tiles[2].rect = NormalizedRect( tile.rect.left, vCenter, hCenter, tile.rect.bottom );
            tile.tiles[3].rect = NormalizedRect( hCenter, vCenter, tile.rect.right, tile.rect.bottom );

            tileSize = tile.tiles[0].rect.geometry( width, height ).size();

            for ( int i = 0; i < tile.nTiles; ++i )
            {
                tile.tiles[ i ].parent = &tile;
                setPixmap( pixmap, rect, tile.tiles[ i ] );
            }

            if ( tile.pixmap )
            {
                totalPixels -= tile.pixmap->width()*tile.pixmap->height();
                delete tile.pixmap;
            }
            tile.pixmap = 0;
        }
    }
    else
    {
        QRect tileRect = tile.rect.geometry( width, height );
        if ( tileRect.width()*tileRect.height() >= TILES_MAXSIZE ) // size ok
        {
            tile.dirty = false;
            for ( int i = 0; i < tile.nTiles; ++i )
                setPixmap( pixmap, rect, tile.tiles[ i ] );
        }
        else // size not ok (too small)
        {
            // remove children tiles
            tile.rect = NormalizedRect();
            for ( int i = 0; i < tile.nTiles; ++i )
            {
                if ( tile.rect.isNull() )
                    tile.rect = tile.tiles[ i ].rect;
                else
                    tile.rect |= tile.tiles[ i ].rect;

                if ( tile.tiles[ i ].pixmap )
                {
                    totalPixels -= tile.tiles[ i ].pixmap->width()*tile.tiles[ i ].pixmap->height();
                    delete tile.tiles[ i ].pixmap;
                }
                tile.tiles[ i ].pixmap = 0;
            }

            tileSize = tile.rect.geometry( width, height ).size();

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
    for ( int i = 0; i < 16; ++i )
    {
        if ( !d->hasPixmap( rect, d->tiles[ i ] ) )
            return false;
    }

    return true;
}

bool TilesManager::Private::hasPixmap( const NormalizedRect &rect, const Tile &tile ) const
{
    if ( !tile.rect.intersects( rect ) )
        return true;

    if ( tile.nTiles == 0 )
        return tile.pixmap && !tile.dirty;

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

QList<Tile> TilesManager::tilesAt( const NormalizedRect &rect )
{
    QList<Tile> result;

    for ( int i = 0; i < 16; ++i )
    {
        d->tilesAt( rect, d->tiles[ i ], result );
    }

    return result;
}

void TilesManager::Private::tilesAt( const NormalizedRect &rect, Tile &tile, QList<Tile> &result )
{
    if ( !tile.rect.intersects( rect ) )
    {
        tile.miss = qMin( tile.miss+1, RANGE_MAX );
        return;
    }

    if ( tile.nTiles == 0 )
    {
        // TODO: check tile size
        tile.miss = qMax( tile.miss-1, RANGE_MIN );
        result.append( tile );
    }
    else
    {
        for ( int i = 0; i < tile.nTiles; ++i )
            tilesAt( rect, tile.tiles[ i ], result );
    }
}

long TilesManager::totalMemory() const
{
    return 4*d->totalPixels;
}

void TilesManager::cleanupPixmapMemory( qulonglong numberOfBytes )
{
    d->rankedTiles.clear();
    for ( int i = 0; i < 16; ++i )
    {
        d->rankTiles( d->tiles[ i ] );
    }
    qSort( d->rankedTiles.begin(), d->rankedTiles.end(), rankedTilesLessThan );

    while ( numberOfBytes > 0 && !d->rankedTiles.isEmpty() )
    {
        Tile *tile = d->rankedTiles.takeLast();
        if ( !tile->pixmap )
            continue;

        long pixels = tile->pixmap->width()*tile->pixmap->height();
        d->totalPixels -= pixels;
        if ( numberOfBytes < 4*pixels )
            numberOfBytes = 0;
        else
            numberOfBytes -= 4*pixels;

        tile->miss = 0;
        delete tile->pixmap;
        tile->pixmap = 0;

        d->onClearPixmap( *tile );
    }
}

void TilesManager::Private::onClearPixmap( const Tile &tile )
{
    if ( !tile.parent )
        return;

    if ( !tile.parent->dirty )
    {
        tile.parent->dirty = true;
        onClearPixmap( *tile.parent );
    }
}

void TilesManager::Private::rankTiles( Tile &tile )
{
    if ( tile.parent )
        tile.miss = qBound( RANGE_MIN, tile.miss + tile.parent->miss, RANGE_MAX );

    if ( tile.pixmap )
    {
        rankedTiles.append( &tile );
    }
    else
    {
        for ( int i = 0; i < tile.nTiles; ++i )
        {
            rankTiles( tile.tiles[ i ] );
        }
        tile.miss = 0;
    }
}

Tile::Tile()
    : pixmap( 0 )
    , dirty ( true )
    , tiles( 0 )
    , nTiles( 0 )
    , parent( 0 )
    , miss( 0 )
{
}

bool Tile::isValid() const
{
    return pixmap && !dirty;
}
