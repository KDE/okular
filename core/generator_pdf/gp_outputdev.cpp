/***************************************************************************
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Andy Goossens <andygoossens@telenet.be>         *
 *   Copyright (C) 2003 by Scott Wheeler <wheeler@kde.org>                 *
 *   Copyright (C) 2003 by Ingo Klöcker <kloecker@kde.org>                 *
 *   Copyright (C) 2003 by Will Andrews <will@csociety.org>                *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Waldo Bastian <bastian@kde.org>                 *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifdef __GNUC__
#pragma implementation
#endif

#include <kdebug.h>
#include <qpixmap.h>
#include <qimage.h>

#include "gp_outputdev.h"
#include "generator_pdf.h"
#include "core/document.h" // for DocumentViewport
#include "core/page.h"
#include "core/link.h"
#include "xpdf/Link.h"
#include "xpdf/GfxState.h"
#include "xpdf/TextOutputDev.h"
#include "splash/SplashBitmap.h"

//NOTE: XPDF/Splash *implementation dependant* code is marked with '###'

/** KPDFOutputDev implementation **/

KPDFOutputDev::KPDFOutputDev( SplashColor paperColor )
    : SplashOutputDev( splashModeRGB8, false, paperColor ),
    m_doc( 0 ), m_pixmap( 0 ), m_image( 0 ), m_text( 0 )
{
}

KPDFOutputDev::~KPDFOutputDev()
{
    clear();
}

void KPDFOutputDev::initDevice( PDFDoc * pdfDoc )
{
    m_doc = pdfDoc;
    startDoc( pdfDoc->getXRef() );
}

void KPDFOutputDev::setParams( int width, int height, bool genT, bool genL, bool genI, bool safe )
{
    clear();

    m_pixmapWidth = width;
    m_pixmapHeight = height;

    m_qtThreadSafety = safe;
    m_generateText = genT;
    m_generateLinks = genL;
    m_generateImages = genI;

    if ( m_generateText )
        m_text = new TextPage( gFalse );
}

QPixmap * KPDFOutputDev::takePixmap()
{
    QPixmap * pix = m_pixmap;
    m_pixmap = 0;
    return pix;
}

QImage * KPDFOutputDev::takeImage()
{
    QImage * img = m_image;
    m_image = 0;
    return img;
}

TextPage * KPDFOutputDev::takeTextPage()
{
    TextPage * text = m_text;
    m_text = 0;
    return text;
}

QValueList< KPDFPageRect * > KPDFOutputDev::takeRects()
{
    if ( m_rects.isEmpty() )
        return m_rects;
    QValueList< KPDFPageRect * > rectsCopy( m_rects );
    m_rects.clear();
    return rectsCopy;
}

//BEGIN - OutputDev hooked calls
void KPDFOutputDev::startPage( int pageNum, GfxState *state )
{
    if ( m_generateText )
        m_text->startPage( state );
    SplashOutputDev::startPage( pageNum, state );
}

void KPDFOutputDev::endPage()
{
    SplashOutputDev::endPage();
    if ( m_generateText )
        m_text->coalesce( gTrue );

    int bh = getBitmap()->getHeight(),
        bw = getBitmap()->getWidth();
    SplashColorPtr dataPtr = getBitmap()->getDataPtr();
    // construct a qimage SHARING the raw bitmap data in memory
    QImage * img = new QImage( (uchar*)dataPtr.rgb8, bw, bh, 32, 0, 0, QImage::IgnoreEndian );

    // use the QImage or convert it immediately to QPixmap for better
    // handling and memory unloading
    if ( m_qtThreadSafety )
    {
        delete m_image;
        // it may happen (in fact it doesn't) that we need a rescaling
        if ( bw != m_pixmapWidth && bh != m_pixmapHeight )
            m_image = new QImage( img->smoothScale( m_pixmapWidth, m_pixmapHeight ) );
        else
            // dereference image from the xpdf memory
            m_image = new QImage( img->copy() );
    }
    else
    {
        delete m_pixmap;
        // it may happen (in fact it doesn't) that we need a rescaling
        if ( bw != m_pixmapWidth || bh != m_pixmapHeight )
            m_pixmap = new QPixmap( img->smoothScale( m_pixmapWidth, m_pixmapHeight ) );
        else
            m_pixmap = new QPixmap( *img );
    }

    // destroy the shared descriptor and (###) unload underlying xpdf bitmap
    delete img;
    SplashOutputDev::startPage( 0, NULL );
}

