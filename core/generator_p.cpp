/***************************************************************************
 *   Copyright (C) 2007  Tobias Koenig <tokoe@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_p.h"

#include <kdebug.h>

#include "fontinfo.h"
#include "generator.h"
#include "utils.h"

using namespace Okular;

PixmapGenerationThread::PixmapGenerationThread( Generator *generator )
    : mGenerator( generator ), mRequest( 0 ), mCalcBoundingBox( false )
{
}

void PixmapGenerationThread::startGeneration( PixmapRequest *request, bool calcBoundingBox )
{
    mRequest = request;
    mCalcBoundingBox = calcBoundingBox;

    start( QThread::InheritPriority );
}

void PixmapGenerationThread::endGeneration()
{
    mRequest = 0;
}

PixmapRequest *PixmapGenerationThread::request() const
{
    return mRequest;
}

QImage PixmapGenerationThread::image() const
{
    return mImage;
}

bool PixmapGenerationThread::calcBoundingBox() const
{
    return mCalcBoundingBox;
}

NormalizedRect PixmapGenerationThread::boundingBox() const
{
    return mBoundingBox;
}

void PixmapGenerationThread::run()
{
    mImage = QImage();

    if ( mRequest )
    {
        mImage = mGenerator->image( mRequest );
        if ( mCalcBoundingBox )
            mBoundingBox = Utils::imageBoundingBox( &mImage );
    }
}


TextPageGenerationThread::TextPageGenerationThread( Generator *generator )
    : mGenerator( generator ), mPage( 0 )
{
}

void TextPageGenerationThread::startGeneration( Page *page )
{
    mPage = page;

    start( QThread::InheritPriority );
}

void TextPageGenerationThread::endGeneration()
{
    mPage = 0;
}

Page *TextPageGenerationThread::page() const
{
    return mPage;
}

TextPage* TextPageGenerationThread::textPage() const
{
    return mTextPage;
}

void TextPageGenerationThread::run()
{
    mTextPage = 0;

    if ( mPage )
        mTextPage = mGenerator->textPage( mPage );
}


FontExtractionThread::FontExtractionThread( Generator *generator, int pages )
    : mGenerator( generator ), mNumOfPages( pages ), mGoOn( true )
{
}

void FontExtractionThread::startExtraction( bool async )
{
    if ( async )
    {
        connect( this, SIGNAL(finished()), this, SLOT(deleteLater()) );
        start( QThread::InheritPriority );
    }
    else
    {
        run();
        deleteLater();
    }
}

void FontExtractionThread::stopExtraction()
{
    mGoOn = false;
}

void FontExtractionThread::run()
{
    for ( int i = -1; i < mNumOfPages && mGoOn; ++i )
    {
        FontInfo::List list = mGenerator->fontsForPage( i );
        foreach ( const FontInfo& fi, list )
        {
            emit gotFont( fi );
        }
        emit progress( i );
    }
}

#include "generator_p.moc"
