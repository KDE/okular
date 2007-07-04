/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "rotationjob_p.h"

#include <QtGui/QMatrix>

using namespace Okular;

RotationJob::RotationJob( const QImage &image, Rotation oldRotation, Rotation newRotation, int id )
    : mImage( image ), mOldRotation( oldRotation ), mNewRotation( newRotation ), mId( id ), m_pd( 0 )
{
}

void RotationJob::setPage( PagePrivate * pd )
{
    m_pd = pd;
}

QImage RotationJob::image() const
{
    return mRotatedImage;
}

Rotation RotationJob::rotation() const
{
    return mNewRotation;
}

int RotationJob::id() const
{
    return mId;
}

PagePrivate * RotationJob::page() const
{
    return m_pd;
}

void RotationJob::run()
{
    QMatrix matrix;

    if ( mOldRotation == mNewRotation ) {
        mRotatedImage = mImage;
        return;
    }

    if ( mOldRotation == Rotation0 ) {
        if ( mNewRotation == Rotation90 )
            matrix.rotate( 90 );
        else if ( mNewRotation == Rotation180 )
            matrix.rotate( 180 );
        else if ( mNewRotation == Rotation270 )
            matrix.rotate( 270 );
    } else if ( mOldRotation == Rotation90 ) {
        if ( mNewRotation == Rotation180 )
            matrix.rotate( 90 );
        else if ( mNewRotation == Rotation270 )
            matrix.rotate( 180 );
        else if ( mNewRotation == Rotation0 )
            matrix.rotate( 270 );
    } else if ( mOldRotation == Rotation180 ) {
        if ( mNewRotation == Rotation270 )
            matrix.rotate( 90 );
        else if ( mNewRotation == Rotation0 )
            matrix.rotate( 180 );
        else if ( mNewRotation == Rotation90 )
            matrix.rotate( 270 );
    } else if ( mOldRotation == Rotation270 ) {
        if ( mNewRotation == Rotation0 )
            matrix.rotate( 90 );
        else if ( mNewRotation == Rotation90 )
            matrix.rotate( 180 );
        else if ( mNewRotation == Rotation180 )
            matrix.rotate( 270 );
    }

    mRotatedImage = mImage.transformed( matrix );
}

#include "rotationjob_p.moc"
