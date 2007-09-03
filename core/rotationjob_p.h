/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_ROTATIONJOB_P_H_
#define _OKULAR_ROTATIONJOB_P_H_

#include <QtGui/QImage>
#include <QtGui/QMatrix>

#include <threadweaver/Job.h>

#include "core/global.h"

namespace Okular {

class PagePrivate;

class RotationJob : public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        RotationJob( const QImage &image, Rotation oldRotation, Rotation newRotation, int id );

        void setPage( PagePrivate * pd );

        QImage image() const;
        Rotation rotation() const;
        int id() const;
        PagePrivate * page() const;

        static QMatrix rotationMatrix( Rotation from, Rotation to );

    protected:
        virtual void run();

    private:
        const QImage mImage;
        Rotation mOldRotation;
        Rotation mNewRotation;
        int mId;
        QImage mRotatedImage;
        PagePrivate * m_pd;
};

}

#endif
