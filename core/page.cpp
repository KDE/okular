/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qpixmap.h>
#include <qstring.h>
#include <qmap.h>
#include <qdom.h>
#include <kdebug.h>

// local includes
#include "page.h"
#include "pagetransition.h"
#include "link.h"
#include "annotations.h"
#include "conf/settings.h"
#include "xpdf/TextOutputDev.h"


/** class KPDFPage **/

KPDFPage::KPDFPage( uint page, float w, float h, int r )
    : m_number( page ), m_rotation( r ), m_width( w ), m_height( h ),
    m_bookmarked( false ), m_text( 0 ), m_transition( 0 )
{
    // if landscape swap width <-> height (rotate 90deg CCW)
    if ( r == 90 || r == 270 )
    {
        m_width = h;
        m_height = w;
    }
    // avoid Division-By-Zero problems in the program
    if ( m_width <= 0 )
        m_width = 1;
    if ( m_height <= 0 )
        m_height = 1;
}

KPDFPage::~KPDFPage()
{
    deletePixmapsAndRects();
    deleteHighlights();
    deleteAnnotations();
    delete m_text;
    delete m_transition;
}


bool KPDFPage::hasPixmap( int id, int width, int height ) const
{
    if ( !m_pixmaps.contains( id ) )
        return false;
    if ( width == -1 || height == -1 )
        return true;
    QPixmap * p = m_pixmaps[ id ];
    return p ? ( p->width() == width && p->height() == height ) : false;
}

bool KPDFPage::hasSearchPage() const
{
    return m_text != 0;
}

bool KPDFPage::hasBookmark() const
{
    return m_bookmarked;
}

bool KPDFPage::hasObjectRect( double x, double y ) const
{
    if ( m_rects.count() < 1 )
        return false;
    QValueList< ObjectRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( x, y ) )
            return true;
    return false;
}

bool KPDFPage::hasHighlights( int s_id ) const
{
    // simple case: have no highlights
    if ( m_highlights.isEmpty() )
        return false;
    // simple case: we have highlights and no id to match
    if ( s_id == -1 )
        return true;
    // iterate on the highlights list to find an entry by id
    QValueList< HighlightRect * >::const_iterator it = m_highlights.begin(), end = m_highlights.end();
    for ( ; it != end; ++it )
        if ( (*it)->s_id == s_id )
            return true;
    return false;
}

bool KPDFPage::hasTransition() const
{
    return m_transition != 0;
}


NormalizedRect * KPDFPage::findText( const QString & text, bool strictCase, NormalizedRect * lastRect ) const
{
    if ( text.isEmpty() )
        return 0;

    // create a xpf's Unicode (unsigned int) array for the given text
    const QChar * str = text.unicode();
    int len = text.length();
    QMemArray<Unicode> u(len);
    for (int i = 0; i < len; ++i)
        u[i] = str[i].unicode();

    // find out the direction of search
    enum SearchDir { FromTop, NextMatch, PrevMatch } dir = lastRect ? NextMatch : FromTop;
    double sLeft, sTop, sRight, sBottom;
    if ( dir == NextMatch )
    {
        sLeft = lastRect->left * m_width;
        sTop = lastRect->top * m_height;
        sRight = lastRect->right * m_width;
        sBottom = lastRect->bottom * m_height;
    }

    // this loop is only for 'bad case' matches
    bool found = false;
    while ( !found )
    {
        if ( dir == FromTop )
            found = m_text->findText( const_cast<Unicode*>(static_cast<const Unicode*>(u)), len, gTrue, gTrue, gFalse, gFalse, &sLeft, &sTop, &sRight, &sBottom );
        else if ( dir == NextMatch )
            found = m_text->findText( const_cast<Unicode*>(static_cast<const Unicode*>(u)), len, gFalse, gTrue, gTrue, gFalse, &sLeft, &sTop, &sRight, &sBottom );
        else if ( dir == PrevMatch )
            // FIXME: this doesn't work as expected (luckily backward search isn't yet used)
            found = m_text->findText( const_cast<Unicode*>(static_cast<const Unicode*>(u)), len, gTrue, gFalse, gFalse, gTrue, &sLeft, &sTop, &sRight, &sBottom );

        // if not found (even in case unsensitive search), terminate
        if ( !found )
            break;

        // check for case sensitivity
        if ( strictCase )
        {
            // since we're in 'Case sensitive' mode, check if words are identical
            GString * realText = m_text->getText( sLeft, sTop, sRight, sBottom );
            found = QString::fromUtf8( realText->getCString() ) == text;
            if ( !found && dir == FromTop )
                dir = NextMatch;
            delete realText;
        }
    }

    // if the page was found, return a new normalizedRect
    if ( found )
        return new NormalizedRect( sLeft / m_width, sTop / m_height, sRight / m_width, sBottom / m_height );
    return 0;
}

