/***************************************************************************
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *   Copyright (C) 2008   by Albert Astals Cid <aacid@kde.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator.h"
#include "generator_p.h"
#include "observer.h"

#include <qeventloop.h>
#include <QtPrintSupport/QPrinter>

#include <QtCore/QDebug>
#include <QIcon>
#include <QMimeDatabase>
#include <KLocalizedString>
#include <kwallet.h>

#include "document.h"
#include "document_p.h"
#include "page.h"
#include "page_p.h"
#include "textpage.h"
#include "utils.h"

using namespace Okular;

GeneratorPrivate::GeneratorPrivate()
    : m_document( 0 ),
      mPixmapGenerationThread( 0 ), mTextPageGenerationThread( 0 ),
      m_mutex( 0 ), m_threadsMutex( 0 ), mPixmapReady( true ), mTextPageReady( true ),
      m_closing( false ), m_closingLoop( 0 ),
      m_dpi(72.0, 72.0)
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
    QObject::connect( mPixmapGenerationThread, SIGNAL(finished()), q, SLOT(pixmapGenerationFinished()),
                      Qt::QueuedConnection );

    return mPixmapGenerationThread;
}

TextPageGenerationThread* GeneratorPrivate::textPageGenerationThread()
{
    if ( mTextPageGenerationThread )
        return mTextPageGenerationThread;

    Q_Q( Generator );
    mTextPageGenerationThread = new TextPageGenerationThread( q );
    QObject::connect( mTextPageGenerationThread, SIGNAL(finished()), q, SLOT(textpageGenerationFinished()),
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

    const QImage& img = mPixmapGenerationThread->image();
    request->page()->setPixmap( request->observer(), new QPixmap( QPixmap::fromImage( img ) ), request->normalizedRect() );
    const int pageNumber = request->page()->number();

    if ( mPixmapGenerationThread->calcBoundingBox() )
        q->updatePageBoundingBox( pageNumber, mPixmapGenerationThread->boundingBox() );
    q->signalPixmapRequestDone( request );
}

void GeneratorPrivate::textpageGenerationFinished()
{
    Q_Q( Generator );
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
    {
        TextPage *tp = mTextPageGenerationThread->textPage();
        page->setTextPage( tp );
        q->signalTextGenerationDone( page, tp );
    }
}

QMutex* GeneratorPrivate::threadsLock()
{
    if ( !m_threadsMutex )
        m_threadsMutex = new QMutex();
    return m_threadsMutex;
}

QVariant GeneratorPrivate::metaData( const QString &, const QVariant & ) const
{
    return QVariant();
}

QImage GeneratorPrivate::image( PixmapRequest * )
{
    return QImage();
}


Generator::Generator(QObject* parent, const QVariantList&)
    : QObject(parent)
    , d_ptr( new GeneratorPrivate() )
{
    d_ptr->q_ptr = this;
}

Generator::Generator(GeneratorPrivate &dd, QObject *parent, const QVariantList &args)
    : QObject(parent), d_ptr(&dd)
{
    d_ptr->q_ptr = this;
    Q_UNUSED(args)
}

Generator::~Generator()
{
    delete d_ptr;
}

bool Generator::loadDocument( const QString & fileName, QVector< Page * > & pagesVector )
{
    Q_UNUSED(fileName);
    Q_UNUSED(pagesVector);

    return false;
}

bool Generator::loadDocumentFromData( const QByteArray &, QVector< Page * > & )
{
    return false;
}

Document::OpenResult Generator::loadDocumentWithPassword( const QString & fileName, QVector< Page * > & pagesVector, const QString & )
{
    return loadDocument( fileName, pagesVector ) ? Document::OpenSuccess : Document::OpenError;
}

Document::OpenResult Generator::loadDocumentFromDataWithPassword( const QByteArray & fileData, QVector< Page * > & pagesVector, const QString & )
{
    return loadDocumentFromData( fileData, pagesVector ) ? Document::OpenSuccess : Document::OpenError;
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

    const bool calcBoundingBox = !request->isTile() && !request->page()->isBoundingBoxKnown();

    if ( request->asynchronous() && hasFeature( Threaded ) )
    {
        d->pixmapGenerationThread()->startGeneration( request, calcBoundingBox );

        /**
         * We create the text page for every page that is visible to the
         * user, so he can use the text extraction tools without a delay.
         */
        if ( hasFeature( TextExtraction ) && !request->page()->hasTextPage() && canGenerateTextPage() && !d->m_closing ) {
            d->mTextPageReady = false;
            d->textPageGenerationThread()->startGeneration( request->page() );
        }

        return;
    }

    const QImage& img = image( request );
    request->page()->setPixmap( request->observer(), new QPixmap( QPixmap::fromImage( img ) ), request->normalizedRect() );
    const int pageNumber = request->page()->number();

    d->mPixmapReady = true;

    signalPixmapRequestDone( request );
    if ( calcBoundingBox )
        updatePageBoundingBox( pageNumber, Utils::imageBoundingBox( &img ) );
}

