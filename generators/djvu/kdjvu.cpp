/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kdjvu.h"

#include <qdom.h>
#include <qfile.h>
#include <qlist.h>
#include <qmap.h>
#include <qpainter.h>
#include <qstring.h>

#include <kdebug.h>
#include <klocale.h>

#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>

kdbgstream &operator<<( kdbgstream & s, const ddjvu_rect_t &r )
{
    s << "[" << r.x << "," << r.y << " - " << r.w << "x" << r.h << "]";
    return s;
}

static void which_ddjvu_message( const ddjvu_message_t *msg )
{
    kDebug() << "which_djvu_message(...): " << msg->m_any.tag << endl;
    switch( msg->m_any.tag )
    {
        case DDJVU_ERROR:
            kDebug() << "ERROR: file " << msg->m_error.filename << ", line " << msg->m_error.lineno << endl;
            kDebug() << "ERROR: function '" << msg->m_error.function << "'" << endl;
            kDebug() << "ERROR: '" << msg->m_error.message << "'" << endl;
            break;
        case DDJVU_INFO:
            kDebug() << "INFO: '" << msg->m_info.message << "'" << endl;
            break;
        case DDJVU_CHUNK:
            kDebug() << "CHUNK: '" << QByteArray( msg->m_chunk.chunkid ) << "'" << endl;
            break;
        default: ;
    }
}

/**
 * Explore the message queue until there are message left in it.
 */
static void handle_ddjvu_messages( ddjvu_context_t *ctx, int wait )
{
    const ddjvu_message_t *msg;
    if ( wait )
        ddjvu_message_wait( ctx );
    while ( ( msg = ddjvu_message_peek( ctx ) ) )
    {
        which_ddjvu_message( msg );
        ddjvu_message_pop( ctx );
    }
}

/**
 * Explore the message queue until the message \p mid is found.
 */
static void wait_for_ddjvu_message( ddjvu_context_t *ctx, ddjvu_message_tag_t mid )
{
    ddjvu_message_wait( ctx );
    const ddjvu_message_t *msg;
    while ( ( msg = ddjvu_message_peek( ctx ) ) && msg && ( msg->m_any.tag != mid ) )
    {
        which_ddjvu_message( msg );
        ddjvu_message_pop( ctx );
    }
}

/**
 * Convert a clockwise coefficient \p r for a rotation to a counter-clockwise
 * and vice versa.
 */
static int flipRotation( int r )
{
    return ( 4 - r ) % 4;
}


// PixmapCacheItem

class PixmapCacheItem
{
    public:
        PixmapCacheItem( int p, int w, int h, int r, const QPixmap& px )
          : page( p ), width( w ), height( h ), rotation( r ), pix( px ) { }

        int page;
        int width;
        int height;
        int rotation;
        QPixmap pix;
};


// KdjVu::Page

KDjVu::Page::Page()
{
}

KDjVu::Page::~Page()
{
}

int KDjVu::Page::width() const
{
    return m_width;
}

int KDjVu::Page::height() const
{
    return m_height;
}

int KDjVu::Page::dpi() const
{
    return m_dpi;
}

int KDjVu::Page::orientation() const
{
    return m_orientation;
}

class KDjVu::Private
{
    public:
        Private()
          : m_djvu_cxt( 0 ), m_djvu_document( 0 ), m_format( 0 ), m_docBookmarks( 0 )
        {
        }

        QPixmap generatePixmapTile( ddjvu_page_t *djvupage, int& res,
            int width, int row, int xdelta, int height, int col, int ydelta );

        void readBookmarks();
        void fillBookmarksRecurse( QDomDocument& maindoc, QDomNode& curnode,
            miniexp_t exp, int offset = -1 );

        ddjvu_context_t *m_djvu_cxt;
        ddjvu_document_t *m_djvu_document;
        ddjvu_format_t *m_format;

        QVector<KDjVu::Page*> m_pages;
        QVector<ddjvu_page_t *> m_pages_cache;

        QList<PixmapCacheItem*> mPixCache;

        QMap<QString, QString> m_metaData;
        QDomDocument * m_docBookmarks;
};

