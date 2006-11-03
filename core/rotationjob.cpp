/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtGui/QMatrix>

#include "rotationjob.h"

using namespace Okular;

RotationJob::RotationJob( const QImage &image, Rotation rotation, int id )
    : mImage( image ), mRotation( rotation ), mId( id )
{
}

QImage RotationJob::image() const
{
    return mRotatedImage;
}

int RotationJob::id() const
{
    return mId;
}

void RotationJob::run()
{
    QMatrix matrix;

    switch ( mRotation ) {
        case Rotation90:
            matrix.rotate( 90 );
            break;
        case Rotation180:
            matrix.rotate( 180 );
            break;
        case Rotation270:
            matrix.rotate( 270 );
            break;
        case Rotation0:
        default:
            break;
    }

    mRotatedImage = mImage.transformed( matrix );
}
