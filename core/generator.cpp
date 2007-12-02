/***************************************************************************
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator.h"
#include "generator_p.h"

#include <qeventloop.h>
#include <QtGui/QPrinter>

#include <kdebug.h>
#include <kicon.h>
#include <klocale.h>

#include "document.h"
#include "document_p.h"
#include "page.h"
#include "textpage.h"

using namespace Okular;

GeneratorPrivate::GeneratorPrivate()
    : m_document( 0 ),
      mPixmapGenerationThread( 0 ), mTextPageGenerationThread( 0 ),
      m_mutex( 0 ), m_threadsMutex( 0 ), mPixmapReady( true ), mTextPageReady( true ),
      m_closing( false ), m_closingLoop( 0 )
{
}

GeneratorPrivate::~GeneratorPrivate()
{
    if ( mPixmapGenerationThread )
        mPixmapGenerationThread->wait();

    delete mPixmapGenerationThread;

    if ( mTextPageGenerationThread )
        mTextPageGenerationThread->wait();

    delete mTextPageGenerationThread;

    delete m_mutex;
    delete m_threadsMutex;
}

PixmapGenerationThread* GeneratorPrivate::pixmapGenerationThread()
{
    if ( mPixmapGenerationThread )
        return mPixmapGenerationThread;

    Q_Q( Generator );
    mPixmapGenerationThread = new PixmapGenerationThread( q );
    QObject::connect( mPixmapGenerationThread, SIGNAL( finished() ),
                      q, SLOT( pixmapGenerationFinished() ),
                      Qt::QueuedConnection );

    return mPixmapGenerationThread;
}

TextPageGenerationThread* GeneratorPrivate::textPageGenerationThread()
{
    if ( mTextPageGenerationThread )
        return mTextPageGenerationThread;

    Q_Q( Generator );
    mTextPageGenerationThread = new TextPageGenerationThread( q );
    QObject::connect( mTextPageGenerationThread, SIGNAL( finished() ),
                      q, SLOT( textpageGenerationFinished() ),
                      Qt::QueuedConnection );

    return mTextPageGenerationThread;
}

void GeneratorPrivate::pixmapGenerationFinished()
{
    Q_Q( Generator );
    PixmapRequest *request = mPixmapGenerationThread->request();
    mPixmapGenerationThread->endGeneration();

    QMutexLocker locker( threadsLock() );
    mPixmapReady = true;

    if ( m_closing )
    {
        delete request;
        if ( mTextPageReady )
        {
            locker.unlock();
            m_closingLoop->quit();
        }
        return;
    }

    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( mPixmapGenerationThread->image() ) ) );

    q->signalPixmapRequestDone( request );
}

void GeneratorPrivate::textpageGenerationFinished()
{
    Page *page = mTextPageGenerationThread->page();
    mTextPageGenerationThread->endGeneration();

    QMutexLocker locker( threadsLock() );
    mTextPageReady = true;

    if ( m_closing )
    {
        delete mTextPageGenerationThread->textPage();
        if ( mPixmapReady )
        {
            locker.unlock();
            m_closingLoop->quit();
        }
        return;
    }

    if ( mTextPageGenerationThread->textPage() )
        page->setTextPage( mTextPageGenerationThread->textPage() );
}

QMutex* GeneratorPrivate::threadsLock()
{
    if ( !m_threadsMutex )
        m_threadsMutex = new QMutex();
    return m_threadsMutex;
}


Generator::Generator( QObject *parent, const QVariantList &args )
    : QObject( parent ), d_ptr( new GeneratorPrivate() )
{
    d_ptr->q_ptr = this;
    Q_UNUSED( args )
}

Generator::Generator( GeneratorPrivate &dd, QObject *parent, const QVariantList &args )
    : QObject( parent ), d_ptr( &dd )
{
    d_ptr->q_ptr = this;
    Q_UNUSED( args )
}

Generator::~Generator()
{
    delete d_ptr;
}

bool Generator::loadDocumentFromData( const QByteArray &, QVector< Page * > & )
{
    return false;
}

bool Generator::closeDocument()
{
    Q_D( Generator );

    d->m_closing = true;

    d->threadsLock()->lock();
    if ( !( d->mPixmapReady && d->mTextPageReady ) )
    {
        QEventLoop loop;
        d->m_closingLoop = &loop;

        d->threadsLock()->unlock();

        loop.exec();

        d->m_closingLoop = 0;
    }
    else
    {
        d->threadsLock()->unlock();
    }

    bool ret = doCloseDocument();

    d->m_closing = false;

    return ret;
}

bool Generator::canGeneratePixmap() const
{
    Q_D( const Generator );
    return d->mPixmapReady;
}

void Generator::generatePixmap( PixmapRequest *request )
{
    Q_D( Generator );
    d->mPixmapReady = false;

    if ( hasFeature( Threaded ) )
    {
        d->pixmapGenerationThread()->startGeneration( request );

        /**
         * We create the text page for every page that is visible to the
         * user, so he can use the text extraction tools without a delay.
         */
        if ( hasFeature( TextExtraction ) && !request->page()->hasTextPage() && canGenerateTextPage() ) {
            d->mTextPageReady = false;
            d->textPageGenerationThread()->startGeneration( request->page() );
        }

        return;
    }

    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( image( request ) ) ) );

    d->mPixmapReady = true;

    signalPixmapRequestDone( request );
}

