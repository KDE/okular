/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kdjvu.h"

#include <qbytearray.h>
#include <qdom.h>
#include <qfile.h>
#include <qhash.h>
#include <qlist.h>
#include <qpainter.h>
#include <qstring.h>

#include <kdebug.h>
#include <klocale.h>

#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>

#include <stdio.h>

kdbgstream &operator<<( kdbgstream & s, const ddjvu_rect_t &r )
{
    s << "[" << r.x << "," << r.y << " - " << r.w << "x" << r.h << "]";
    return s;
}

static void which_ddjvu_message( const ddjvu_message_t *msg )
{
#ifdef KDJVU_DEBUG
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
        case DDJVU_PROGRESS:
            kDebug() << "PROGRESS: '" << msg->m_progress.percent << "'" << endl;
            break;
        default: ;
    }
#else
    Q_UNUSED( msg );
#endif
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


// ImageCacheItem

class ImageCacheItem
{
    public:
        ImageCacheItem( int p, int w, int h, const QImage& i )
          : page( p ), width( w ), height( h ), img( i ) { }

        int page;
        int width;
        int height;
        QImage img;
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

// KDjVu::Link

KDjVu::Link::~Link()
{
}

KDjVu::Link::LinkArea KDjVu::Link::areaType() const
{
    return m_area;
}

QPoint KDjVu::Link::point() const
{
    return m_point;
}

QSize KDjVu::Link::size() const
{
    return m_size;
}

QPolygon KDjVu::Link::polygon() const
{
    return m_poly;
}

// KDjVu::PageLink

KDjVu::PageLink::PageLink()
{
}

int KDjVu::PageLink::type() const
{
    return KDjVu::Link::PageLink;
}

QString KDjVu::PageLink::page() const
{
    return m_page;
}

// KDjVu::UrlLink

KDjVu::UrlLink::UrlLink()
{
}

int KDjVu::UrlLink::type() const
{
    return KDjVu::Link::UrlLink;
}

QString KDjVu::UrlLink::url() const
{
    return m_url;
}

// KDjVu::Annotation

KDjVu::Annotation::~Annotation()
{
}

QPoint KDjVu::Annotation::point() const
{
    return m_point;
}

QString KDjVu::Annotation::comment() const
{
    return m_comment;
}

QColor KDjVu::Annotation::color() const
{
    return m_color;
}

// KDjVu::TextAnnotation

KDjVu::TextAnnotation::TextAnnotation()
  : m_inlineText( true )
{
}

QSize KDjVu::TextAnnotation::size() const
{
    return m_size;
}

int KDjVu::TextAnnotation::type() const
{
    return KDjVu::Annotation::TextAnnotation;
}

bool KDjVu::TextAnnotation::inlineText() const
{
    return m_inlineText;
}

// KDjVu::LineAnnotation

KDjVu::LineAnnotation::LineAnnotation()
  : m_isArrow( false ), m_width( 1 )
{
}

int KDjVu::LineAnnotation::type() const
{
    return KDjVu::Annotation::LineAnnotation;
}

QPoint KDjVu::LineAnnotation::point2() const
{
    return m_point2;
}

bool KDjVu::LineAnnotation::isArrow() const
{
    return m_isArrow;
}

int KDjVu::LineAnnotation::width() const
{
    return m_width;
}



class KDjVu::Private
{
    public:
        Private()
          : m_djvu_cxt( 0 ), m_djvu_document( 0 ), m_format( 0 ), m_docBookmarks( 0 )
        {
        }

        QImage generateImageTile( ddjvu_page_t *djvupage, int& res,
            int width, int row, int xdelta, int height, int col, int ydelta );

        void readBookmarks();
        void fillBookmarksRecurse( QDomDocument& maindoc, QDomNode& curnode,
            miniexp_t exp, int offset = -1 );

        ddjvu_context_t *m_djvu_cxt;
        ddjvu_document_t *m_djvu_document;
        ddjvu_format_t *m_format;
        unsigned int* m_formatmask;

