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

#include "core/global.h"

namespace Okular {

class RotationJob : public QThread
{
    public:
        RotationJob( const QImage &image, Rotation oldRotation, Rotation newRotation, int id );

        QImage image() const;
        Rotation rotation() const;
        int id() const;

    protected:
        virtual void run();

    private:
        const QImage mImage;
        Rotation mOldRotation;
        Rotation mNewRotation;
        int mId;
        QImage mRotatedImage;
};

}

#endif
