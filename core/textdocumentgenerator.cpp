/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "textdocumentgenerator.h"
#include "textdocumentgenerator_p.h"

#include <QtCore/QFile>
#include <QtCore/QStack>
#include <QtCore/QTextStream>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include <klocale.h>
#include <kprinter.h>

#include <okular/core/action.h>
#include <okular/core/annotations.h>
#include <okular/core/page.h>
#include <okular/core/textpage.h>

#include "document.h"

using namespace Okular;

/**
 * Generic Converter Implementation
 */
TextDocumentConverter::TextDocumentConverter()
    : QObject( 0 ), d( new Private )
{
}

TextDocumentConverter::~TextDocumentConverter()
{
    delete d;
}

DocumentViewport TextDocumentConverter::calculateViewport( QTextDocument *document, const QTextBlock &block )
{
    return Utils::calculateViewport( document, block );
}

/**
 * Generic Generator Implementation
 */
Okular::TextPage* TextDocumentGenerator::Private::createTextPage( int pageNumber ) const
{
    Okular::TextPage *textPage = new Okular::TextPage;

    int start, end;

    Utils::calculatePositions( mDocument, pageNumber, start, end );

    QTextCursor cursor( mDocument );
    for ( int i = start; i < end - 1; ++i ) {
        cursor.setPosition( i );
        cursor.setPosition( i + 1, QTextCursor::KeepAnchor );

        QString text = cursor.selectedText();
        if ( text.length() == 1 && text[ 0 ].isPrint() ) {
            QRectF rect;
            Utils::calculateBoundingRect( mDocument, i, i + 1, rect, pageNumber );

            textPage->append( text, new Okular::NormalizedRect( rect.left(), rect.top(), rect.right(), rect.bottom() ) );
        }
    }

    return textPage;
}

void TextDocumentGenerator::Private::addAction( Action *action, int cursorBegin, int cursorEnd )
{
    if ( !action )
        return;

    LinkPosition position;
    position.link = action;
    position.startPosition = cursorBegin;
    position.endPosition = cursorEnd;

    mLinkPositions.append( position );
}

void TextDocumentGenerator::Private::addAnnotation( Annotation *annotation, int cursorBegin, int cursorEnd )
{
    if ( !annotation )
        return;

    AnnotationPosition position;
    position.annotation = annotation;
    position.startPosition = cursorBegin;
    position.endPosition = cursorEnd;

    mAnnotationPositions.append( position );
}

void TextDocumentGenerator::Private::addTitle( int level, const QString &title, const QTextBlock &block )
{
    TitlePosition position;
    position.level = level;
    position.title = title;
    position.block = block;

    mTitlePositions.append( position );
}

void TextDocumentGenerator::Private::addMetaData( const QString &key, const QString &value, const QString &title )
{
    mDocumentInfo.set( key, value, title );
}

void TextDocumentGenerator::Private::generateLinkInfos()
{
    for ( int i = 0; i < mLinkPositions.count(); ++i ) {
        const LinkPosition &linkPosition = mLinkPositions[ i ];

        LinkInfo info;
        info.link = linkPosition.link;

        Utils::calculateBoundingRect( mDocument, linkPosition.startPosition, linkPosition.endPosition,
                                      info.boundingRect, info.page );

        mLinkInfos.append( info );
    }
}

void TextDocumentGenerator::Private::generateAnnotationInfos()
{
    for ( int i = 0; i < mAnnotationPositions.count(); ++i ) {
        const AnnotationPosition &annotationPosition = mAnnotationPositions[ i ];

        AnnotationInfo info;
        info.annotation = annotationPosition.annotation;

        Utils::calculateBoundingRect( mDocument, annotationPosition.startPosition, annotationPosition.endPosition,
                                      info.boundingRect, info.page );

        mAnnotationInfos.append( info );
    }
}