QPixmap KDjVu::Private::generatePixmapTile( ddjvu_page_t *djvupage, int& res,
    int width, int row, int xdelta, int height, int col, int ydelta )
{
    ddjvu_rect_t renderrect;
    renderrect.x = row * xdelta;
    renderrect.y = col * ydelta;
    int realwidth = qMin( width - renderrect.x, xdelta );
    int realheight = qMin( height - renderrect.y, ydelta );
    renderrect.w = realwidth;
    renderrect.h = realheight;
    kDebug() << "renderrect: " << renderrect << endl;
    ddjvu_rect_t pagerect;
    pagerect.x = 0;
    pagerect.y = 0;
    pagerect.w = width;
    pagerect.h = height;
    kDebug() << "pagerect: " << pagerect << endl;
    handle_ddjvu_messages( m_djvu_cxt, false );
    char* imagebuffer = new char[ realwidth * realheight * 4 + 1 ];
    res = ddjvu_page_render( djvupage, DDJVU_RENDER_COLOR,
                  &pagerect, &renderrect, m_format, realwidth * 4, imagebuffer );
    kDebug() << "rendering result: " << res << endl;
    handle_ddjvu_messages( m_djvu_cxt, false );
    QPixmap pix;
    if ( res )
    {
        QImage img( (uchar*)imagebuffer, realwidth, realheight, QImage::Format_RGB32 );
        pix = QPixmap::fromImage( img );
    }
    delete [] imagebuffer;

    return pix;
}

void KDjVu::Private::readBookmarks()
{
    if ( !m_djvu_document )
        return;

    miniexp_t outline;
    while ( ( outline = ddjvu_document_get_outline( m_djvu_document ) ) == miniexp_dummy )
        handle_ddjvu_messages( m_djvu_cxt, true );

    if ( miniexp_listp( outline ) &&
         ( miniexp_length( outline ) > 0 ) &&
         miniexp_symbolp( miniexp_nth( 0, outline ) ) &&
         ( QString::fromUtf8( miniexp_to_name( miniexp_nth( 0, outline ) ) ) == QLatin1String( "bookmarks" ) ) )
    {
        m_docBookmarks = new QDomDocument( "KDjVuBookmarks" );
        fillBookmarksRecurse( *m_docBookmarks, *m_docBookmarks, outline, 1 );
    }
}

void KDjVu::Private::fillBookmarksRecurse( QDomDocument& maindoc, QDomNode& curnode,
    miniexp_t exp, int offset )
{
    if ( !miniexp_listp( exp ) )
        return;

    int l = miniexp_length( exp );
    for ( int i = qMax( offset, 0 ); i < l; ++i )
    {
        miniexp_t cur = miniexp_nth( i, exp );

        if ( miniexp_consp( cur ) && ( miniexp_length( cur ) > 0 ) &&
             miniexp_stringp( miniexp_nth( 0, cur ) ) && miniexp_stringp( miniexp_nth( 1, cur ) ) )
        {
            QString title = QString::fromUtf8( miniexp_to_str( miniexp_nth( 0, cur ) ) );
            QString dest = QString::fromUtf8( miniexp_to_str( miniexp_nth( 1, cur ) ) );
            QDomElement el;
            if ( dest.isEmpty() || ( ( dest.at( 0 ) == QLatin1Char( '#' ) ) && ( dest.remove( 0, 1 ) != title ) ) )
            {
                el = maindoc.createElement( "item" );
                el.setAttribute( "title", title );
                el.setAttribute( "destination", dest );
                curnode.appendChild( el );
            }
            if ( !el.isNull() && ( miniexp_length( cur ) > 2 ) )
            {
                fillBookmarksRecurse( maindoc, el, cur, 2 );
            }
        }
    }
}


KDjVu::KDjVu() : QObject(), d( new Private )
{
    // creating the djvu context
    d->m_djvu_cxt = ddjvu_context_create( "KDjVu" );
    // creating the rendering format
    unsigned int* mask = new unsigned int[3];
    mask[0] = 0x00ff0000;
    mask[1] = 0x0000ff00;
    mask[2] = 0x000000ff;
    d->m_format = ddjvu_format_create( DDJVU_FORMAT_RGBMASK32, 3, mask );
    ddjvu_format_set_row_order( d->m_format, 1 );
    ddjvu_format_set_y_direction( d->m_format, 1 );
}


KDjVu::~KDjVu()
{
    closeFile();

    ddjvu_format_release( d->m_format );
    ddjvu_context_release( d->m_djvu_cxt );

    delete d;
}

