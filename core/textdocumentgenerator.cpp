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
#include <QtCore/QMutex>
#include <QtCore/QStack>
#include <QtCore/QTextStream>
#include <QtCore/QVector>
#include <QtGui/QFontDatabase>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPrinter>
#if QT_VERSION >= 0x040500
#include <QtGui/QTextDocumentWriter>
#endif

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
    : QObject( 0 ), d_ptr( new TextDocumentConverterPrivate )
{
}

TextDocumentConverter::~TextDocumentConverter()
{
    delete d_ptr;
}

DocumentViewport TextDocumentConverter::calculateViewport( QTextDocument *document, const QTextBlock &block )
{
    return TextDocumentUtils::calculateViewport( document, block );
}

TextDocumentGenerator* TextDocumentConverter::generator() const
{
    return d_ptr->mParent ? d_ptr->mParent->q_func() : 0;
}

/**
 * Generic Generator Implementation
 */
Okular::TextPage* TextDocumentGeneratorPrivate::createTextPage( int pageNumber ) const
{
#ifdef OKULAR_TEXTDOCUMENT_THREADED_RENDERING
    Q_Q( const TextDocumentGenerator );
#endif

    Okular::TextPage *textPage = new Okular::TextPage;

    int start, end;

#ifdef OKULAR_TEXTDOCUMENT_THREADED_RENDERING
    q->userMutex()->lock();
#endif
    TextDocumentUtils::calculatePositions( mDocument, pageNumber, start, end );

    {
    QTextCursor cursor( mDocument );
    for ( int i = start; i < end - 1; ++i ) {
        cursor.setPosition( i );
        cursor.setPosition( i + 1, QTextCursor::KeepAnchor );

        QString text = cursor.selectedText();
        if ( text.length() == 1 ) {
            QRectF rect;
            TextDocumentUtils::calculateBoundingRect( mDocument, i, i + 1, rect, pageNumber );
            if ( pageNumber == -1 )
                text = "\n";

            textPage->append( text, new Okular::NormalizedRect( rect.left(), rect.top(), rect.right(), rect.bottom() ) );
        }
    }
    }
#ifdef OKULAR_TEXTDOCUMENT_THREADED_RENDERING
    q->userMutex()->unlock();
#endif

    return textPage;
}

void TextDocumentGeneratorPrivate::addAction( Action *action, int cursorBegin, int cursorEnd )
{
    if ( !action )
        return;

    LinkPosition position;
    position.link = action;
    position.startPosition = cursorBegin;
    position.endPosition = cursorEnd;

    mLinkPositions.append( position );
}

void TextDocumentGeneratorPrivate::addAnnotation( Annotation *annotation, int cursorBegin, int cursorEnd )
{
    if ( !annotation )
        return;

    annotation->setFlags( annotation->flags() | Okular::Annotation::External );

    AnnotationPosition position;
    position.annotation = annotation;
    position.startPosition = cursorBegin;
    position.endPosition = cursorEnd;

    mAnnotationPositions.append( position );
}

void TextDocumentGeneratorPrivate::addTitle( int level, const QString &title, const QTextBlock &block )
{
    TitlePosition position;
    position.level = level;
    position.title = title;
    position.block = block;

    mTitlePositions.append( position );
}

void TextDocumentGeneratorPrivate::addMetaData( const QString &key, const QString &value, const QString &title )
{
    mDocumentInfo.set( key, value, title );
}

void TextDocumentGeneratorPrivate::addMetaData( DocumentInfo::Key key, const QString &value )
{
    mDocumentInfo.set( key, value );
}

void TextDocumentGeneratorPrivate::generateLinkInfos()
{
    for ( int i = 0; i < mLinkPositions.count(); ++i ) {
        const LinkPosition &linkPosition = mLinkPositions[ i ];

        LinkInfo info;
        info.link = linkPosition.link;

        TextDocumentUtils::calculateBoundingRect( mDocument, linkPosition.startPosition, linkPosition.endPosition,
                                                  info.boundingRect, info.page );

        if ( info.page >= 0 )
            mLinkInfos.append( info );
    }
}

void TextDocumentGeneratorPrivate::generateAnnotationInfos()
{
    for ( int i = 0; i < mAnnotationPositions.count(); ++i ) {
        const AnnotationPosition &annotationPosition = mAnnotationPositions[ i ];

        AnnotationInfo info;
        info.annotation = annotationPosition.annotation;

        TextDocumentUtils::calculateBoundingRect( mDocument, annotationPosition.startPosition, annotationPosition.endPosition,
                                                  info.boundingRect, info.page );

        if ( info.page >= 0 )
            mAnnotationInfos.append( info );
    }
}