void TextDocumentGenerator::Private::generateTitleInfos()
{
    QStack<QDomNode> parentNodeStack;

    QDomNode parentNode = mDocumentSynopsis;

    int level = 1000;
    for ( int i = 0; i < mTitlePositions.count(); ++i )
        level = qMin( level, mTitlePositions[ i ].level );

    for ( int i = 0; i < mTitlePositions.count(); ++i ) {
        const TitlePosition &position = mTitlePositions[ i ];

        Okular::DocumentViewport viewport = Utils::calculateViewport( mDocument, position.block );

        QDomElement item = mDocumentSynopsis.createElement( position.title );
        item.setAttribute( "Viewport", viewport.toString() );

        int newLevel = position.level;
        if ( newLevel == level ) {
            parentNode.appendChild( item );
        } else if ( newLevel > level ) {
            parentNodeStack.push( parentNode );
            parentNode = parentNode.lastChildElement();
            parentNode.appendChild( item );
            level = newLevel;
        } else {
            for ( int i = level; i > newLevel; i-- ) {
                level--;
                parentNode = parentNodeStack.pop();
            }

            parentNode.appendChild( item );
        }
    }
}

TextDocumentGenerator::TextDocumentGenerator( TextDocumentConverter *converter )
    : Okular::Generator(), d( new Private( this, converter ) )
{
    setFeature( TextExtraction );

    connect( converter, SIGNAL( addAction( Action*, int, int ) ),
             this, SLOT( addAction( Action*, int, int ) ) );
    connect( converter, SIGNAL( addAnnotation( Annotation*, int, int ) ),
             this, SLOT( addAnnotation( Annotation*, int, int ) ) );
    connect( converter, SIGNAL( addTitle( int, const QString&, const QTextBlock& ) ),
             this, SLOT( addTitle( int, const QString&, const QTextBlock& ) ) );
    connect( converter, SIGNAL( addMetaData( const QString&, const QString&, const QString& ) ),
             this, SLOT( addMetaData( const QString&, const QString&, const QString& ) ) );

    connect( converter, SIGNAL( error( const QString&, int ) ),
             this, SIGNAL( error( const QString&, int ) ) );
    connect( converter, SIGNAL( warning( const QString&, int ) ),
             this, SIGNAL( warning( const QString&, int ) ) );
    connect( converter, SIGNAL( notice( const QString&, int ) ),
             this, SIGNAL( notice( const QString&, int ) ) );
}

TextDocumentGenerator::~TextDocumentGenerator()
{
    delete d;
}

bool TextDocumentGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    d->mTitlePositions.clear();
    d->mLinkPositions.clear();
    d->mLinkAddedList.clear();
    d->mAnnotationAddedList.clear();
    d->mLinkInfos.clear();
    d->mAnnotationInfos.clear();

    d->mDocument = d->mConverter->convert( fileName );

    if ( !d->mDocument )
        return false;

    d->generateTitleInfos();
    d->generateLinkInfos();
    d->generateAnnotationInfos();

    pagesVector.resize( d->mDocument->pageCount() );

    const QSize size = d->mDocument->pageSize().toSize();
    for ( int i = 0; i < d->mDocument->pageCount(); ++i ) {
        Okular::Page * page = new Okular::Page( i, size.width(), size.height(), Okular::Rotation0 );
        pagesVector[ i ] = page;
    }

    return true;
}

bool TextDocumentGenerator::closeDocument()
{
    delete d->mDocument;
    d->mDocument = 0;

    return true;
}

bool TextDocumentGenerator::canGeneratePixmap() const
{
    return true;
}

