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

#include "area.h"

#include <QtCore/QSet>
#include <QtCore/QThread>
#include <QtGui/QImage>

class QEventLoop;
class QMutex;

namespace Okular {

class DocumentPrivate;
class FontInfo;
class Generator;
class Page;
class PixmapGenerationThread;
class PixmapRequest;
class TextPage;
class TextPageGenerationThread;

class GeneratorPrivate
{
    public:
        GeneratorPrivate();

        virtual ~GeneratorPrivate();

        Q_DECLARE_PUBLIC( Generator )
        Generator *q_ptr;

        PixmapGenerationThread* pixmapGenerationThread();
        TextPageGenerationThread* textPageGenerationThread();

        void pixmapGenerationFinished();
        void textpageGenerationFinished();

        QMutex* threadsLock();

        virtual QVariant metaData( const QString &key, const QVariant &option ) const;
        virtual QImage image( PixmapRequest * );

        DocumentPrivate *m_document;
        // NOTE: the following should be a QSet< GeneratorFeature >,
        // but it is not to avoid #include'ing generator.h
        QSet< int > m_features;
        PixmapGenerationThread *mPixmapGenerationThread;
        TextPageGenerationThread *mTextPageGenerationThread;
        mutable QMutex *m_mutex;
        QMutex *m_threadsMutex;
        bool mPixmapReady : 1;
        bool mTextPageReady : 1;
        bool m_closing : 1;
        QEventLoop *m_closingLoop;
};


class PixmapRequestPrivate
{
    public:
        void swap();

        int mId;
        int mPageNumber;
        int mWidth;
        int mHeight;
        int mPriority;
        bool mAsynchronous;
        bool mForce : 1;
        Page *mPage;
};


class PixmapGenerationThread : public QThread
{
    Q_OBJECT

    public:
        PixmapGenerationThread( Generator *generator );

        void startGeneration( PixmapRequest *request, bool calcBoundingRect );

        void endGeneration();

        PixmapRequest *request() const;

        QImage image() const;
        bool calcBoundingBox() const;
        NormalizedRect boundingBox() const;

    protected:
        virtual void run();

    private:
        Generator *mGenerator;
        PixmapRequest *mRequest;
        QImage mImage;
        NormalizedRect mBoundingBox;
        bool mCalcBoundingBox : 1;
};


class TextPageGenerationThread : public QThread
{
    Q_OBJECT

    public:
        TextPageGenerationThread( Generator *generator );

        void startGeneration( Page *page );

        void endGeneration();

        Page *page() const;

        TextPage* textPage() const;

    protected:
        virtual void run();

    private:
        Generator *mGenerator;
        Page *mPage;
        TextPage *mTextPage;
};

class FontExtractionThread : public QThread
{
    Q_OBJECT

    public:
        FontExtractionThread( Generator *generator, int pages );

        void startExtraction( bool async );
        void stopExtraction();

    Q_SIGNALS:
        void gotFont( const Okular::FontInfo& );
        void progress( int page );

    protected:
        virtual void run();

    private:
        Generator *mGenerator;
        int mNumOfPages;
        bool mGoOn;
};

}

#endif