void TextDocumentGeneratorPrivate::generateTitleInfos()
{
    QStack< QPair<int,QDomNode> > parentNodeStack;

    QDomNode parentNode = mDocumentSynopsis;

    parentNodeStack.push( qMakePair( 0, parentNode ) );

    for ( int i = 0; i < mTitlePositions.count(); ++i ) {
        const TitlePosition &position = mTitlePositions[ i ];

        Okular::DocumentViewport viewport = TextDocumentUtils::calculateViewport( mDocument, position.block );

        QDomElement item = mDocumentSynopsis.createElement( position.title );
        item.setAttribute( "Viewport", viewport.toString() );

        int headingLevel = position.level;

        // we need a parent, which has to be at a higher heading level than this heading level
        // so we just work through the stack
        while ( ! parentNodeStack.isEmpty() ) {
            int parentLevel = parentNodeStack.top().first;
            if ( parentLevel < headingLevel ) {
                // this is OK as a parent
                parentNode = parentNodeStack.top().second;
                break;
            } else {
                // we'll need to be further into the stack
                parentNodeStack.pop();
            }
        }
        parentNode.appendChild( item );
        parentNodeStack.push( qMakePair( headingLevel, QDomNode(item) ) );
    }
}

TextDocumentGenerator::TextDocumentGenerator( TextDocumentConverter *converter, QObject *parent, const QVariantList &args )
    : Okular::Generator( *new TextDocumentGeneratorPrivate( converter ), parent, args )
{
    converter->d_ptr->mParent = d_func();

    setFeature( TextExtraction );
    setFeature( PrintNative );
    setFeature( PrintToFile );
#ifdef OKULAR_TEXTDOCUMENT_THREADED_RENDERING
    if ( QFontDatabase::supportsThreadedFontRendering() )
        setFeature( Threaded );
#endif

    connect( converter, SIGNAL( addAction( Action*, int, int ) ),
             this, SLOT( addAction( Action*, int, int ) ) );
    connect( converter, SIGNAL( addAnnotation( Annotation*, int, int ) ),
             this, SLOT( addAnnotation( Annotation*, int, int ) ) );
    connect( converter, SIGNAL( addTitle( int, const QString&, const QTextBlock& ) ),
             this, SLOT( addTitle( int, const QString&, const QTextBlock& ) ) );
    connect( converter, SIGNAL( addMetaData( const QString&, const QString&, const QString& ) ),
             this, SLOT( addMetaData( const QString&, const QString&, const QString& ) ) );
    connect( converter, SIGNAL( addMetaData( DocumentInfo::Key, const QString& ) ),
             this, SLOT( addMetaData( DocumentInfo::Key, const QString& ) ) );

    connect( converter, SIGNAL( error( const QString&, int ) ),
             this, SIGNAL( error( const QString&, int ) ) );
    connect( converter, SIGNAL( warning( const QString&, int ) ),
             this, SIGNAL( warning( const QString&, int ) ) );
    connect( converter, SIGNAL( notice( const QString&, int ) ),
             this, SIGNAL( notice( const QString&, int ) ) );
}

TextDocumentGenerator::~TextDocumentGenerator()
{
}

bool TextDocumentGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    Q_D( TextDocumentGenerator );
    d->mDocument = d->mConverter->convert( fileName );

    if ( !d->mDocument )
    {
        // loading failed, cleanup all the stuff eventually gathered from the converter
        d->mTitlePositions.clear();
        Q_FOREACH ( const TextDocumentGeneratorPrivate::LinkPosition &linkPos, d->mLinkPositions )
        {
            delete linkPos.link;
        }
        d->mLinkPositions.clear();
        Q_FOREACH ( const TextDocumentGeneratorPrivate::AnnotationPosition &annPos, d->mAnnotationPositions )
        {
            delete annPos.annotation;
        }
        d->mAnnotationPositions.clear();

        return false;
    }

    d->generateTitleInfos();
    d->generateLinkInfos();
    d->generateAnnotationInfos();

    pagesVector.resize( d->mDocument->pageCount() );

    const QSize size = d->mDocument->pageSize().toSize();

    QVector< QLinkedList<Okular::ObjectRect*> > objects( d->mDocument->pageCount() );
    for ( int i = 0; i < d->mLinkInfos.count(); ++i ) {
        const TextDocumentGeneratorPrivate::LinkInfo &info = d->mLinkInfos.at( i );

        const QRectF rect = info.boundingRect;
        objects[ info.page ].append( new Okular::ObjectRect( rect.left(), rect.top(), rect.right(), rect.bottom(), false,
                                                             Okular::ObjectRect::Action, info.link ) );
    }

    QVector< QLinkedList<Okular::Annotation*> > annots( d->mDocument->pageCount() );
    for ( int i = 0; i < d->mAnnotationInfos.count(); ++i ) {
        const TextDocumentGeneratorPrivate::AnnotationInfo &info = d->mAnnotationInfos[ i ];

        QRect rect( 0, info.page * size.height(), size.width(), size.height() );
        info.annotation->setBoundingRectangle( Okular::NormalizedRect( rect.left(), rect.top(), rect.right(), rect.bottom() ) );
        annots[ info.page ].append( info.annotation );
    }

    for ( int i = 0; i < d->mDocument->pageCount(); ++i ) {
        Okular::Page * page = new Okular::Page( i, size.width(), size.height(), Okular::Rotation0 );
        pagesVector[ i ] = page;

        if ( !objects.at( i ).isEmpty() ) {
            page->setObjectRects( objects.at( i ) );
        }
        QLinkedList<Okular::Annotation*>::ConstIterator annIt = annots.at( i ).begin(), annEnd = annots.at( i ).end();
        for ( ; annIt != annEnd; ++annIt ) {
            page->addAnnotation( *annIt );
        }
    }

    return true;
}