void TextDocumentGenerator::generatePixmap( Okular::PixmapRequest * request )
{
    if ( !d->mDocument )
        return;

    const QSize size = d->mDocument->pageSize().toSize();

    QPixmap *pixmap = new QPixmap( request->width(), request->height() );
    pixmap->fill( Qt::white );

    QPainter p;
    p.begin( pixmap );

    qreal width = request->width();
    qreal height = request->height();

    p.scale( width / (qreal)size.width(), height / (qreal)size.height() );

    QRect rect;
    rect = QRect( 0, request->pageNumber() * size.height(), size.width(), size.height() );
    p.translate( QPoint( 0, request->pageNumber() * size.height() * -1 ) );
    d->mDocument->drawContents( &p, rect );
    p.end();

    request->page()->setPixmap( request->id(), pixmap );

    /**
     * Add link information
     */
    if ( !d->mLinkAddedList.contains( request->pageNumber() ) ) {
        QLinkedList<Okular::ObjectRect*> objects;
        for ( int i = 0; i < d->mLinkInfos.count(); ++i ) {
            const Private::LinkInfo &info = d->mLinkInfos[ i ];

            if ( info.page == request->pageNumber() ) {
                const QRectF rect = info.boundingRect;
                objects.append( new Okular::ObjectRect( rect.left(), rect.top(), rect.right(), rect.bottom(), false,
                                                        Okular::ObjectRect::Action, info.link ) );
            }
        }
        request->page()->setObjectRects( objects );

        d->mLinkAddedList.insert( request->pageNumber() );
    }

    /**
     * Add annotations
     */
    if ( !d->mAnnotationAddedList.contains( request->pageNumber() ) ) {
        for ( int i = 0; i < d->mAnnotationInfos.count(); ++i ) {
            const Private::AnnotationInfo &info = d->mAnnotationInfos[ i ];

            if ( info.page == request->pageNumber() ) {
                info.annotation->setBoundingRectangle( Okular::NormalizedRect( rect.left(), rect.top(), rect.right(), rect.bottom() ) );

                request->page()->addAnnotation( info.annotation );
            }
        }

        d->mAnnotationAddedList.insert( request->pageNumber() );
    }

    signalPixmapRequestDone( request );
}

Okular::TextPage* TextDocumentGenerator::textPage( Okular::Page * page )
{
    return d->createTextPage( page->number() );
}

bool TextDocumentGenerator::print( KPrinter& printer )
{
    if ( !d->mDocument )
        return false;

    QPainter p( &printer );

    const QSize size = d->mDocument->pageSize().toSize();
    for ( int i = 0; i < d->mDocument->pageCount(); ++i ) {
        if ( i != 0 )
            printer.newPage();

        QRect rect( 0, i * size.height(), size.width(), size.height() );
        p.translate( QPoint( 0, i * size.height() * -1 ) );
        d->mDocument->drawContents( &p, rect );
    }

    return true;
}

const Okular::DocumentInfo* TextDocumentGenerator::generateDocumentInfo()
{
    return &d->mDocumentInfo;
}

const Okular::DocumentSynopsis* TextDocumentGenerator::generateDocumentSynopsis()
{
    if ( !d->mDocumentSynopsis.hasChildNodes() )
        return 0;
    else
        return &d->mDocumentSynopsis;
}

Okular::ExportFormat::List TextDocumentGenerator::exportFormats(   ) const
{
    static Okular::ExportFormat::List formats;
    if ( formats.isEmpty() ) {
        formats.append( Okular::ExportFormat::plainText() );
        formats.append( Okular::ExportFormat( i18n( "PDF" ), KMimeType::mimeType( "application/pdf" ) ) );
    }

    return formats;
}

bool TextDocumentGenerator::exportTo( const QString &fileName, const Okular::ExportFormat &format )
{
    if ( !d->mDocument )
        return false;

    if ( format.mimeType()->name() == QLatin1String( "application/pdf" ) ) {
        QFile file( fileName );
        if ( !file.open( QIODevice::WriteOnly ) )
            return false;

        QPrinter printer( QPrinter::HighResolution );
        printer.setOutputFormat( QPrinter::PdfFormat );
        printer.setOutputFileName( fileName );
        d->mDocument->print( &printer );

        return true;
    } else if ( format.mimeType()->name() == QLatin1String( "text/plain" ) ) {
        QFile file( fileName );
        if ( !file.open( QIODevice::WriteOnly ) )
            return false;

        QTextStream out( &file );
        out << d->mDocument->toPlainText();

        return true;
    }

    return false;
}

#include "textdocumentgenerator.moc"