void KPDFOutputDev::drawLink( Link * link, Catalog * catalog )
{
    if ( !link->isOk() )
        return;

    if ( m_generateLinks )
    {
        // create the link descriptor
        KPDFLink * l = generateLink( link->getAction() );
        if ( l )
        {
            // create the page rect representing the link
            double x1, y1, x2, y2;
            link->getRect( &x1, &y1, &x2, &y2 );
            int left, top, right, bottom;
            cvtUserToDev( x1, y1, &left, &top );
            cvtUserToDev( x2, y2, &right, &bottom );
            double nl = (double)left / (double)m_pixmapWidth,
                   nt = (double)top / (double)m_pixmapHeight,
                   nr = (double)right / (double)m_pixmapWidth,
                   nb = (double)bottom / (double)m_pixmapHeight;
            // create the rect using normalized coords
            KPDFPageRect * rect = new KPDFPageRect( nl, nt, nr, nb );
            // attach the link  object to the rect and add it to the vector container
            rect->setPointer( l, KPDFPageRect::Link );
            m_rects.push_back( rect );
        }
    }
    SplashOutputDev::drawLink( link, catalog );
}

void KPDFOutputDev::updateFont( GfxState *state )
{
    if ( m_generateText )
        m_text->updateFont( state );
    SplashOutputDev::updateFont( state );
}

void KPDFOutputDev::drawChar( GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, Unicode *u, int uLen )
{
    if ( m_generateText )
        m_text->addChar( state, x, y, dx, dy, code, u, uLen );
    SplashOutputDev::drawChar( state, x, y, dx, dy, originX, originY, code, u, uLen );
}

GBool KPDFOutputDev::beginType3Char( GfxState *state, double x, double y, double dx, double dy, CharCode code, Unicode *u, int uLen )
{
    if ( m_generateText )
        m_text->addChar( state, x, y, dx, dy, code, u, uLen );
    return SplashOutputDev::beginType3Char( state, x, y, dx, dy, code, u, uLen );
}

void KPDFOutputDev::drawImage( GfxState *state, Object *ref, Stream *str,
    int _width, int _height, GfxImageColorMap *colorMap, int *maskColors, GBool inlineImg )
{
    if ( m_generateImages )
    {
        // find out image rect from the Coord Transform Matrix
        double * ctm = state->getCTM();
        int left = (int)ctm[4],
            top = (int)ctm[5],
            width = (int)ctm[0],
            height = (int)ctm[3];
        // normalize width
        if ( width < 0 )
        {
            width = -width;
            left -= width;
        }
        // normalize height
        if ( height < 0 )
        {
            height = -height;
            top -= height;
        }
        if ( width > 10 && height > 10 )
        {
            // build a descriptor for the image rect
            double nl = (double)left / (double)m_pixmapWidth,
                   nt = (double)top / (double)m_pixmapHeight,
                   nr = (double)(left + width) / (double)m_pixmapWidth,
                   nb = (double)(top + height) / (double)m_pixmapHeight;
            // create the rect using normalized coords
            KPDFPageRect * r = new KPDFPageRect( nl, nt, nr, nb );
            r->setPointer( 0, KPDFPageRect::Image );
            // add the rect to the vector container
            m_rects.push_back( r );
        }
    }
    SplashOutputDev::drawImage( state, ref, str, _width, _height, colorMap, maskColors, inlineImg );
}
//END - OutputDev hooked calls

//BEGIN - private helpers
void KPDFOutputDev::clear()
{
    // delete rects
    if ( m_rects.count() )
    {
        QValueList< KPDFPageRect * >::iterator it = m_rects.begin(), end = m_rects.end();
        for ( ; it != end; ++it )
            delete *it;
        m_rects.clear();
    }
    // delete pixmap
    if ( m_pixmap )
    {
        delete m_pixmap;
        m_pixmap = 0;
    }
    // delete image
    if ( m_image )
    {
        delete m_image;
        m_image = 0;
    }
    // delete text
    if ( m_text )
    {
        delete m_text;
        m_text = 0;
    }
}