const QString KPDFPage::getText( const NormalizedRect & rect ) const
{
    if ( !m_text )
        return QString::null;
    int left = (int)( rect.left * m_width ),
        top = (int)( rect.top * m_height ),
        right = (int)( rect.right * m_width ),
        bottom = (int)( rect.bottom * m_height );
    GString * text = m_text->getText( left, top, right, bottom );
    QString result = QString::fromUtf8( text->getCString() );
    delete text;
    return result; 
}

const ObjectRect * KPDFPage::getObjectRect( double x, double y ) const
{
    QValueList< ObjectRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( x, y ) )
            return *it;
    return 0;
}

const KPDFPageTransition * KPDFPage::getTransition() const
{
    return m_transition;
}


void KPDFPage::setPixmap( int id, QPixmap * pixmap )
{
    if ( m_pixmaps.contains( id ) )
        delete m_pixmaps[id];
    m_pixmaps[id] = pixmap;
}

void KPDFPage::setSearchPage( TextPage * tp )
{
    delete m_text;
    m_text = tp;
}

void KPDFPage::setBookmark( bool state )
{
    m_bookmarked = state;
}

void KPDFPage::setObjectRects( const QValueList< ObjectRect * > rects )
{
    QValueList< ObjectRect * >::iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        delete *it;
    m_rects = rects;
}

void KPDFPage::setHighlight( int s_id, NormalizedRect * &rect, const QColor & color )
{
    // create a HighlightRect descriptor taking values from params
    HighlightRect * hr = new HighlightRect();
    hr->s_id = s_id;
    hr->color = color;
    hr->left = rect->left;
    hr->top = rect->top;
    hr->right = rect->right;
    hr->bottom = rect->bottom;
    // append the HighlightRect to the list
    m_highlights.append( hr );
    // delete old object and change reference
    delete rect;
    rect = hr;
}

void KPDFPage::addAnnotation( Annotation * annotation )
{
    m_annotations.append( annotation );
}

void KPDFPage::setTransition( KPDFPageTransition * transition )
{
    delete m_transition;
    m_transition = transition;
}

void KPDFPage::deletePixmap( int id )
{
    if ( m_pixmaps.contains( id ) )
    {
        delete m_pixmaps[ id ];
        m_pixmaps.remove( id );
    }
}

void KPDFPage::deletePixmapsAndRects()
{
    // delete all stored pixmaps
    QMap<int,QPixmap *>::iterator it = m_pixmaps.begin(), end = m_pixmaps.end();
    for ( ; it != end; ++it )
        delete *it;
    m_pixmaps.clear();
    // delete ObjectRects
    QValueList< ObjectRect * >::iterator rIt = m_rects.begin(), rEnd = m_rects.end();
    for ( ; rIt != rEnd; ++rIt )
        delete *rIt;
    m_rects.clear();
}

void KPDFPage::deleteHighlights( int s_id )
{
    // delete highlights by ID
    QValueList< HighlightRect * >::iterator it = m_highlights.begin(), end = m_highlights.end();
    while ( it != end )
    {
        HighlightRect * highlight = *it;
        if ( s_id == -1 || highlight->s_id == s_id )
        {
            it = m_highlights.remove( it );
            delete highlight;
        }
        else
            ++it;
    }
}

void KPDFPage::deleteAnnotations()
{
    // delete all stored annotations
    QValueList< Annotation * >::iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
    for ( ; aIt != aEnd; ++aIt )
        delete *aIt;
    m_annotations.clear();
}

void KPDFPage::restoreLocalContents( const QDomNode & pageNode )
{
    // iterate over all chilren (bookmark, annotationList, ...)
    QDomNode childNode = pageNode.firstChild();
    while ( childNode.isElement() )
    {
        QDomElement childElement = childNode.toElement();
        childNode = childNode.nextSibling();

        // parse annotationList child element
        if ( childElement.tagName() == "annotationList" )
        {
            // iterate over all annotations
            QDomNode annotationNode = childElement.firstChild();
            while( annotationNode.isElement() )
            {
                // get annotation element and advance to next annot
                QDomElement annotElement = annotationNode.toElement();
                annotationNode = annotationNode.nextSibling();

                if ( !annotElement.hasAttribute( "type" ) )
                    continue;

                // build annotation of given type
                Annotation * annotation = 0;
                int typeNumber = annotElement.attribute( "type" ).toInt();
                switch ( typeNumber )
                {
                    case Annotation::APopup:
                        annotation = new PopupAnnotation( annotElement );
                        break;
                    case Annotation::AText:
                        annotation = new TextAnnotation( annotElement );
                        break;
                    case Annotation::ALine:
                        annotation = new LineAnnotation( annotElement );
                        break;
                    case Annotation::AGeom:
                        annotation = new GeomAnnotation( annotElement );
                        break;
                    case Annotation::AHighlight:
                        annotation = new HighlightAnnotation( annotElement );
                        break;
                    case Annotation::AStamp:
                        annotation = new StampAnnotation( annotElement );
                        break;
                    case Annotation::AInk:
                        annotation = new InkAnnotation( annotElement );
                        break;
                }

                // append annotation to the list or show warning
                if ( annotation )
                    m_annotations.append( annotation );
                else
                    kdWarning() << "can't restore Annotation of type '" << typeNumber << "from XML." << endl;
            }
        }

        // parse bookmark child element
        else if ( childElement.tagName() == "bookmark" )
            m_bookmarked = true;
    }
}