bool Generator::canGenerateTextPage() const
{
    Q_D( const Generator );
    return d->mTextPageReady;
}

void Generator::generateTextPage( Page *page )
{
    Q_D( Generator );
    page->setTextPage( textPage( page ) );
    d->mTextPageReady = true;
}

QImage Generator::image( PixmapRequest * )
{
    return QImage();
}

TextPage* Generator::textPage( Page* )
{
    return 0;
}

const DocumentInfo * Generator::generateDocumentInfo()
{
    return 0;
}

const DocumentSynopsis * Generator::generateDocumentSynopsis()
{
    return 0;
}

FontInfo::List Generator::fontsForPage( int )
{
    return FontInfo::List();
}

const QList<EmbeddedFile*> * Generator::embeddedFiles() const
{
    return 0;
}

Generator::PageSizeMetric Generator::pagesSizeMetric() const
{
    return None;
}

bool Generator::isAllowed( Permission ) const
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

bool Generator::print( QPrinter& )
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

bool Generator::hasFeature( GeneratorFeature feature ) const
{
    Q_D( const Generator );
    return d->m_features.contains( feature );
}

void Generator::signalPixmapRequestDone( PixmapRequest * request )
{
    Q_D( Generator );
    if ( d->m_document )
        d->m_document->requestDone( request );
    else
        Q_ASSERT( !"No document set for generator in signalPixmapRequestDone!" );
}

const Document * Generator::document() const
{
    Q_D( const Generator );
    return d->m_document->m_parent;
}

void Generator::setFeature( GeneratorFeature feature, bool on )
{
    Q_D( Generator );
    if ( on )
        d->m_features.insert( feature );
    else
        d->m_features.remove( feature );
}

QVariant Generator::documentMetaData( const QString &key, const QVariant &option ) const
{
    Q_D( const Generator );
    if ( !d->m_document )
        return QVariant();

    return d->m_document->documentMetaData( key, option );
}

QMutex* Generator::userMutex() const
{
    Q_D( const Generator );
    if ( !d->m_mutex )
    {
        d->m_mutex = new QMutex();
    }
    return d->m_mutex;
}


PixmapRequest::PixmapRequest( int id, int pageNumber, int width, int height, int priority, bool asynchronous )
  : d( new PixmapRequestPrivate )
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

void PixmapRequest::swap()
{
    qSwap( d->mWidth, d->mHeight );
}

class Okular::ExportFormatPrivate : public QSharedData
{
    public:
        ExportFormatPrivate( const QString &description, const KMimeType::Ptr &mimeType, const KIcon &icon = KIcon() )
            : QSharedData(), mDescription( description ), mMimeType( mimeType ), mIcon( icon )
        {
        }
        ~ExportFormatPrivate()
        {
        }

        QString mDescription;
        KMimeType::Ptr mMimeType;
        KIcon mIcon;
};

ExportFormat::ExportFormat()
    : d( new ExportFormatPrivate( QString(), KMimeType::Ptr() ) )
{
}

ExportFormat::ExportFormat( const QString &description, const KMimeType::Ptr &mimeType )
    : d( new ExportFormatPrivate( description, mimeType ) )
{
}

ExportFormat::ExportFormat( const KIcon &icon, const QString &description, const KMimeType::Ptr &mimeType )
    : d( new ExportFormatPrivate( description, mimeType, icon ) )
{
}

ExportFormat::~ExportFormat()
{
}

ExportFormat::ExportFormat( const ExportFormat &other )
    : d( other.d )
{
}

ExportFormat& ExportFormat::operator=( const ExportFormat &other )
{
    if ( this == &other )
        return *this;

    d = other.d;

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

bool ExportFormat::isNull() const
{
    return d->mMimeType.isNull() || d->mDescription.isNull();
}

ExportFormat ExportFormat::standardFormat( StandardExportFormat type )
{
    switch ( type )
    {
        case PlainText:
            return ExportFormat( KIcon( "text" ), i18n( "Plain &Text..." ), KMimeType::mimeType( "text/plain" ) );
            break;
        case PDF:
            return ExportFormat( KIcon( "application-pdf" ), i18n( "PDF" ), KMimeType::mimeType( "application/pdf" ) );
            break;
    }
    return ExportFormat();
}

bool ExportFormat::operator==( const ExportFormat &other ) const
{
    return d == other.d;
}

bool ExportFormat::operator!=( const ExportFormat &other ) const
{
    return d != other.d;
}

QDebug operator<<( QDebug str, const Okular::PixmapRequest &req )
{
    QString s = QString( "%1 PixmapRequest (id: %2) (%3x%4), prio %5, pageNo %6" )
        .arg( QString( req.asynchronous() ? "Async" : "Sync" ) )
        .arg( req.id() )
        .arg( req.width() )
        .arg( req.height() )
        .arg( req.priority() )
        .arg( req.pageNumber() );
    str << qPrintable( s );
    return str;
}

#include "generator.moc"
