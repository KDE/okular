/***************************************************************************
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QTextStream>

#include "generator.h"

using namespace Okular;

Generator::Generator()
 : m_document( 0 )
{
}

Generator::~Generator()
{
}

bool Generator::canGenerateTextPage() const
{
  return false;
}

void Generator::generateSyncTextPage( Page* )
{
}

const DocumentInfo * Generator::generateDocumentInfo()
{
  return 0;
}

const DocumentSynopsis * Generator::generateDocumentSynopsis()
{
  return 0;
}

const DocumentFonts * Generator::generateDocumentFonts()
{
  return 0;
}

const QList<EmbeddedFile*> * Generator::embeddedFiles() const
{
  return 0;
}

Generator::PageSizeMetric Generator::pagesSizeMetric() const
{
  return None;
}

bool Generator::isAllowed( int ) const
{
  return true;
}

QString Generator::getXMLFile() const
{
  return QString();
}

void Generator::setupGUI( KActionCollection*, QToolBox* )
{
}

void Generator::freeGUI()
{
}

bool Generator::supportsSearching() const
{
  return false;
}

bool Generator::prefersInternalSearching() const
{
  return false;
}

RegularAreaRect * Generator::findText( const QString&, SearchDir, const bool,
                                       const RegularAreaRect*, Page* ) const
{
  return 0;
}

QString Generator::getText( const RegularAreaRect*, Page* ) const
{
  return QString();
}

bool Generator::supportsRotation() const
{
  return false;
}

void Generator::rotationChanged( int, int )
{
}

bool Generator::supportsPaperSizes () const
{
  return false;
}

QStringList Generator::paperSizes () const
{
  return QStringList();
}

void Generator::setPaperSize( QVector<Page*>&, int )
{
}

bool Generator::canConfigurePrinter() const
{
  return false;
}

bool Generator::print( KPrinter& )
{
  return false;
}

QString Generator::metaData( const QString&, const QString& ) const
{
  return QString();
}

bool Generator::reparseConfig()
{
  return false;
}

void Generator::addPages( KConfigDialog* )
{
}

bool Generator::canExportToText() const
{
  return false;
}

bool Generator::exportToText( const QString& )
{
  return false;
}

QList<ExportEntry*> Generator::exportFormats() const
{
  return QList<ExportEntry*>();
}

bool Generator::exportTo( const QString&, const KMimeType::Ptr& )
{
  return false;
}

bool Generator::handleEvent( QEvent* )
{
  return true;
}

void Generator::setDocument( Document *document )
{
  m_document = document;
}

void Generator::signalRequestDone( PixmapRequest * request )
{
  m_document->requestDone( request );
}

Document * Generator::document() const
{
    return m_document;
}


QTextStream& operator<< (QTextStream& str, const PixmapRequest *req)
{
    QString s;
    s += QString(req->async ? "As" : "S") + QString("ync PixmapRequest (id: %1) (%2x%3) ").arg(req->id,req->width,req->height);
    s += QString("prio: %1, pageNo: %2) ").arg(req->priority,req->pageNumber);
    return (str << s);
}

#include "generator.moc"
