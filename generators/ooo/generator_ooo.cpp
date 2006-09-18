/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>

#include <qpainter.h>
#include <qpixmap.h>
#include <qimage.h>
#include <kimageeffect.h>
#include <kprinter.h>

#include "core/page.h"
#include "generator_ooo.h"

#include "document.h"
#include "converter.h"

OKULAR_EXPORT_PLUGIN(KOOOGenerator)

KOOOGenerator::KOOOGenerator( KPDFDocument * document )
  : Generator( document ), mDocument( 0 )
{
}

KOOOGenerator::~KOOOGenerator()
{
  delete mDocument;
  mDocument = 0;
}

bool KOOOGenerator::loadDocument( const QString & fileName, QVector<KPDFPage*> & pagesVector )
{
  OOO::Document document( fileName );
  if ( !document.open() )
    return false;

  OOO::Converter converter( &document );

  if ( !converter.convert() )
    return false;

  mDocument = converter.textDocument();
  mDocumentSynopsis = converter.tableOfContents();

  OOO::MetaInformation::List metaInformation = converter.metaInformation();
  for ( int i = 0; i < metaInformation.count(); ++i ) {
    mDocumentInfo.set( metaInformation[ i ].key(),
                       metaInformation[ i ].value(),
                       metaInformation[ i ].title() );
  }

  pagesVector.resize( mDocument->pageCount() );

  const QSize size = mDocument->pageSize().toSize();
  for ( int i = 0; i < mDocument->pageCount(); ++i ) {
    KPDFPage * page = new KPDFPage( i, size.width(), size.height(), 0 );
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

bool KOOOGenerator::canGeneratePixmap( bool )
{
  return true;
}

void KOOOGenerator::generatePixmap( PixmapRequest * request )
{
  const QSize size = mDocument->pageSize().toSize();

  QPixmap *pm = new QPixmap( request->width, request->height );
  pm->fill( Qt::white );


  QPainter p;
  p.begin( pm );

  qreal width = request->width;
  qreal height = request->height;

  if ( request->documentRotation % 2 == 1 )
    qSwap( width, height );

  switch ( request->documentRotation ) {
    case 0:
    default:
      break;
    case 1:
      p.rotate( 90 );
      p.translate( QPoint( 0, qRound(-size.height() * (height / (qreal)size.height())) ) );
      break;
    case 2:
      p.rotate( 180 );
      p.translate( QPoint( qRound(-size.width() * (width / (qreal)size.width())),
                           qRound(-size.height() * (height / (qreal)size.height())) ) );
      break;
    case 3:
      p.rotate( 270 );
      p.translate( QPoint( qRound(-size.width() * (width / (qreal)size.width())), 0 ) );
      break;
  }

  p.scale( width / (qreal)size.width(), height / (qreal)size.height() );

  QRect rect;
  rect = QRect( 0, request->pageNumber * size.height(), size.width(), size.height() );
  p.translate( QPoint( 0, request->pageNumber * size.height() * -1 ) );
  mDocument->drawContents( &p, rect );
  p.end();

  request->page->setPixmap( request->id, pm );

  signalRequestDone( request );
}

void KOOOGenerator::setOrientation( QVector<KPDFPage*> & pagesVector, int orientation )
{
  const QSize size = mDocument->pageSize().toSize();

  int width = size.width();
  int height = size.height();

  if ( orientation % 2 == 1 )
    qSwap( width, height );

  qDeleteAll( pagesVector );

  for ( int i = 0; i < mDocument->pageCount(); ++i ) {
    KPDFPage * page = new KPDFPage( i, width, height, 0 );
    pagesVector[ i ] = page;
  }
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

const DocumentInfo* KOOOGenerator::generateDocumentInfo()
{
  return &mDocumentInfo;
}

const DocumentSynopsis* KOOOGenerator::generateDocumentSynopsis()
{
  if ( !mDocumentSynopsis.hasChildNodes() )
    return 0;
  else
    return &mDocumentSynopsis;
}
#include "generator_ooo.moc"