void KPDFPage::saveLocalContents( QDomNode & parentNode, QDomDocument & document )
{
    // only add a node if there is some stuff to write into
    if ( m_bookmarked || !m_annotations.isEmpty() )
    {
        // create the page node and set the 'number' attribute
        QDomElement pageElement = document.createElement( "page" );
        parentNode.appendChild( pageElement );
        pageElement.setAttribute( "number", m_number );

        // add bookmark info if is bookmarked
        if ( m_bookmarked )
        {
            // create the pageElement's 'bookmark' child
            QDomElement bookmarkElement = document.createElement( "bookmark" );
            pageElement.appendChild( bookmarkElement );

            // add attributes to the element
            //bookmarkElement.setAttribute( "name", bookmark name );
        }

        // add annotations info if has got any
        if ( !m_annotations.isEmpty() )
        {
            // create the annotationList
            QDomElement annotListElement = document.createElement( "annotationList" );
            pageElement.appendChild( annotListElement );

            // add every annotation to the annotationList
            QValueList< Annotation * >::iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
            for ( ; aIt != aEnd; ++aIt )
            {
                // get annotation
                const Annotation * a = *aIt;
                // create annotation element and set type
                QDomElement annotElement = document.createElement( "annotation" );
                annotListElement.appendChild( annotElement );
                annotElement.setAttribute( "type", (uint)a->subType() );
                // add children and attributes to annotation element
                a->store( annotElement, document );
            }
        }
    }
}


/** class NormalizedPoint **/
NormalizedPoint::NormalizedPoint()
    : x( 0.0 ), y( 0.0 ) {}

NormalizedPoint::NormalizedPoint( double dX, double dY )
    : x( dX ), y( dY ) {}

NormalizedPoint::NormalizedPoint( int iX, int iY, int xScale, int yScale )
    : x( (double)iX / (double)xScale ), y( (double)iY / (double)yScale ) {}


/** class NormalizedRect **/

NormalizedRect::NormalizedRect()
    : left( 0.0 ), top( 0.0 ), right( 0.0 ), bottom( 0.0 ) {}

NormalizedRect::NormalizedRect( double l, double t, double r, double b )
    // note: check for swapping coords?
    : left( l ), top( t ), right( r ), bottom( b ) {}

NormalizedRect::NormalizedRect( const QRect & r, double xScale, double yScale )
    : left( (double)r.left() / xScale ), top( (double)r.top() / yScale ),
    right( (double)r.right() / xScale ), bottom( (double)r.bottom() / yScale ) {}

bool NormalizedRect::contains( double x, double y ) const
{
    return x >= left && x <= right && y >= top && y <= bottom;
}

bool NormalizedRect::intersects( const NormalizedRect & r ) const
{
    return (r.left < right) && (r.right > left) && (r.top < bottom) && (r.bottom > top);
}

bool NormalizedRect::intersects( double l, double t, double r, double b ) const
{
    return (l < right) && (r > left) && (t < bottom) && (b > top);
}

QRect NormalizedRect::geometry( int xScale, int yScale ) const
{
    int l = (int)( left * xScale ),
        t = (int)( top * yScale ),
        r = (int)( right * xScale ),
        b = (int)( bottom * yScale );
    return QRect( l, t, r - l + 1, b - t + 1 );
}


/** class ObjectRect **/

ObjectRect::ObjectRect( double l, double t, double r, double b, ObjectType type, void * pnt )
    // assign coordinates swapping them if negative width or height
    : NormalizedRect( r > l ? l : r, b > t ? t : b, r > l ? r : l, b > t ? b : t ),
    m_objectType( type ), m_pointer( pnt )
{
}

ObjectRect::~ObjectRect()
{
    if ( !m_pointer )
        return;

    if ( m_objectType == Link )
        delete static_cast<KPDFLink*>( m_pointer );
    else
        kdDebug() << "Object deletion not implemented for type '" << m_objectType << "' ." << endl;
}
