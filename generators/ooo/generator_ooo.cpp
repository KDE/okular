/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include <kprinter.h>
#include <klocale.h>

#include "converter.h"
#include "core/link.h"
#include "core/page.h"
#include "document.h"

#include "generator_ooo.h"

OKULAR_EXPORT_PLUGIN(KOOOGenerator)

KOOOGenerator::KOOOGenerator()
  : Okular::Generator(), mDocument( 0 )
{
}

KOOOGenerator::~KOOOGenerator()
{
  delete mDocument;
  mDocument = 0;
}

bool KOOOGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
  OOO::Document document( fileName );
  if ( !document.open() )
    return false;

  OOO::Converter converter( &document );

  if ( !converter.convert() )
    return false;

  mDocument = converter.textDocument();
  mDocumentSynopsis = converter.tableOfContents();
  mLinks = converter.links();

  OOO::MetaInformation::List metaInformation = converter.metaInformation();
  for ( int i = 0; i < metaInformation.count(); ++i ) {
    mDocumentInfo.set( metaInformation[ i ].key(),
                       metaInformation[ i ].value(),
                       metaInformation[ i ].title() );
  }

  pagesVector.resize( mDocument->pageCount() );

  const QSize size = mDocument->pageSize().toSize();
  for ( int i = 0; i < mDocument->pageCount(); ++i ) {
    Okular::Page * page = new Okular::Page( i, size.width(), size.height(), 0 );
    pagesVector[ i ] = page;
  }

  return true;
}

bool KOOOGenerator::closeDocument()
{
  delete mDocument;
  mDocument = 0;

  return true;
}

bool KOOOGenerator::canGeneratePixmap( bool ) const
{
  return true;
}

void KOOOGenerator::generatePixmap( Okular::PixmapRequest * request )
{
  const QSize size = mDocument->pageSize().toSize();

  QPixmap pixmap = QPixmap( request->width(), request->height() );
  pixmap.fill( Qt::white );

  QPainter p;
  p.begin( &pixmap );

  qreal width = request->width();
  qreal height = request->height();

  p.scale( width / (qreal)size.width(), height / (qreal)size.height() );

  QRect rect;
  rect = QRect( 0, request->pageNumber() * size.height(), size.width(), size.height() );
  p.translate( QPoint( 0, request->pageNumber() * size.height() * -1 ) );
  mDocument->drawContents( &p, rect );
  p.end();

  request->page()->setImage( request->id(), pixmap.toImage() );

  /**
   * Add link information
   */
  QLinkedList<Okular::ObjectRect*> objects;
  for ( int i = 0; i < mLinks.count(); ++i ) {
    if ( mLinks[ i ].page == request->pageNumber() ) {
      const QRectF rect = mLinks[ i ].boundingRect;
      double x = rect.x() / request->width();
      double y = rect.y() / request->height();
      double w = rect.width() / request->width();
      double h = rect.height() / request->height();
      objects.append( new Okular::ObjectRect( x, y, w, h, false,
                                              Okular::ObjectRect::Link, new Okular::LinkBrowse( mLinks[ i ].url ) ) );
    }
  }
  request->page()->setObjectRects( objects );

  signalRequestDone( request );
}

bool KOOOGenerator::print( KPrinter& printer )
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

const Okular::DocumentInfo* KOOOGenerator::generateDocumentInfo()
{
  return &mDocumentInfo;
}

const Okular::DocumentSynopsis* KOOOGenerator::generateDocumentSynopsis()
{
  if ( !mDocumentSynopsis.hasChildNodes() )
    return 0;
  else
    return &mDocumentSynopsis;
}

Okular::ExportFormat::List KOOOGenerator::exportFormats(   ) const
{
  static Okular::ExportFormat::List formats;
  if ( formats.isEmpty() ) {
    formats.append( Okular::ExportFormat( i18n( "PDF" ), KMimeType::mimeType( "application/pdf" ) ) );
    formats.append( Okular::ExportFormat( i18n( "Plain Text" ), KMimeType::mimeType( "text/plain" ) ) );
  }

  return formats;
}

bool KOOOGenerator::exportTo( const QString &fileName, const Okular::ExportFormat &format )
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

#include "generator_ooo.moc"

