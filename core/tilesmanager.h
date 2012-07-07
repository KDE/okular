#ifndef _OKULAR_TILES_MANAGER_H_
#define _OKULAR_TILES_MANAGER_H_

#include "okular_export.h"
#include "area.h"

class QPixmap;

namespace Okular {

class Tile;

class OKULAR_EXPORT TilesManager
{
    public:
        TilesManager( int width, int height );
        virtual ~TilesManager();

        void setPixmap( QPixmap *pixmap, const NormalizedRect &rect );
        bool hasPixmap( const NormalizedRect &rect );
        QList<Tile*> tilesAt( const NormalizedRect &rect ) const;

        int width;
        int height;

    private:
        Tile *mTiles[16];
};

class OKULAR_EXPORT Tile
{
    public:
        Tile();
        virtual ~Tile();

        NormalizedRect rect;
        QPixmap *pixmap;
};

}

#endif // _OKULAR_TILES_MANAGER_H_