bool KDjVu::openFile( const QString & fileName )
{
    // first, close the old file
    if ( d->m_djvu_document )
        closeFile();

    // load the document...
    d->m_djvu_document = ddjvu_document_create_by_filename( d->m_djvu_cxt, QFile::encodeName( fileName ), true );
    if ( !d->m_djvu_document ) return false;
    // ...and wait for its loading
    wait_for_ddjvu_message( d->m_djvu_cxt, DDJVU_DOCINFO );

    kDebug() << "# of pages: " << ddjvu_document_get_pagenum( d->m_djvu_document ) << endl;
    int numofpages = ddjvu_document_get_pagenum( d->m_djvu_document );
    d->m_pages.clear();
    d->m_pages.resize( numofpages );
    d->m_pages_cache.clear();
    d->m_pages_cache.resize( numofpages );

    // get the document type
    QString doctype;
    switch ( ddjvu_document_get_type( d->m_djvu_document ) )
    {
        case DDJVU_DOCTYPE_UNKNOWN:
            doctype = i18nc( "Type of DjVu document", "Unknown" );
            break;
        case DDJVU_DOCTYPE_SINGLEPAGE:
            doctype = i18nc( "Type of DjVu document", "Single Page" );
            break;
        case DDJVU_DOCTYPE_BUNDLED:
            doctype = i18nc( "Type of DjVu document", "Bundled" );
            break;
        case DDJVU_DOCTYPE_INDIRECT:
            doctype = i18nc( "Type of DjVu document", "Indirect" );
            break;
        case DDJVU_DOCTYPE_OLD_BUNDLED:
            doctype = i18nc( "Type of DjVu document", "Bundled (old)" );
            break;
        case DDJVU_DOCTYPE_OLD_INDEXED:
            doctype = i18nc( "Type of DjVu document", "Indexed (old)" );
            break;
    }
    if ( !doctype.isEmpty() )
        d->m_metaData[ "documentType" ] = doctype;
    // get the number of components
    d->m_metaData[ "componentFile" ] = QString::number( ddjvu_document_get_filenum( d->m_djvu_document ) );

    // read the pages
    for ( int i = 0; i < numofpages; ++i )
    {
        // wait for the new page to be loaded
        ddjvu_page_t *newpage = ddjvu_page_create_by_pageno( d->m_djvu_document, i );
        ddjvu_status_t sts;
        while ( ( sts = ddjvu_page_decoding_status( newpage ) ) < DDJVU_JOB_OK )
            handle_ddjvu_messages( d->m_djvu_cxt, true );
        if ( sts >= DDJVU_JOB_FAILED )
        {
            kDebug() << "\t>>> page " << i << " failed: " << sts << endl;
            break;
        }
        d->m_pages_cache[i] = newpage;

        KDjVu::Page *p = new KDjVu::Page();
        p->m_width = ddjvu_page_get_width( newpage );
        p->m_height = ddjvu_page_get_height( newpage );
        p->m_dpi = ddjvu_page_get_resolution( newpage );
        p->m_orientation = flipRotation( ddjvu_page_get_initial_rotation( newpage ) );
        d->m_pages[i] = p;
    }

    return true;
}

void KDjVu::closeFile()
{
    // deleting the old TOC
    delete d->m_docBookmarks;
    d->m_docBookmarks = 0;
    // deleting the pages
    qDeleteAll( d->m_pages );
    // releasing the djvu pages
    QVector<ddjvu_page_t *>::Iterator it = d->m_pages_cache.begin(), itEnd = d->m_pages_cache.end();
    for ( ; it != itEnd; ++it )
        ddjvu_page_release( *it );
    d->m_pages_cache.clear();
    // clearing the pixmap cache
    qDeleteAll( d->mPixCache );
    // clearing the old metadata
    d->m_metaData.clear();
    // releasing the old document
    if ( d->m_djvu_document )
        ddjvu_document_release( d->m_djvu_document );
    d->m_djvu_document = 0;
}

QString KDjVu::getMetaData( const QString & key ) const
{
    return d->m_metaData.contains( key ) ? d->m_metaData[ key ] : QString();
}

const QDomDocument * KDjVu::documentBookmarks() const
{
    if ( !d->m_docBookmarks )
        d->readBookmarks();
    return d->m_docBookmarks;
}

