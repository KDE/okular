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
    if ( mOldRotation == mNewRotation ) {
        mRotatedImage = mImage;
        return;
    }

    QMatrix matrix = rotationMatrix( mOldRotation, mNewRotation );

    mRotatedImage = mImage.transformed( matrix );
}

QMatrix RotationJob::rotationMatrix( Rotation from, Rotation to )
{
    QMatrix matrix;

    if ( from == Rotation0 ) {
        if ( to == Rotation90 )
            matrix.rotate( 90 );
        else if ( to == Rotation180 )
            matrix.rotate( 180 );
        else if ( to == Rotation270 )
            matrix.rotate( 270 );
    } else if ( from == Rotation90 ) {
        if ( to == Rotation180 )
            matrix.rotate( 90 );
        else if ( to == Rotation270 )
            matrix.rotate( 180 );
        else if ( to == Rotation0 )
            matrix.rotate( 270 );
    } else if ( from == Rotation180 ) {
        if ( to == Rotation270 )
            matrix.rotate( 90 );
        else if ( to == Rotation0 )
            matrix.rotate( 180 );
        else if ( to == Rotation90 )
            matrix.rotate( 270 );
    } else if ( from == Rotation270 ) {
        if ( to == Rotation0 )
            matrix.rotate( 90 );
        else if ( to == Rotation90 )
            matrix.rotate( 180 );
        else if ( to == Rotation180 )
            matrix.rotate( 270 );
    }

    return matrix;
}

#include "rotationjob_p.moc"
