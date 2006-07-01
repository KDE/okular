/***************************************************************************
 *   Copyright (C) 2006 by Luigi Toscano <luigi.toscano@tiscali.it>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "utils.h"

namespace okularUtils
{

QRect rotateRect( const QRect & source, int width, int height, int orientation )
{
    QRect ret;

    // adapt the coordinates of the boxes to the rotation
    switch ( orientation )
    {
        case 1:
            ret = QRect( width - source.y() - source.height(), source.x(),
                         source.height(), source.width() );
            break;
        case 2:
            ret = QRect( width - source.x() - source.width(), height - source.y() - source.height(),
                         source.width(), source.height() );
            break;
        case 3:
            ret = QRect( source.y(), height - source.x() - source.width(),
                         source.height(), source.width() );
            break;
        case 0:  // no modifications
        default: // other cases
            ret = source;
    }

    return ret;
}

}