bool TextDocumentGenerator::doCloseDocument()
{
    Q_D( TextDocumentGenerator );
    delete d->mDocument;
    d->mDocument = 0;

    d->mTitlePositions.clear();
    d->mLinkPositions.clear();
    d->mLinkInfos.clear();
    d->mAnnotationPositions.clear();
    d->mAnnotationInfos.clear();
    // do not use clear() for the following two, otherwise they change type
    d->mDocumentInfo = Okular::DocumentInfo();
    d->mDocumentSynopsis = Okular::DocumentSynopsis();

    return true;
}

bool TextDocumentGenerator::canGeneratePixmap() const
{
    return Generator::canGeneratePixmap();
}

void TextDocumentGenerator::generatePixmap( Okular::PixmapRequest * request )
{
    Generator::generatePixmap( request );
}

QImage TextDocumentGeneratorPrivate::image( PixmapRequest * request )
{
    if ( !mDocument )
        return QImage();

#ifdef OKULAR_TEXTDOCUMENT_THREADED_RENDERING
    Q_Q( TextDocumentGenerator );
#endif

    QImage image( request->width(), request->height(), QImage::Format_ARGB32 );
    image.fill( Qt::white );

    QPainter p;
    p.begin( &image );

    qreal width = request->width();
    qreal height = request->height();

    const QSize size = mDocument->pageSize().toSize();

    p.scale( width / (qreal)size.width(), height / (qreal)size.height() );

    QRect rect;
    rect = QRect( 0, request->pageNumber() * size.height(), size.width(), size.height() );
    p.translate( QPoint( 0, request->pageNumber() * size.height() * -1 ) );
#ifdef OKULAR_TEXTDOCUMENT_THREADED_RENDERING
    q->userMutex()->lock();
#endif
    mDocument->drawContents( &p, rect );
#ifdef OKULAR_TEXTDOCUMENT_THREADED_RENDERING
    q->userMutex()->unlock();
#endif
    p.end();

    return image;
}

Okular::TextPage* TextDocumentGenerator::textPage( Okular::Page * page )
{
    Q_D( TextDocumentGenerator );
    return d->createTextPage( page->number() );
}

bool TextDocumentGenerator::print( QPrinter& printer )
{
    Q_D( TextDocumentGenerator );
    if ( !d->mDocument )
        return false;

    d->mDocument->print( &printer );

    return true;
}

const Okular::DocumentInfo* TextDocumentGenerator::generateDocumentInfo()
{
    Q_D( TextDocumentGenerator );
    return &d->mDocumentInfo;
}

const Okular::DocumentSynopsis* TextDocumentGenerator::generateDocumentSynopsis()
{
    Q_D( TextDocumentGenerator );
    if ( !d->mDocumentSynopsis.hasChildNodes() )
        return 0;
    else
        return &d->mDocumentSynopsis;
}

QVariant TextDocumentGeneratorPrivate::metaData( const QString &key, const QVariant &option ) const
{
    Q_UNUSED( option )
    if ( key == "DocumentTitle" )
    {
        return mDocumentInfo.get( "title" );
    }
    return QVariant();
}

Okular::ExportFormat::List TextDocumentGenerator::exportFormats(   ) const
{
    static Okular::ExportFormat::List formats;
    if ( formats.isEmpty() ) {
        formats.append( Okular::ExportFormat::standardFormat( Okular::ExportFormat::PlainText ) );
        formats.append( Okular::ExportFormat::standardFormat( Okular::ExportFormat::PDF ) );
#if QT_VERSION >= 0x040500
	if ( QTextDocumentWriter::supportedDocumentFormats().contains( "ODF" ) ) {
	    formats.append( Okular::ExportFormat::standardFormat( Okular::ExportFormat::OpenDocumentText ) );
	}
	if ( QTextDocumentWriter::supportedDocumentFormats().contains( "HTML" ) ) {
	    formats.append( Okular::ExportFormat::standardFormat( Okular::ExportFormat::HTML ) );
	}
#endif
    }

    return formats;
}

bool TextDocumentGenerator::exportTo( const QString &fileName, const Okular::ExportFormat &format )
{
    Q_D( TextDocumentGenerator );
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
#if QT_VERSION >= 0x040500
    } else if ( format.mimeType()->name() == QLatin1String( "application/vnd.oasis.opendocument.text" ) ) {
	QTextDocumentWriter odfWriter( fileName, "odf" );

	return odfWriter.write( d->mDocument );
    } else if ( format.mimeType()->name() == QLatin1String( "text/html" ) ) {
	QTextDocumentWriter odfWriter( fileName, "html" );

	return odfWriter.write( d->mDocument );
#endif
    }
    return false;
}

#include "textdocumentgenerator.moc"

