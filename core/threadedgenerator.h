/***************************************************************************
 *   Copyright (C) 2007  Tobias Koenig <tokoe@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_THREADEDGENERATOR_H
#define OKULAR_THREADEDGENERATOR_H

#include <okular/core/okular_export.h>

#include <okular/core/generator.h>

namespace Okular {

/**
 * ThreadedGenerator is meant to be a base class for Okular generators
 * which supports multithreaded generation of page pixmaps, text pages
 * and other data structures.
 */
class OKULAR_EXPORT ThreadedGenerator : public Generator
{
    friend class PixmapGenerationThread;
    friend class TextPageGenerationThread;

    Q_OBJECT

    public:
        /**
         * Creates a new threaded generator.
         */
        ThreadedGenerator();

        /**
         * Destroys the threaded generator.
         */
        ~ThreadedGenerator();

        /**
         * Returns whether the generator is ready to
         * handle a new pixmap generation request.
         */
        bool canRequestPixmap() const;

        /**
         * This method can be called to trigger the generation of
         * a new pixmap as described by @p request.
         */
        void requestPixmap( PixmapRequest * request );

        /**
         * This method can be called to trigger the generation of
         * a text page for the given @p page.
         * @see TextPage
         */
        void requestTextPage( Page * page );

    protected:
        /**
         * Returns the image of the page as specified in
         * the passed pixmap @p request.
         *
         * Note: This method is executed in its own separated thread!
         */
        virtual QImage image( PixmapRequest *page ) = 0;

        /**
         * Returns the text page for the given @p page.
         *
         * Note: This method is executed in its own separated thread!
         */
        virtual TextPage* textPage( Page *page );

        /**
         * @internal
         */
        bool canGeneratePixmap() const;

        /**
         * @internal
         */
        void generatePixmap( PixmapRequest* );

        /**
         * @internal
         */
        void generateSyncTextPage( Page* );

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void pixmapGenerationFinished() )
        Q_PRIVATE_SLOT( d, void textpageGenerationFinished() )
};

}

#endif
