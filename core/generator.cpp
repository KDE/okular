/***************************************************************************
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <kdebug.h>
#include <kicon.h>

#include "document.h"
#include "generator.h"

using namespace Okular;

class Generator::Private
{
    public:
        Private()
            : m_document( 0 )
        {
        }

        Document * m_document;
};

Generator::Generator()
    : d( new Private )
{
}

Generator::~Generator()
{
    delete d;
}

bool Generator::loadDocumentFromData( const QByteArray &, QVector< Page * > & )
{
    return false;
}

bool Generator::canRequestPixmap() const
{
    return canGeneratePixmap();
}

void Generator::requestPixmap( PixmapRequest *request )
{
    generatePixmap( request );
}

void Generator::requestTextPage( Page *page )
{
    generateSyncTextPage( page );
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

bool Generator::isAllowed( Permissions ) const
{
    return true;
}

void Generator::rotationChanged( Rotation, Rotation )
{
}

PageSize::List Generator::pageSizes() const
{
    return PageSize::List();
}

void Generator::pageSizeChanged( const PageSize &, const PageSize & )
{
}

bool Generator::print( KPrinter& )
{
    return false;
}

QVariant Generator::metaData( const QString&, const QVariant& ) const
{
    return QVariant();
}

ExportFormat::List Generator::exportFormats() const
{
    return ExportFormat::List();
}

bool Generator::exportTo( const QString&, const ExportFormat& )
{
    return false;
}

void Generator::setDocument( Document *document )
{
    d->m_document = document;
}

bool Generator::hasFeature( GeneratorFeature ) const
{
    return false;
}


void Generator::signalRequestDone( PixmapRequest * request )
{
    if ( d->m_document )
        d->m_document->requestDone( request );
    else
        Q_ASSERT( !"No document set for generator in signalRequestDone!" );
}

Document * Generator::document() const
{
    return d->m_document;
}

class PixmapRequest::Private
{
    public:
        int mId;
        int mPageNumber;
        int mWidth;
        int mHeight;
        int mPriority;
        bool mAsynchronous;
        Page *mPage;
};


PixmapRequest::PixmapRequest( int id, int pageNumber, int width, int height, int priority, bool asynchronous )
  : d( new Private )
{
    d->mId = id;
    d->mPageNumber = pageNumber;
    d->mWidth = width;
    d->mHeight = height;
    d->mPriority = priority;
    d->mAsynchronous = asynchronous;
}

PixmapRequest::~PixmapRequest()
{
    delete d;
}

int PixmapRequest::id() const
{
    return d->mId;
}

int PixmapRequest::pageNumber() const
{
    return d->mPageNumber;
}

int PixmapRequest::width() const
{
    return d->mWidth;
}

int PixmapRequest::height() const
{
    return d->mHeight;
}

int PixmapRequest::priority() const
{
    return d->mPriority;
}

bool PixmapRequest::asynchronous() const
{
    return d->mAsynchronous;
}

Page* PixmapRequest::page() const
{
    return d->mPage;
}

void PixmapRequest::setPriority( int priority )
{
    d->mPriority = priority;
}

void PixmapRequest::setAsynchronous( bool asynchronous )
{
    d->mAsynchronous = asynchronous;
}

void PixmapRequest::setPage( Page *page )
{
    d->mPage = page;
}

void PixmapRequest::swap()
{
    qSwap( d->mWidth, d->mHeight );
}

class ExportFormat::Private
{
    public:
        Private( const QString &description, const KMimeType::Ptr &mimeType, const KIcon &icon = KIcon() )
            : mDescription( description ), mMimeType( mimeType ), mIcon( icon )
        {
        }

        QString mDescription;
        KMimeType::Ptr mMimeType;
        KIcon mIcon;
};

ExportFormat::ExportFormat()
    : d( new Private( QString(), KMimeType::Ptr() ) )
{
}

ExportFormat::ExportFormat( const QString &description, const KMimeType::Ptr &mimeType )
    : d( new Private( description, mimeType ) )
{
}

ExportFormat::ExportFormat( const KIcon &icon, const QString &description, const KMimeType::Ptr &mimeType )
    : d( new Private( description, mimeType, icon ) )
{
}

ExportFormat::~ExportFormat()
{
    delete d;
}

ExportFormat::ExportFormat( const ExportFormat &other )
    : d( new Private( QString(), KMimeType::Ptr() ) )
{
    *d = *other.d;
}

ExportFormat& ExportFormat::operator=( const ExportFormat &other )
{
    if ( this == &other )
        return *this;

    *d = *other.d;

    return *this;
}

QString ExportFormat::description() const
{
    return d->mDescription;
}

KMimeType::Ptr ExportFormat::mimeType() const
{
    return d->mMimeType;
}

KIcon ExportFormat::icon() const
{
    return d->mIcon;
}

kdbgstream& operator<<( kdbgstream &str, const Okular::PixmapRequest &req )
{
    QString s = QString( "%1 PixmapRequest (id: %2) (%3x%4), prio %5, pageNo %6" )
        .arg( QString( req.asynchronous() ? "Async" : "Sync" ) )
        .arg( req.id() )
        .arg( req.width() )
        .arg( req.height() )
        .arg( req.priority() )
        .arg( req.pageNumber() );
    str << s;
    return str;
}

#include "generator.moc"
