/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_ROTATIONJOB_H
#define _OKULAR_ROTATIONJOB_H

#include <QtCore/QThread>
#include <QtGui/QImage>

namespace Okular {

class RotationJob : public QThread
{
    public:
        enum Rotation
        {
            Rotation0,
            Rotation90,
            Rotation180,
            Rotation270
        };

        RotationJob( const QImage &image, Rotation rotation, int id );

        QImage image() const;
        int id() const;

    protected:
        virtual void run();

    private:
        const QImage mImage;
        Rotation mRotation;
        int mId;
        QImage mRotatedImage;
};

}

#endif