const QVector<KDjVu::Page*> &KDjVu::pages() const
{
    return d->m_pages;
}

QPixmap KDjVu::pixmap( int page, int width, int height, int rotation )
{
    bool found = false;
    QList<PixmapCacheItem*>::Iterator it = d->mPixCache.begin(), itEnd = d->mPixCache.end();
    for ( ; ( it != itEnd ) && !found; ++it )
    {
        PixmapCacheItem* cur = *it;
        if ( ( cur->page == page ) &&
             ( cur->width == width ) &&
             ( cur->height == height ) &&
             ( cur->rotation == rotation ) )
            found = true;
    }
    if ( !found )
        return QPixmap();

    // taking the element and pushing to the top of the list
    --it;
    PixmapCacheItem* cur2 = *it;
    d->mPixCache.erase( it );
    d->mPixCache.push_front( cur2 );

    return cur2->pix;
}

void KDjVu::requestPixmap( int page, int width, int height, int rotation )
{
    QPixmap tmp = pixmap( page, width, height, rotation );
    if ( !tmp.isNull() )
    {
        emit pixmapGenerated( page, tmp );
        return;
    }

    if ( !d->m_pages_cache.at( page ) )
    {
        ddjvu_page_t *newpage = ddjvu_page_create_by_pageno( d->m_djvu_document, page );
        // wait for the new page to be loaded
        ddjvu_status_t sts;
        while ( ( sts = ddjvu_page_decoding_status( newpage ) ) < DDJVU_JOB_OK )
            handle_ddjvu_messages( d->m_djvu_cxt, true );
        d->m_pages_cache[page] = newpage;
    }
    ddjvu_page_t *djvupage = d->m_pages_cache[page];

    if ( ddjvu_page_get_rotation( djvupage ) != flipRotation( rotation ) )
    {
// TODO: test documents with initial rotation != 0
//        ddjvu_page_set_rotation( djvupage, m_pages.at( page )->orientation() );
        ddjvu_page_set_rotation( djvupage, (ddjvu_page_rotation_t)flipRotation( rotation ) );
    }

    static const int xdelta = 1500;
    static const int ydelta = 1500;

    int xparts = width / xdelta + 1;
    int yparts = height / ydelta + 1;

    QPixmap newpix( width, height );

    int res = 10000;
    if ( ( xparts == 1 ) && ( yparts == 1 ) )
    {
         // only one part -- render at once with no need to auxiliary pixmap
         newpix = d->generatePixmapTile( djvupage, res,
                 width, 0, xdelta, height, 0, ydelta );
    }
    else
    {
        // more than one part -- need to render piece-by-piece and to compose
        // the results
        QPainter p;
        p.begin( &newpix );
        int parts = xparts * yparts;
        for ( int i = 0; i < parts; ++i )
        {
            int row = i % xparts;
            int col = i / xparts;
            int tmpres = 0;
            QPixmap tempp = d->generatePixmapTile( djvupage, tmpres,
                    width, row, xdelta, height, col, ydelta );
            if ( tmpres )
            {
                p.drawPixmap( row * xdelta, col * ydelta, tempp );
            }
            res = qMin( tmpres, res );
        }
        p.end();
    }

    QPixmap pix;

    if ( res )
    {
        pix = newpix;

        // delete all the cached pixmaps for the current page with a size that
        // differs no more than 35% of the new pixmap size
        int pixsize = pix.width() * pix.height();
        if ( pixsize > 0 )
        {
            for( int i = 0; i < d->mPixCache.count(); )
            {
                PixmapCacheItem* cur = d->mPixCache.at(i);
                if ( ( cur->page == page ) &&
                     ( cur->rotation == rotation ) &&
                     ( abs( cur->pix.width() * cur->pix.height() - pixsize ) < pixsize * 0.35 ) )
                {
                    d->mPixCache.removeAt( i );
                    delete cur;
                }
                else
                    ++i;
            }
        }

        // the pixmap cache has too many elements, remove the last
        if ( d->mPixCache.size() >= 10 )
        {
            delete d->mPixCache.last();
            d->mPixCache.removeLast();
        }
        PixmapCacheItem* pch = new PixmapCacheItem( page, width, height, rotation, pix );
        d->mPixCache.push_front( pch );
    }

    emit pixmapGenerated( page, pix );
}


#include "kdjvu.moc"
