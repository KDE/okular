#include "tilesmanager.h"

#include <QPixmap>
#include <QtCore/qmath.h>

using namespace Okular;

TilesManager::TilesManager( int width, int height )
    : width( width ), height( height )
{
    for ( int i = 0; i < 16; ++i )
    {
        mTiles[ i ] = 0;
    }
}

TilesManager::~TilesManager()
{
    for ( int i = 0; i < 16; ++i )
    {
        if ( mTiles[ i ] )
        {
            delete mTiles[ i ];
            mTiles[ i ] = 0;
        }
    }
}

void TilesManager::setPixmap( QPixmap *pixmap, const NormalizedRect &rect )
{
    const double dim = 0.25;
    int left = qCeil( rect.left/dim );
    int top = qCeil( rect.top/dim );
    int right = rect.right/dim;
    int bottom = rect.bottom/dim;

    const QRect pixmapRect = rect.geometry( width, height );

    for ( int y = top; y < bottom; y++ )
    {
        for ( int x = left; x < right; x++ )
        {
            const NormalizedRect tileRect( x*dim, y*dim, x*dim+dim, y*dim+dim );
            int index = 4*y+x;
            if ( !mTiles[ index ] )
                mTiles[ index ] = new Tile();

            Tile *tile = mTiles[ index ];
            tile->rect = tileRect;
            if ( tile->pixmap )
                delete (tile->pixmap);

            tile->pixmap = new QPixmap( pixmap->copy( tileRect.geometry( width, height ).translated( -pixmapRect.topLeft() ) ) );
        }
    }

    delete pixmap;
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
            if ( !mTiles[ index ] )
                return false;

            if ( !mTiles[ index ]->pixmap )
                return false;

            const NormalizedRect tileRect( x*dim, y*dim, x*dim+dim, y*dim+dim );
            QRect tileQRect = tileRect.geometry( width, height );
            if ( tileQRect.width() != mTiles[ index ]->pixmap->width() )
                return false;
            if ( tileQRect.height() != mTiles[ index ]->pixmap->height() )
                return false;
        }
    }

    return true;
}

QList<Tile*> TilesManager::tilesAt( const NormalizedRect &rect ) const
{
    QList<Tile*> result;

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
            if ( mTiles[ index ] )
            {
                if ( !mTiles[ index ]->pixmap )
                    continue;

                const NormalizedRect tileRect( x*dim, y*dim, x*dim+dim, y*dim+dim );
                QRect tileQRect = tileRect.geometry( width, height );
                if ( tileQRect.width() == mTiles[ index ]->pixmap->width()
                        && tileQRect.height() == mTiles[ index ]->pixmap->height() )
                {
                    result.append( mTiles[ index ] );
                }
            }
        }
    }

    return result;
}

Tile::Tile()
    : pixmap( 0 )
{
}

Tile::~Tile() {
    delete pixmap;
}
