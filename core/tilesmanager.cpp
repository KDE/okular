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

using namespace Okular;

TilesManager::TilesManager( int width, int height )
    : m_width( width ), m_height( height )
{
    const double dim = 0.25;
    for ( int i = 0; i < 16; ++i )
    {
        int x = i % 4;
        int y = i / 4;
        m_tiles[ i ].rect = NormalizedRect( x*dim, y*dim, x*dim+dim, y*dim+dim );
    }
}

TilesManager::~TilesManager()
{
    for ( int i = 0; i < 16; ++i )
    {
        if ( m_tiles[ i ].pixmap )
            delete m_tiles[ i ].pixmap;
    }
}

void TilesManager::setWidth( int width )
{
    if ( width == m_width )
        return;

    m_width = width;

    for ( int i = 0; i < 16; ++i )
    {
        m_tiles[ i ].dirty = true;
    }
}

void TilesManager::setHeight( int height )
{
    if ( height == m_height )
        return;

    m_height = height;

    for ( int i = 0; i < 16; ++i )
    {
        m_tiles[ i ].dirty = true;
    }
}

void TilesManager::setPixmap( const QPixmap *pixmap, const NormalizedRect &rect )
{
    const double dim = 0.25;
    int left = qCeil( rect.left/dim );
    int top = qCeil( rect.top/dim );
    int right = rect.right/dim;
    int bottom = rect.bottom/dim;

    const QRect pixmapRect = rect.geometry( m_width, m_height );

    for ( int y = top; y < bottom; y++ )
    {
        for ( int x = left; x < right; x++ )
        {
            const NormalizedRect tileRect( x*dim, y*dim, x*dim+dim, y*dim+dim );
            int index = 4*y+x;

            if ( m_tiles[ index ].pixmap )
                delete (m_tiles[ index ].pixmap);

            m_tiles[ index ].pixmap = new QPixmap( pixmap->copy( tileRect.geometry( m_width, m_height ).translated( -pixmapRect.topLeft() ) ) );
            m_tiles[ index ].dirty = false;
        }
    }
}

bool TilesManager::hasPixmap( const NormalizedRect &rect )
{
    const double dim = 0.25;
    int left = rect.left/dim;
    int top = rect.top/dim;
    int right = qCeil( rect.right/dim );
    int bottom = qCeil( rect.bottom/dim );

    for ( int y = top; y < bottom; y++ )
    {
        for ( int x = left; x < right; x++ )
        {
            int index = 4*y + x;
            if ( !m_tiles[ index ].pixmap )
                return false;

            if ( m_tiles[ index ].dirty )
                return false;
        }
    }

    return true;
}

QList<Tile> TilesManager::tilesAt( const NormalizedRect &rect ) const
{
    QList<Tile> result;

    const double dim = 0.25;
    int left = rect.left/dim;
    int top = rect.top/dim;
    int right = qCeil( rect.right/dim );
    int bottom = qCeil( rect.bottom/dim );

    for ( int y = top; y < bottom; y++ )
    {
        for ( int x = left; x < right; x++ )
        {
            int index = 4*y + x;
            result.append( m_tiles[ index ] );
        }
    }

    return result;
}

long TilesManager::totalMemory() const
{
    long totalPixels = 0;

    for ( int i = 0; i < 16; i++ )
    {
        QPixmap *pixmap = m_tiles[ i ].pixmap;
        if ( pixmap )
            totalPixels += pixmap->width() * pixmap->height();
    }

    return 4*totalPixels;
}

Tile::Tile()
    : pixmap( 0 )
    , dirty ( true )
{
}

bool Tile::isValid() const
{
    return pixmap && !dirty;
}
