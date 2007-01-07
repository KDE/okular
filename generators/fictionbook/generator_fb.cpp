/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QTextDocument>

#include <klocale.h>
#include <kprinter.h>

#include <okular/core/link.h>
#include <okular/core/page.h>
#include <okular/core/textpage.h>

#include "converter.h"
#include "document.h"
#include "generator_fb.h"

OKULAR_EXPORT_PLUGIN(FictionBookGenerator)

static void calculateBoundingRect( QTextDocument *document, int startPosition, int endPosition, QRectF &rect, int &page )
{
    const QSizeF pageSize = document->pageSize();

    const QTextBlock startBlock = document->findBlock( startPosition );
    const QRectF startBoundingRect = document->documentLayout()->blockBoundingRect( startBlock );

    const QTextBlock endBlock = document->findBlock( endPosition );
    const QRectF endBoundingRect = document->documentLayout()->blockBoundingRect( endBlock );

    QTextLayout *startLayout = startBlock.layout();
    QTextLayout *endLayout = endBlock.layout();

    int startPos = startPosition - startBlock.position();
    int endPos = endPosition - endBlock.position();
    const QTextLine startLine = startLayout->lineForTextPosition( startPos );
    const QTextLine endLine = endLayout->lineForTextPosition( endPos );

    double x = startBoundingRect.x() + startLine.cursorToX( startPos );
    double y = startBoundingRect.y() + startLine.y();
    double r = endBoundingRect.x() + endLine.cursorToX( endPos );
    double b = endBoundingRect.y() + endLine.y() + endLine.height();

    int offset = qRound( y ) % qRound( pageSize.height() );

    page = qRound( y ) / qRound( pageSize.height() );
    rect = QRectF( x / pageSize.width(), offset / pageSize.height(),
                   (r - x) / pageSize.width(), (b - y) / pageSize.height() );
}

static void calculatePositions( QTextDocument *document, int page, int &start, int &end )
{
    const QAbstractTextDocumentLayout *layout = document->documentLayout();
    const QSizeF pageSize = document->pageSize();
    double margin = document->rootFrame()->frameFormat().margin();

    /**
     * Take the upper left and lower left corner including the margin
     */
    start = layout->hitTest( QPointF( margin, (page * pageSize.height()) + margin ), Qt::FuzzyHit );
    end = layout->hitTest( QPointF( margin, ((page + 1) * pageSize.height()) - margin ), Qt::FuzzyHit );
}

FictionBookGenerator::FictionBookGenerator()
    : Okular::Generator(), mDocument( 0 )
{
}

FictionBookGenerator::~FictionBookGenerator()
{
    delete mDocument;
    mDocument = 0;
}

bool FictionBookGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    FictionBook::Document document( fileName );
    if ( !document.open() )
        return false;

    FictionBook::Converter converter( &document );

    if ( !converter.convert() )
        return false;

    mDocument = converter.textDocument();
    mDocumentInfo = converter.documentInfo();
    mDocumentSynopsis = converter.tableOfContents();
    mLinks = converter.links();

    pagesVector.resize( mDocument->pageCount() );

    const QSize size = mDocument->pageSize().toSize();
    for ( int i = 0; i < mDocument->pageCount(); ++i ) {
        Okular::Page * page = new Okular::Page( i, size.width(), size.height(), Okular::Rotation0 );
        pagesVector[ i ] = page;
    }

    return true;
}

bool FictionBookGenerator::closeDocument()
{
    delete mDocument;
    mDocument = 0;

    return true;
}

bool FictionBookGenerator::canGeneratePixmap( bool ) const
{
    return true;
}