KPDFLink * KPDFOutputDev::generateLink( LinkAction * a )
// note: this function is called when processing a page, when the MUTEX is already LOCKED
{
    KPDFLink * link = NULL;
    if ( a ) switch ( a->getKind() )
    {
        case actionGoTo:
            {
            LinkGoTo * g = (LinkGoTo *) a;
            // ceate link: no ext file, namedDest, object pointer
            link = new KPDFLinkGoto( QString::null, decodeViewport( g->getNamedDest(), g->getDest() ) );
            }
            break;

        case actionGoToR:
            {
            LinkGoToR * g = (LinkGoToR *) a;
            // copy link file
            const char * fileName = g->getFileName()->getCString();
            // ceate link: fileName, namedDest, object pointer
            link = new KPDFLinkGoto( (QString)fileName, decodeViewport( g->getNamedDest(), g->getDest() ) );
            }
            break;

        case actionLaunch:
            {
            LinkLaunch * e = (LinkLaunch *)a;
            GString * p = e->getParams();
            link = new KPDFLinkExecute( e->getFileName()->getCString(), p ? p->getCString() : 0 );
            }
            break;

        case actionNamed:
            {
            const char * name = ((LinkNamed *)a)->getName()->getCString();
            if ( !strcmp( name, "NextPage" ) )
                link = new KPDFLinkAction( KPDFLinkAction::PageNext );
            else if ( !strcmp( name, "PrevPage" ) )
                link = new KPDFLinkAction( KPDFLinkAction::PagePrev );
            else if ( !strcmp( name, "FirstPage" ) )
                link = new KPDFLinkAction( KPDFLinkAction::PageFirst );
            else if ( !strcmp( name, "LastPage" ) )
                link = new KPDFLinkAction( KPDFLinkAction::PageLast );
            else if ( !strcmp( name, "GoBack" ) )
                link = new KPDFLinkAction( KPDFLinkAction::HistoryBack );
            else if ( !strcmp( name, "GoForward" ) )
                link = new KPDFLinkAction( KPDFLinkAction::HistoryForward );
            else if ( !strcmp( name, "Quit" ) )
                link = new KPDFLinkAction( KPDFLinkAction::Quit );
            else if ( !strcmp( name, "GoToPage" ) )
                link = new KPDFLinkAction( KPDFLinkAction::GoToPage );
            else if ( !strcmp( name, "Find" ) )
                link = new KPDFLinkAction( KPDFLinkAction::Find );
            else
                kdDebug() << "Unknown named action: '" << name << "'" << endl;
            }
            break;

        case actionURI:
            link = new KPDFLinkBrowse( ((LinkURI *)a)->getURI()->getCString() );
            break;

        case actionMovie:
/*          { TODO this (Movie link)
            m_type = Movie;
            LinkMovie * m = (LinkMovie *) a;
            // copy Movie parameters (2 IDs and a const char *)
            Ref * r = m->getAnnotRef();
            m_refNum = r->num;
            m_refGen = r->gen;
            copyString( m_uri, m->getTitle()->getCString() );
            }
*/          break;

        case actionUnknown:
            kdDebug() << "Unknown link." << endl;
            break;
    }

    // link may be zero at that point
    return link;
}

DocumentViewport KPDFOutputDev::decodeViewport( GString * namedDest, LinkDest * dest )
// note: this function is called when processing a page, when the MUTEX is already LOCKED
{
    DocumentViewport vp( -1 );

    if ( namedDest && !dest )
        dest = m_doc->findDest( namedDest );

    if ( !dest || !dest->isOk() )
        return vp;

    // get destination page number
    if ( !dest->isPageRef() )
        vp.pageNumber = dest->getPageNum() - 1;
    else
    {
        Ref ref = dest->getPageRef();
        vp.pageNumber = m_doc->findPage( ref.num, ref.gen ) - 1;
    }

    // get destination position
    // TODO add other attributes to the viewport (taken from link)
    switch ( dest->getKind() )
    {
        case destXYZ:
/*            OD -> cvtUserToDev( dest->getLeft(), dest->getTop(), &X, &Y );
            if ( dest->getChangeLeft() )
                make hor change
            if ( dest->getChangeTop() )
                make ver change
            if ( dest->getChangeZoom() )
                make zoom change
*/          break;

        case destFit:
        case destFitB:
            //vp.fitWidth = true;
            //vp.fitHeight = true;
            break;

        case destFitH:
        case destFitBH:
//            read top, fit Width
            //vp.fitWidth = true;
            break;

        case destFitV:
        case destFitBV:
//            read left, fit Height
            //vp.fitHeight = true;
            break;

        case destFitR:
//            read and fit left,bottom,right,top
            break;
    }

    return vp;
}
//END - private helpers


/** KPDFTextDev implementation **/

KPDFTextDev::KPDFTextDev()
{
    m_text = new TextPage( gFalse );
}

KPDFTextDev::~KPDFTextDev()
{
    delete m_text;
}

TextPage * KPDFTextDev::takeTextPage()
{
    TextPage * t = m_text;
    m_text = 0;
    return t;
}

void KPDFTextDev::startPage( int, GfxState *state )
{
    if ( !m_text )
        m_text = new TextPage( gFalse );
    m_text->startPage( state );
}

void KPDFTextDev::endPage()
{
    m_text->endPage();
    m_text->coalesce( gTrue );
}

void KPDFTextDev::updateFont( GfxState *state )
{
    m_text->updateFont( state );
}

void KPDFTextDev::drawChar( GfxState *state, double x, double y, double dx, double dy, double /*originX*/, double /*originY*/, CharCode code, Unicode *u, int uLen )
{
    m_text->addChar( state, x, y, dx, dy, code, u, uLen );
}