        QVector<KDjVu::Page*> m_pages;
        QVector<ddjvu_page_t *> m_pages_cache;

        QList<ImageCacheItem*> mImgCache;

        QHash<QString, QVariant> m_metaData;
        QDomDocument * m_docBookmarks;
};

QImage KDjVu::Private::generateImageTile( ddjvu_page_t *djvupage, int& res,
    int width, int row, int xdelta, int height, int col, int ydelta )
{
    ddjvu_rect_t renderrect;
    renderrect.x = row * xdelta;
    renderrect.y = col * ydelta;
    int realwidth = qMin( width - renderrect.x, xdelta );
    int realheight = qMin( height - renderrect.y, ydelta );
    renderrect.w = realwidth;
    renderrect.h = realheight;
#ifdef KDJVU_DEBUG
    kDebug() << "renderrect: " << renderrect << endl;
#endif
    ddjvu_rect_t pagerect;
    pagerect.x = 0;
    pagerect.y = 0;
    pagerect.w = width;
    pagerect.h = height;
#ifdef KDJVU_DEBUG
    kDebug() << "pagerect: " << pagerect << endl;
#endif
    handle_ddjvu_messages( m_djvu_cxt, false );
    char* imagebuffer = new char[ realwidth * realheight * 4 + 1 ];
    res = ddjvu_page_render( djvupage, DDJVU_RENDER_COLOR,
                  &pagerect, &renderrect, m_format, realwidth * 4, imagebuffer );
#ifdef KDJVU_DEBUG
    kDebug() << "rendering result: " << res << endl;
#endif
    handle_ddjvu_messages( m_djvu_cxt, false );
    QImage res_img;
    if ( res )
    {
        QImage img( (uchar*)imagebuffer, realwidth, realheight, QImage::Format_RGB32 );
        res_img = img.copy();
    }
    delete [] imagebuffer;

    return res_img;
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
    d->m_formatmask = new unsigned int[3];
    d->m_formatmask[0] = 0x00ff0000;
    d->m_formatmask[1] = 0x0000ff00;
    d->m_formatmask[2] = 0x000000ff;
    d->m_format = ddjvu_format_create( DDJVU_FORMAT_RGBMASK32, 3, d->m_formatmask );
    ddjvu_format_set_row_order( d->m_format, 1 );
    ddjvu_format_set_y_direction( d->m_format, 1 );
}


KDjVu::~KDjVu()
{
    closeFile();

    ddjvu_format_release( d->m_format );
    delete [] d->m_formatmask;
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
    d->m_metaData[ "componentFile" ] = ddjvu_document_get_filenum( d->m_djvu_document );

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
    d->m_pages.clear();
    // releasing the djvu pages
    QVector<ddjvu_page_t *>::Iterator it = d->m_pages_cache.begin(), itEnd = d->m_pages_cache.end();
    for ( ; it != itEnd; ++it )
        ddjvu_page_release( *it );
    d->m_pages_cache.clear();
    // clearing the image cache
    qDeleteAll( d->mImgCache );
    d->mImgCache.clear();
    // clearing the old metadata
    d->m_metaData.clear();
    // releasing the old document
    if ( d->m_djvu_document )
        ddjvu_document_release( d->m_djvu_document );
    d->m_djvu_document = 0;
}

QVariant KDjVu::metaData( const QString & key ) const
{
    return d->m_metaData.contains( key ) ? d->m_metaData[ key ] : QVariant();
}

const QDomDocument * KDjVu::documentBookmarks() const
{
    if ( !d->m_docBookmarks )
        d->readBookmarks();
    return d->m_docBookmarks;
}

void KDjVu::linksAndAnnotationsForPage( int pageNum, QList<KDjVu::Link*> *links, QList<KDjVu::Annotation*> *annotations ) const
{
    if ( ( pageNum < 0 ) || ( pageNum >= d->m_pages.count() ) || ( !links && !annotations ) )
        return;

    miniexp_t annots;
    while ( ( annots = ddjvu_document_get_pageanno( d->m_djvu_document, pageNum ) ) == miniexp_dummy )
        handle_ddjvu_messages( d->m_djvu_cxt, true );

    if ( !miniexp_listp( annots ) )
        return;

    if ( links )
        links->clear();
    if ( annotations )
        annotations->clear();

    int l = miniexp_length( annots );
    for ( int i = 0; i < l; ++i )
    {
        miniexp_t cur = miniexp_nth( i, annots );
        int num = miniexp_length( cur );
        if ( ( num < 4 ) || !miniexp_symbolp( miniexp_nth( 0, cur ) ) ||
             ( qstrncmp( miniexp_to_name( miniexp_nth( 0, cur ) ), "maparea", 7 ) != 0 ) )
            continue;

        QString target;
        QString type;
        if ( miniexp_symbolp( miniexp_nth( 0, miniexp_nth( 3, cur ) ) ) )
            type = QString::fromUtf8( miniexp_to_name( miniexp_nth( 0, miniexp_nth( 3, cur ) ) ) );
        KDjVu::Link* link = 0;
        KDjVu::Annotation* ann = 0;
        miniexp_t urlexp = miniexp_nth( 1, cur );
        if ( links &&
             ( type == QLatin1String( "rect" ) ||
               type == QLatin1String( "oval" ) ||
               type == QLatin1String( "poly" ) ) )
        {
            if ( miniexp_stringp( urlexp ) )
            {
                target = QString::fromUtf8( miniexp_to_str( miniexp_nth( 1, cur ) ) );
            }
            else if ( miniexp_listp( urlexp ) && ( miniexp_length( urlexp ) == 3 ) &&
                      miniexp_symbolp( miniexp_nth( 0, urlexp ) ) &&
                      ( qstrncmp( miniexp_to_name( miniexp_nth( 0, urlexp ) ), "url", 3 ) == 0 ) )
            {
                target = QString::fromUtf8( miniexp_to_str( miniexp_nth( 1, urlexp ) ) );
            }
            if ( target.isEmpty() || ( ( target.length() > 0 ) && target.at(0) == QLatin1Char( '#' ) ) )
            {
                KDjVu::PageLink* plink = new KDjVu::PageLink();
                plink->m_page = target;
                link = plink;
            }
            else
            {
                KDjVu::UrlLink* ulink = new KDjVu::UrlLink();
                ulink->m_url = target;
                link = ulink;
            }
        }
        else if ( annotations &&
                  ( type == QLatin1String( "text" ) ||
                    type == QLatin1String( "line" ) ) )
        {
            miniexp_t area = miniexp_nth( 3, cur );
            int a = miniexp_to_int( miniexp_nth( 1, area ) );
            int b = miniexp_to_int( miniexp_nth( 2, area ) );
            int c = miniexp_to_int( miniexp_nth( 3, area ) );
            int d = miniexp_to_int( miniexp_nth( 4, area ) );
            if ( type == QLatin1String( "text" ) )
            {
                KDjVu::TextAnnotation * textann = new KDjVu::TextAnnotation();
                textann->m_size = QSize( c, d );
                ann = textann;
            }
            else if ( type == QLatin1String( "line" ) )
            {
                KDjVu::LineAnnotation * lineann = new KDjVu::LineAnnotation();
                lineann->m_point2 = QPoint( c, d );
                ann = lineann;
            }
            ann->m_point = QPoint( a, b );
            ann->m_comment = QString::fromUtf8( miniexp_to_str( miniexp_nth( 2, cur ) ) );
        }
        if ( link /* safety check */ && links )
        {
            link->m_area = KDjVu::Link::UnknownArea;
            miniexp_t area = miniexp_nth( 3, cur );
            int arealength = miniexp_length( area );
            if ( ( arealength == 5 ) && ( type == QLatin1String( "rect" ) || type == QLatin1String( "oval" ) ) )
            {
                link->m_point = QPoint( miniexp_to_int( miniexp_nth( 1, area ) ), miniexp_to_int( miniexp_nth( 2, area ) ) );
                link->m_size = QSize( miniexp_to_int( miniexp_nth( 3, area ) ), miniexp_to_int( miniexp_nth( 4, area ) ) );
                if ( type == QLatin1String( "rect" ) )
                {
                    link->m_area = KDjVu::Link::RectArea;
                }
                else
                {
                    link->m_area = KDjVu::Link::EllipseArea;
                }
            }
            else if ( ( arealength > 0 ) && ( arealength % 2 == 1 ) &&
                      type == QLatin1String( "poly" ) )
            {
                link->m_area = KDjVu::Link::PolygonArea;
                QPolygon poly;
                for ( int j = 1; j < arealength; j += 2 ) 
                {
                    poly << QPoint( miniexp_to_int( miniexp_nth( j, area ) ), miniexp_to_int( miniexp_nth( j + 1, area ) ) );
                }
                link->m_poly = poly;
            }

            if ( link->m_area != KDjVu::Link::UnknownArea )
                links->append( link );
        }
        else if ( ann /* safety check */ && annotations )
        {
            if ( type == QLatin1String( "text" ) )
            {
                KDjVu::TextAnnotation* txtann = (KDjVu::TextAnnotation*)ann;
                txtann->m_color.setRgb( 255, 255, 255, 0 );
                for ( int j = 4; j < num; ++j )
                {
                    miniexp_t curelem = miniexp_nth( j, cur );
                    if ( !miniexp_listp( curelem ) )
                        continue;

                    QString id = QString::fromUtf8( miniexp_to_name( miniexp_nth( 0, curelem ) ) );
                    if ( id == QLatin1String( "backclr" ) )
                        txtann->m_color.setNamedColor( QString::fromUtf8( miniexp_to_name( miniexp_nth( 1, curelem ) ) ) );
                    else if ( id == QLatin1String( "pushpin" ) )
                        txtann->m_inlineText = false;
                }
            }
            else if ( type == QLatin1String( "line" ) )
            {
                KDjVu::LineAnnotation* lineann = (KDjVu::LineAnnotation*)ann;
                lineann->m_color = Qt::black;
                for ( int j = 4; j < num; ++j )
                {
                    miniexp_t curelem = miniexp_nth( j, cur );
                    if ( !miniexp_listp( curelem ) )
                        continue;

                    QString id = QString::fromUtf8( miniexp_to_name( miniexp_nth( 0, curelem ) ) );
                    if ( id == QLatin1String( "lineclr" ) )
                        lineann->m_color.setNamedColor( QString::fromUtf8( miniexp_to_name( miniexp_nth( 1, curelem ) ) ) );
                    else if ( id == QLatin1String( "arrow" ) )
                        lineann->m_isArrow = true;
                    else if ( id == QLatin1String( "width" ) )
                        lineann->m_width = miniexp_to_int( miniexp_nth( 1, curelem ) );
                }
            }
            annotations->append( ann );
        }
    }
}

const QVector<KDjVu::Page*> &KDjVu::pages() const
{
    return d->m_pages;
}

QImage KDjVu::image( int page, int width, int height, int rotation )
{
    bool found = false;
    QList<ImageCacheItem*>::Iterator it = d->mImgCache.begin(), itEnd = d->mImgCache.end();
    for ( ; ( it != itEnd ) && !found; ++it )
    {
        ImageCacheItem* cur = *it;
        if ( ( cur->page == page ) &&
             ( rotation % 2 == 0
               ? cur->width == width && cur->height == height
               : cur->width == height && cur->height == width ) )
            found = true;
    }
    if ( !found )
        return QImage();

    // taking the element and pushing to the top of the list
    --it;
    ImageCacheItem* cur2 = *it;
    d->mImgCache.erase( it );
    d->mImgCache.push_front( cur2 );

    return cur2->img;
}

void KDjVu::requestImage( int page, int width, int height, int rotation )
{
    QImage tmp = image( page, width, height, rotation );
    if ( !tmp.isNull() )
    {
        emit imageGenerated( page, tmp );
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

/*
    if ( ddjvu_page_get_rotation( djvupage ) != flipRotation( rotation ) )
    {
// TODO: test documents with initial rotation != 0
//        ddjvu_page_set_rotation( djvupage, m_pages.at( page )->orientation() );
        ddjvu_page_set_rotation( djvupage, (ddjvu_page_rotation_t)flipRotation( rotation ) );
    }
*/

    static const int xdelta = 1500;
    static const int ydelta = 1500;

    int xparts = width / xdelta + 1;
    int yparts = height / ydelta + 1;

    QImage newimg( width, height, QImage::Format_RGB32 );

    int res = 10000;
    if ( ( xparts == 1 ) && ( yparts == 1 ) )
    {
         // only one part -- render at once with no need to auxiliary image
         newimg = d->generateImageTile( djvupage, res,
                 width, 0, xdelta, height, 0, ydelta );
    }
    else
    {
        // more than one part -- need to render piece-by-piece and to compose
        // the results
        QPainter p;
        p.begin( &newimg );
        int parts = xparts * yparts;
        for ( int i = 0; i < parts; ++i )
        {
            int row = i % xparts;
            int col = i / xparts;
            int tmpres = 0;
            QImage tempp = d->generateImageTile( djvupage, tmpres,
                    width, row, xdelta, height, col, ydelta );
            if ( tmpres )
            {
                p.drawImage( row * xdelta, col * ydelta, tempp );
            }
            res = qMin( tmpres, res );
        }
        p.end();
    }

    QImage resimg;

    if ( res )
    {
        resimg = newimg;

        // delete all the cached pixmaps for the current page with a size that
        // differs no more than 35% of the new pixmap size
        int imgsize = resimg.width() * resimg.height();
        if ( imgsize > 0 )
        {
            for( int i = 0; i < d->mImgCache.count(); )
            {
                ImageCacheItem* cur = d->mImgCache.at(i);
                if ( ( cur->page == page ) &&
                     ( abs( cur->img.width() * cur->img.height() - imgsize ) < imgsize * 0.35 ) )
                {
                    d->mImgCache.removeAt( i );
                    delete cur;
                }
                else
                    ++i;
            }
        }

        // the image cache has too many elements, remove the last
        if ( d->mImgCache.size() >= 10 )
        {
            delete d->mImgCache.last();
            d->mImgCache.removeLast();
        }
        ImageCacheItem* ich = new ImageCacheItem( page, width, height, newimg );
        d->mImgCache.push_front( ich );
    }

    emit imageGenerated( page, newimg );
}

bool KDjVu::exportAsPostScript( const QString & fileName, const QList<int>& pageList ) const
{
    if ( !d->m_djvu_document || pageList.isEmpty() )
        return false;

    QString pl;
    foreach ( int p, pageList )
    {
        if ( !pl.isEmpty() )
            pl += QString::fromLatin1( "," );
        pl += QString::number( p );
    }
    pl.prepend( "-page=" );

    QByteArray fn = QFile::encodeName( fileName );
    FILE* f = fopen( fn.constData(), "w+" );
    if ( !f )
    {
        kDebug() << "KDjVu::exportAsPostScript(): error while opening the file" << endl;
        return false;
    }

    // setting the options
    static const int optc = 1;
    const char ** optv = (const char**)malloc( 1 * sizeof( char* ) );
    QByteArray plb = pl.toAscii();
    optv[0] = plb.constData();

    ddjvu_job_t *printjob = ddjvu_document_print( d->m_djvu_document, f, optc, optv );
    while ( !ddjvu_job_done( printjob ) )
       handle_ddjvu_messages( d->m_djvu_cxt, true );

    free( optv );

    // no need to close 'f' with fclose(), as it's already done by djvulibre

    return true;
}


#include "kdjvu.moc"
