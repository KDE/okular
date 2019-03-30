/***************************************************************************
 *   Copyright (C) 2017 by Julian Wolff <wolff@julianwolff.de>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "converter.h"

#include "generator_md.h"

#include <KLocalizedString>

#include <QDir>
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
 : m_markdownFile( nullptr )
{
}

Converter::~Converter()
{
    if ( m_markdownFile )
    {
        fclose( m_markdownFile );
    }
}

QTextDocument* Converter::convert( const QString &fileName )
{
    m_markdownFile = fopen( fileName.toLocal8Bit(), "rb" );
    if ( !m_markdownFile ) {
        emit error( i18n( "Failed to open the document" ), -1 );
        return nullptr;
    }

    m_fileDir = QDir( fileName.left( fileName.lastIndexOf( '/' ) ) );

    QTextDocument *doc = convertOpenFile();
    extractLinks( doc->rootFrame() );
    return doc;
}

void Converter::convertAgain()
{
    setDocument( convertOpenFile() );
}

QTextDocument *Converter::convertOpenFile()
{
    rewind( m_markdownFile );

    MMIOT *markdownHandle = mkd_in( m_markdownFile, 0 );
    
    int flags = MKD_FENCEDCODE | MKD_GITHUBTAGS | MKD_AUTOLINK;
    if (!MarkdownGenerator::isFancyPantsEnabled())
        flags |= MKD_NOPANTS;
    if ( !mkd_compile( markdownHandle, flags ) ) {
        emit error( i18n( "Failed to compile the Markdown document." ), -1 );
        return 0;
    }
    
    char *htmlDocument;
    const int size = mkd_document( markdownHandle, &htmlDocument );

    const QString html = QString::fromUtf8( htmlDocument, size );
    
    QTextDocument *textDocument = new QTextDocument;
    textDocument->setPageSize( QSizeF( 980, 1307 ) );
    textDocument->setHtml( html );
    textDocument->setDefaultFont( generator()->generalSettings()->font() );
    
    mkd_cleanup( markdownHandle );
    
    QTextFrameFormat frameFormat;
    frameFormat.setMargin( 45 );

    QTextFrame *rootFrame = textDocument->rootFrame();
    rootFrame->setFrameFormat( frameFormat );
    
    convertImages( rootFrame, m_fileDir, textDocument );

    return textDocument;
}

void Converter::extractLinks(QTextFrame * parent)
{
    for ( QTextFrame::iterator it = parent->begin(); !it.atEnd(); ++it ) {
        QTextFrame *textFrame = it.currentFrame();
        const QTextBlock textBlock = it.currentBlock();

        if ( textFrame ) {
            extractLinks(textFrame);
        } else if ( textBlock.isValid() ) {
            extractLinks(textBlock);
        }
    }
}

void Converter::extractLinks(const QTextBlock & parent)
{
    for ( QTextBlock::iterator it = parent.begin(); !it.atEnd(); ++it ) {
        const QTextFragment textFragment = it.fragment();
        if ( textFragment.isValid() ) {
            const QTextCharFormat textCharFormat = textFragment.charFormat();
            if ( textCharFormat.isAnchor() ) {
                Okular::BrowseAction *action = new Okular::BrowseAction( QUrl( textCharFormat.anchorHref() ) );
                emit addAction( action, textFragment.position(), textFragment.position()+textFragment.length() );
        
            }
        }
    }
}

void Converter::convertImages(QTextFrame * parent, const QDir &dir, QTextDocument *textDocument)
{
    for ( QTextFrame::iterator it = parent->begin(); !it.atEnd(); ++it ) {
        QTextFrame *textFrame = it.currentFrame();
        const QTextBlock textBlock = it.currentBlock();

        if ( textFrame ) {
            convertImages(textFrame, dir, textDocument);
        } else if ( textBlock.isValid() ) {
            convertImages(textBlock, dir, textDocument);
        }
    }
}

void Converter::convertImages(const QTextBlock & parent, const QDir &dir, QTextDocument *textDocument)
{
    for ( QTextBlock::iterator it = parent.begin(); !it.atEnd(); ++it ) {
        const QTextFragment textFragment = it.fragment();
        if ( textFragment.isValid() ) {
            const QTextCharFormat textCharFormat = textFragment.charFormat();
            if( textCharFormat.isImageFormat() ) {
                
                //TODO: Show images from http URIs
                
                QTextImageFormat format;
                
                format.setName( QDir::cleanPath( dir.absoluteFilePath( textCharFormat.toImageFormat().name() ) ) );
                const QImage img = QImage( format.name() );
                
                if ( img.width() > 890 ) {
                    format.setWidth( 890 );
                    format.setHeight( img.height() * 890. / img.width() );
                } else {
                    format.setWidth( img.width() );
                    format.setHeight( img.height() );
                }
                
                QTextCursor cursor( textDocument );
                cursor.setPosition( textFragment.position(), QTextCursor::MoveAnchor );
                cursor.setPosition( textFragment.position() + textFragment.length(), QTextCursor::KeepAnchor );
                cursor.removeSelectedText();
                cursor.insertImage( format );
    
            }
        }
    }
}
