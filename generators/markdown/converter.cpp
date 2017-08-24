/***************************************************************************
 *   Copyright (C) 2017 by Julian Wolff <wolff@julianwolff.de>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "converter.h"

#include <KLocalizedString>

#include <QTemporaryFile>
#include <QTextDocument>
#include <QTextStream>
#include <QTextFrame>
#include <QTextCursor>

#include <core/action.h>

#include "debug_md.h"

extern "C" {
#include <mkdio.h>
}

// older versions of discount might not have these flags.
// defining them to 0 allows us to convert without them
#ifndef MKD_FENCEDCODE 
#define MKD_FENCEDCODE 0
#endif

#ifndef MKD_GITHUBTAGS 
#define MKD_GITHUBTAGS 0
#endif

#ifndef MKD_AUTOLINK 
#define MKD_AUTOLINK 0
#endif
    
using namespace Markdown;

Converter::Converter()
: mTextDocument(nullptr)
{
}

Converter::~Converter()
{
}

QTextDocument* Converter::convert( const QString &fileName )
{
    FILE *markdownFile = fopen( fileName.toLocal8Bit(), "rb" );
    mDir = QDir( fileName.left( fileName.lastIndexOf( '/' ) ) );
    
    MMIOT *markdownHandle = mkd_in( markdownFile, 0 );
    
    if ( !mkd_compile( markdownHandle, MKD_FENCEDCODE | MKD_GITHUBTAGS | MKD_AUTOLINK ) ) {
        emit error( i18n( "Failed to compile markdown document!" ), -1 );
        return 0;
    }
    
    char *htmlDocument;
    int size = mkd_document( markdownHandle, &htmlDocument );
    
    QString html = QString::fromUtf8( htmlDocument, size );
    
    mTextDocument = new QTextDocument;
    mTextDocument->setPageSize( QSizeF( 980, 1307 ) );
    mTextDocument->setHtml( html );
    
    mkd_cleanup( markdownHandle );
    
    QTextFrameFormat frameFormat;
    frameFormat.setMargin( 45 );

    QTextFrame *rootFrame = mTextDocument->rootFrame();
    rootFrame->setFrameFormat( frameFormat );
    
    convertLinks();
    convertImages();

    return mTextDocument;
}

void Converter::convertLinks()
{
    convertLinks( mTextDocument->rootFrame() );
}


void Converter::convertLinks(QTextFrame * parent)
{
    for ( QTextFrame::iterator it = parent->begin(); !it.atEnd(); ++it ) {
        QTextFrame *textFrame = it.currentFrame();
        QTextBlock textBlock = it.currentBlock();

        if ( textFrame ) {
            convertLinks(textFrame);
        }
        else if ( textBlock.isValid() ) {
            convertLinks(textBlock);
        }
    }
}

void Converter::convertLinks(QTextBlock & parent)
{
    for ( QTextBlock::iterator it = parent.begin(); !it.atEnd(); ++it ) {
        QTextFragment textFragment = it.fragment();
        if ( textFragment.isValid() ) {
            QTextCharFormat textCharFormat = textFragment.charFormat();
            if ( textCharFormat.isAnchor() ) {
                Okular::BrowseAction *action = new Okular::BrowseAction( QUrl( textCharFormat.anchorHref() ) );
                emit addAction( action, textFragment.position(), textFragment.position()+textFragment.length() );
        
            }
        }
    }
}

void Converter::convertImages()
{
    convertImages( mTextDocument->rootFrame() );
}

void Converter::convertImages(QTextFrame * parent)
{
    for ( QTextFrame::iterator it = parent->begin(); !it.atEnd(); ++it ) {
        QTextFrame *textFrame = it.currentFrame();
        QTextBlock textBlock = it.currentBlock();

        if ( textFrame ) {
            convertImages(textFrame);
        }
        else if ( textBlock.isValid() ) {
            convertImages(textBlock);
        }
    }
}

void Converter::convertImages(QTextBlock & parent)
{
    for ( QTextBlock::iterator it = parent.begin(); !it.atEnd(); ++it ) {
        QTextFragment textFragment = it.fragment();
        if ( textFragment.isValid() ) {
            QTextCharFormat textCharFormat = textFragment.charFormat();
            if( textCharFormat.isImageFormat() ) {
                
                //TODO: Show images from http URIs
                
                QTextImageFormat format;
                
                format.setName( QDir::cleanPath( mDir.absoluteFilePath( textCharFormat.toImageFormat().name() ) ) );
                const QImage img = QImage( format.name() );
                
                if ( img.width() > 890 ) {
                    format.setWidth( 890 );
                    format.setHeight( img.height() * 890. / img.width() );
                }
                else {
                    format.setWidth( img.width() );
                    format.setHeight( img.height() );
                }
                
                QTextCursor cursor( mTextDocument );
                cursor.setPosition( textFragment.position(), QTextCursor::MoveAnchor );
                cursor.setPosition( textFragment.position() + textFragment.length(), QTextCursor::KeepAnchor );
                cursor.removeSelectedText();
                cursor.insertImage( format );
    
            }
        }
    }
}
