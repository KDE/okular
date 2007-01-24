/***************************************************************************
 *   Copyright (C) 2007  Tobias Koenig <tokoe@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_THREADEDGENERATOR_P_H
#define OKULAR_THREADEDGENERATOR_P_H

#include <QtCore/QThread>

namespace Okular {

class PixmapGenerationThread : public QThread
{
    public:
        PixmapGenerationThread( ThreadedGenerator *generator )
            : mGenerator( generator ), mRequest( 0 )
        {
        }

        void startGeneration( PixmapRequest *request )
        {
            mRequest = request;

            start( QThread::InheritPriority );
        }

        void endGeneration()
        {
            mRequest = 0;
        }

        PixmapRequest *request() const
        {
            return mRequest;
        }

        QImage image() const
        {
            return mImage;
        }

    protected:
        virtual void run()
        {
            mImage = QImage();

            if ( mRequest )
                mImage = mGenerator->image( mRequest );
        }

    private:
        ThreadedGenerator *mGenerator;
        PixmapRequest *mRequest;
        QImage mImage;
};


class TextPageGenerationThread : public QThread
{
    public:
        TextPageGenerationThread( ThreadedGenerator *generator )
            : mGenerator( generator ), mPage( 0 )
        {
        }

        void startGeneration( Page *page )
        {
            mPage = page;

            start( QThread::InheritPriority );
        }

        void endGeneration()
        {
            mPage = 0;
        }

        Page *page() const
        {
            return mPage;
        }

        TextPage* textPage() const
        {
            return mTextPage;
        }

    protected:
        virtual void run()
        {
            mTextPage = 0;

            if ( mPage )
                mTextPage = mGenerator->textPage( mPage );
        }

    private:
        ThreadedGenerator *mGenerator;
        Page *mPage;
        TextPage *mTextPage;
};

}

#endif
