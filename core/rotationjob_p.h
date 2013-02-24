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
#include <QtGui/QTransform>

#include <threadweaver/Job.h>

#include "core/global.h"
#include "core/area.h"

namespace Okular {

class DocumentObserver;
class PagePrivate;

class RotationJob : public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        RotationJob( const QImage &image, Rotation oldRotation, Rotation newRotation, DocumentObserver *observer );

        void setPage( PagePrivate * pd );
        void setRect( const NormalizedRect &rect );

        QImage image() const;
        Rotation rotation() const;
        DocumentObserver *observer() const;
        PagePrivate * page() const;
        NormalizedRect rect() const;

        static QTransform rotationMatrix( Rotation from, Rotation to );

    protected:
        virtual void run();

    private:
        const QImage mImage;
        Rotation mOldRotation;
        Rotation mNewRotation;
        DocumentObserver *mObserver;
        QImage mRotatedImage;
        PagePrivate * m_pd;
        NormalizedRect mRect;
};

}

#endif