void FictionBookGenerator::generatePixmap( Okular::PixmapRequest * request )
{
    const QSize size = mDocument->pageSize().toSize();

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
    mDocument->drawContents( &p, rect );
    p.end();

    request->page()->setPixmap( request->id(), pixmap );

    /**
     * Add link information
     */
    QLinkedList<Okular::ObjectRect*> objects;
    for ( int i = 0; i < mLinks.count(); ++i ) {
        if ( mLinks[ i ].page == request->pageNumber() ) {
            const QRectF rect = mLinks[ i ].boundingRect;
            objects.append( new Okular::ObjectRect( rect.left(), rect.top(), rect.right(), rect.bottom(), false,
                                                    Okular::ObjectRect::Link, new Okular::LinkBrowse( mLinks[ i ].url ) ) );
        }
    }
    request->page()->setObjectRects( objects );

    signalRequestDone( request );
}

bool FictionBookGenerator::canGenerateTextPage() const
{
    qDebug( "tokoe: can generate text page called" );
    return true;
}

void FictionBookGenerator::generateSyncTextPage( Okular::Page * page )
{
    qDebug( "tokoe: generate sync text page called" );
    page->setTextPage( createTextPage( page->number() ) );
}

bool FictionBookGenerator::print( KPrinter& printer )
{
    QPainter p( &printer );

    const QSize size = mDocument->pageSize().toSize();
    for ( int i = 0; i < mDocument->pageCount(); ++i ) {
        if ( i != 0 )
            printer.newPage();

        QRect rect( 0, i * size.height(), size.width(), size.height() );
        p.translate( QPoint( 0, i * size.height() * -1 ) );
        mDocument->drawContents( &p, rect );
    }

    return true;
}

const Okular::DocumentInfo* FictionBookGenerator::generateDocumentInfo()
{
    return &mDocumentInfo;
}

const Okular::DocumentSynopsis* FictionBookGenerator::generateDocumentSynopsis()
{
    if ( !mDocumentSynopsis.hasChildNodes() )
        return 0;
    else
        return &mDocumentSynopsis;
}

Okular::ExportFormat::List FictionBookGenerator::exportFormats(   ) const
{
    static Okular::ExportFormat::List formats;
    if ( formats.isEmpty() ) {
        formats.append( Okular::ExportFormat( i18n( "PDF" ), KMimeType::mimeType( "application/pdf" ) ) );
        formats.append( Okular::ExportFormat( i18n( "Plain Text" ), KMimeType::mimeType( "text/plain" ) ) );
    }

    return formats;
}

bool FictionBookGenerator::exportTo( const QString &fileName, const Okular::ExportFormat &format )
{
    if ( format.mimeType()->name() == QLatin1String( "application/pdf" ) ) {
        QFile file( fileName );
        if ( !file.open( QIODevice::WriteOnly ) )
            return false;

        QPrinter printer( QPrinter::HighResolution );
        printer.setOutputFormat( QPrinter::PdfFormat );
        printer.setOutputFileName( fileName );
        mDocument->print( &printer );

        return true;
    } else if ( format.mimeType()->name() == QLatin1String( "text/plain" ) ) {
        QFile file( fileName );
        if ( !file.open( QIODevice::WriteOnly ) )
            return false;

        QTextStream out( &file );
        out << mDocument->toPlainText();

        return true;
    }

    return false;
}

Okular::TextPage* FictionBookGenerator::createTextPage( int pageNumber ) const
{
    Okular::TextPage *textPage = new Okular::TextPage;

    int start, end;

    calculatePositions( mDocument, pageNumber, start, end );

    QTextCursor cursor( mDocument );
    for ( int i = start; i < end - 1; ++i ) {
        cursor.setPosition( i );
        cursor.setPosition( i + 1, QTextCursor::KeepAnchor );

        QString text = cursor.selectedText();
        if ( text.length() == 1 && text[ 0 ].isPrint() ) {
            QRectF rect;
            calculateBoundingRect( mDocument, i, i + 1, rect, pageNumber );

            textPage->append( text, new Okular::NormalizedRect( rect.left(), rect.top(), rect.right(), rect.bottom() ) );
        }
    }

    return textPage;
}

#include "generator_fb.moc"

