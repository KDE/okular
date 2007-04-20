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

#include <QtCore/QSet>
#include <QtCore/QThread>
#include <QtGui/QImage>

class KAboutData;
class KComponentData;

namespace Okular {

class Document;
class Generator;
class Page;
class PixmapGenerationThread;
class PixmapRequest;
class TextPage;
class TextPageGenerationThread;

class GeneratorPrivate
{
    public:
        GeneratorPrivate( Generator *parent );

        ~GeneratorPrivate();

        PixmapGenerationThread* pixmapGenerationThread();
        TextPageGenerationThread* textPageGenerationThread();

        void pixmapGenerationFinished();
        void textpageGenerationFinished();

        Document *m_document;
        // NOTE: the following should be a QSet< GeneratorFeature >,
        // but it is not to avoid #include'ing generator.h
        QSet< int > m_features;
        KAboutData *m_about;
        KComponentData *m_componentData;
        Generator *m_generator;
        PixmapGenerationThread *mPixmapGenerationThread;
        TextPageGenerationThread *mTextPageGenerationThread;
        bool mPixmapReady;
        bool mTextPageReady;
};


class PixmapGenerationThread : public QThread
{
    Q_OBJECT

    public:
        PixmapGenerationThread( Generator *generator );

        void startGeneration( PixmapRequest *request );

        void endGeneration();

        PixmapRequest *request() const;

        QImage image() const;

    protected:
        virtual void run();

    private:
        Generator *mGenerator;
        PixmapRequest *mRequest;
        QImage mImage;
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

}

#endif