bool Generator::canGenerateTextPage() const
{
    Q_D( const Generator );
    return d->mTextPageReady;
}

void Generator::generateTextPage( Page *page )
{
    TextPage *tp = textPage( page );
    page->setTextPage( tp );
    signalTextGenerationDone( page, tp );
}

QImage Generator::image( PixmapRequest *request )
{
    Q_D( Generator );
    return d->image( request );
}

TextPage* Generator::textPage( Page* )
{
    return 0;
}

DocumentInfo Generator::generateDocumentInfo(const QSet<DocumentInfo::Key> &keys) const
{
    Q_UNUSED(keys);

    return DocumentInfo();
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

Generator::PrintError Generator::printError() const
{
    return UnknownPrintError;
}

QVariant Generator::metaData( const QString &key, const QVariant &option ) const
{
    Q_D( const Generator );
    return d->metaData( key, option );
}

ExportFormat::List Generator::exportFormats() const
{
    return ExportFormat::List();
}

bool Generator::exportTo( const QString&, const ExportFormat& )
{
    return false;
}

void Generator::walletDataForFile( const QString &fileName, QString *walletName, QString *walletFolder, QString *walletKey ) const
{
    *walletKey = fileName.section( QLatin1Char('/'), -1, -1);
    *walletName = KWallet::Wallet::NetworkWallet();
    *walletFolder = QStringLiteral("KPdf");
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
    {
        delete request;
    }
}

void Generator::signalTextGenerationDone( Page *page, TextPage *textPage )
{
    Q_D( Generator );
    if ( d->m_document )
        d->m_document->textGenerationDone( page );
    else
        delete textPage;
}

const Document * Generator::document() const
{
    Q_D( const Generator );
    if ( d->m_document )
    {
        return d->m_document->m_parent;
    }
    return 0;
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

void Generator::updatePageBoundingBox( int page, const NormalizedRect & boundingBox )
{
    Q_D( Generator );
    if ( d->m_document ) // still connected to document?
        d->m_document->setPageBoundingBox( page, boundingBox );
}

void Generator::requestFontData(const Okular::FontInfo & /*font*/, QByteArray * /*data*/)
{

}

void Generator::setDPI(const QSizeF & dpi)
{
     Q_D( Generator );
     d->m_dpi = dpi;
}

QSizeF Generator::dpi() const
{
     Q_D( const Generator );
     return d->m_dpi;
}

QAbstractItemModel * Generator::layersModel() const
{
    return 0;
}

PixmapRequest::PixmapRequest( DocumentObserver *observer, int pageNumber, int width, int height, int priority, PixmapRequestFeatures features )
  : d( new PixmapRequestPrivate )
{
    d->mObserver = observer;
    d->mPageNumber = pageNumber;
    d->mWidth = width;
    d->mHeight = height;
    d->mPriority = priority;
    d->mFeatures = features;
    d->mForce = false;
    d->mTile = false;
    d->mNormalizedRect = NormalizedRect();
}

PixmapRequest::~PixmapRequest()
{
    delete d;
}

DocumentObserver *PixmapRequest::observer() const
{
    return d->mObserver;
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
    return d->mFeatures & Asynchronous;
}

bool PixmapRequest::preload() const
{
    return d->mFeatures & Preload;
}

Page* PixmapRequest::page() const
{
    return d->mPage;
}

void PixmapRequest::setTile( bool tile )
{
    d->mTile = tile;
}

bool PixmapRequest::isTile() const
{
    return d->mTile;
}

void PixmapRequest::setNormalizedRect( const NormalizedRect &rect )
{
    if ( d->mNormalizedRect == rect )
        return;

    d->mNormalizedRect = rect;
}

const NormalizedRect& PixmapRequest::normalizedRect() const
{
    return d->mNormalizedRect;
}

Okular::TilesManager* PixmapRequestPrivate::tilesManager() const
{
    return mPage->d->tilesManager(mObserver);
}

void PixmapRequestPrivate::swap()
{
    qSwap( mWidth, mHeight );
}

class Okular::ExportFormatPrivate : public QSharedData
{
    public:
        ExportFormatPrivate( const QString &description, const QMimeType &mimeType, const QIcon &icon = QIcon() )
            : QSharedData(), mDescription( description ), mMimeType( mimeType ), mIcon( icon )
        {
        }
        ~ExportFormatPrivate()
        {
        }

        QString mDescription;
        QMimeType mMimeType;
        QIcon mIcon;
};

ExportFormat::ExportFormat()
    : d( new ExportFormatPrivate( QString(), QMimeType() ) )
{
}

ExportFormat::ExportFormat( const QString &description, const QMimeType &mimeType )
    : d( new ExportFormatPrivate( description, mimeType ) )
{
}

ExportFormat::ExportFormat( const QIcon &icon, const QString &description, const QMimeType &mimeType )
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

QMimeType ExportFormat::mimeType() const
{
    return d->mMimeType;
}

QIcon ExportFormat::icon() const
{
    return d->mIcon;
}

bool ExportFormat::isNull() const
{
    return !d->mMimeType.isValid() || d->mDescription.isNull();
}

ExportFormat ExportFormat::standardFormat( StandardExportFormat type )
{
    QMimeDatabase db;
    switch ( type )
    {
        case PlainText:
            return ExportFormat( QIcon::fromTheme( QStringLiteral("text-x-generic") ), i18n( "Plain &Text..." ), db.mimeTypeForName( QStringLiteral("text/plain") ) );
            break;
        case PDF:
            return ExportFormat( QIcon::fromTheme( QStringLiteral("application-pdf") ), i18n( "PDF" ), db.mimeTypeForName( QStringLiteral("application/pdf") ) );
            break;
        case OpenDocumentText:
            return ExportFormat(
                QIcon::fromTheme( QStringLiteral("application-vnd.oasis.opendocument.text") ),
                i18nc( "This is the document format", "OpenDocument Text" ),
                db.mimeTypeForName( QStringLiteral("application/vnd.oasis.opendocument.text") ) );
	    break;
        case HTML:
            return ExportFormat( QIcon::fromTheme( QStringLiteral("text-html") ), i18nc( "This is the document format", "HTML" ), db.mimeTypeForName( QStringLiteral("text/html") ) );
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
    QString s = QStringLiteral( "PixmapRequest(#%2, %1, %3x%4, page %6, prio %5)" )
        .arg( QString( req.asynchronous() ? QStringLiteral ( "async" ) : QStringLiteral ( "sync" ) ) )
        .arg( (qulonglong)req.observer() )
        .arg( req.width() )
        .arg( req.height() )
        .arg( req.priority() )
        .arg( req.pageNumber() );
    str << qPrintable( s );
    return str;
}

#include "moc_generator.cpp"

/* kate: replace-tabs on; indent-width 4; */
