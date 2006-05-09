/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
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
#include "settings.h"
#include "area.h"

// temp includes
#include <sys/time.h>
class TextSelection;

/** class KPDFPage **/

KPDFPage::KPDFPage( uint page, double w, double h, int r )
    : m_number( page ), m_rotation( r ), m_width( w ), m_height( h ),
    m_bookmarked( false ), m_text( 0 ), m_transition( 0 )
{
    // if landscape swap width <-> height (rotate 90deg CCW)
/*    if ( r == 90 || r == 270 )
    {
        m_width = h;
        m_height = w;
    }*/
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

RegularAreaRect * KPDFPage::getTextArea ( TextSelection * sel ) const
{
    if (m_text)
	return m_text->getTextArea (sel);
    return 0;
}

bool KPDFPage::hasObjectRect( double x, double y ) const
{
    if ( m_rects.count() < 1 )
        return false;
    QLinkedList< ObjectRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
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
    QLinkedList< HighlightAreaRect * >::const_iterator it = m_highlights.begin(), end = m_highlights.end();
    for ( ; it != end; ++it )
        if ( (*it)->s_id == s_id )
            return true;
    return false;
}

bool KPDFPage::hasTransition() const
{
    return m_transition != 0;
}


RegularAreaRect * KPDFPage::findText( const QString & text, SearchDir dir, const bool strictCase, 
	const RegularAreaRect * lastRect/*, const Generator &generator */) const
{
	RegularAreaRect* ret=0;
	if ( text.isEmpty() )
		return ret;
/*
    if (generator->preferInternalSearch())
	return generator->;*/

	ret=m_text->findText(text, dir, strictCase,lastRect);
	return ret;
/*

    */
}

QString * KPDFPage::getText( const RegularAreaRect * area ) const
{
	QString *ret= 0;

	if ( !m_text )
		return ret;

	ret = m_text->getText( area );

	return ret;
}

const ObjectRect * KPDFPage::getObjectRect( ObjectRect::ObjectType type, double x, double y ) const
{
    QLinkedList< ObjectRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( x, y ) && ((*it)->objectType() == type) )
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

void KPDFPage::setSearchPage( KPDFTextPage * tp )
{
    delete m_text;
    m_text = tp;
}

void KPDFPage::setBookmark( bool state )
{
    m_bookmarked = state;
}

void KPDFPage::setObjectRects( const QLinkedList< ObjectRect * > rects )
{
    qDeleteAll(m_rects);
    m_rects = rects;
}

/*
void KPDFPage::setHighlight( int s_id, const QColor & color )
{
	QLinkedList<HighlightAreaRect*>::Iterator it=m_highlights.begin();
	HighlightAreaRect* tmp;
	while(it!=m_highlights.end())
	{
		tmp = *(it);
		if (tmp->s_id == s_id)
			tmp->color=color;
		++it;
	}
}*/

// 
void KPDFPage::setHighlight( int s_id, RegularAreaRect *rect, const QColor & color )
{
    HighlightAreaRect * hr = new HighlightAreaRect(rect);
    hr->s_id = s_id;
    hr->color = color;
    m_highlights.append( hr );

/*	for (i=static_cast<RegularAreaRect::Iterator>(rect.begin());i!=rect.end();i++)
	{
		// create a HighlightRect descriptor taking values from params
		NormalizedRect *t = new NormalizedRect;
		t->left = i->left;
		t->top = i->top;
		t->right = i->right;
		t->bottom = i->bottom;
		// append the HighlightRect to the list
		hr->append( t );
		// delete old object and change reference (whyyy?)
		delete rect;
		rect = hr;
	}*/
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
    qDeleteAll(m_rects);
    m_rects.clear();
}

void KPDFPage::deleteHighlights( int s_id )
{
    // delete highlights by ID
    QLinkedList< HighlightAreaRect* >::iterator it = m_highlights.begin(), end = m_highlights.end();
    while ( it != end )
    {
        HighlightAreaRect* highlight = *it;
        if ( s_id == -1 || highlight->s_id == s_id )
        {
            it = m_highlights.erase( it );
            delete highlight;
        }
        else
            ++it;
    }
}

void KPDFPage::deleteAnnotations()
{
    // delete all stored annotations
    QLinkedList< Annotation * >::iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
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
             struct timeval ts, te;
             gettimeofday( &ts, NULL );
            // iterate over all annotations
            QDomNode annotationNode = childElement.firstChild();
            while( annotationNode.isElement() )
            {
                // get annotation element and advance to next annot
                QDomElement annotElement = annotationNode.toElement();
                annotationNode = annotationNode.nextSibling();

                // get annotation from the dom element
                Annotation * annotation = AnnotationUtils::createAnnotation( annotElement );

                // append annotation to the list or show warning
                if ( annotation )
                    m_annotations.append( annotation );
                else
                    kWarning() << "page (" << m_number << "): can't restore an annotation from XML." << endl;
            }
             gettimeofday( &te, NULL );
             double startTime = (double)ts.tv_sec + ((double)ts.tv_usec) / 1000000.0;
             double endTime = (double)te.tv_sec + ((double)te.tv_usec) / 1000000.0;
             kDebug() << "annots: XML Load time: " << (endTime-startTime)*1000.0 << "ms" << endl;
        }

        // parse bookmark child element
        else if ( childElement.tagName() == "bookmark" )
            m_bookmarked = true;
    }
}

void KPDFPage::saveLocalContents( QDomNode & parentNode, QDomDocument & document )
{
    // only add a node if there is some stuff to write into
    if ( !m_bookmarked && m_annotations.isEmpty() )
        return;

    // create the page node and set the 'number' attribute
    QDomElement pageElement = document.createElement( "page" );
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
#if 0
        struct timeval ts, te;
        gettimeofday( &ts, NULL );
#endif
        // create the annotationList
        QDomElement annotListElement = document.createElement( "annotationList" );

        // add every annotation to the annotationList
        QLinkedList< Annotation * >::iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
        for ( ; aIt != aEnd; ++aIt )
        {
            // get annotation
            const Annotation * a = *aIt;
            // only save kpdf annotations (not the embedded in file ones)
            if ( !(a->flags & Annotation::External) )
            {
                // append an filled-up element called 'annotation' to the list
                QDomElement annElement = document.createElement( "annotation" );
                AnnotationUtils::storeAnnotation( a, annElement, document );
                annotListElement.appendChild( annElement );
            }
        }

        // append the annotationList element if annotations have been set
        if ( annotListElement.hasChildNodes() )
            pageElement.appendChild( annotListElement );
#if 0
        gettimeofday( &te, NULL );
        double startTime = (double)ts.tv_sec + ((double)ts.tv_usec) / 1000000.0;
        double endTime = (double)te.tv_sec + ((double)te.tv_usec) / 1000000.0;
        kDebug() << "annots: XML Save Time: " << (endTime-startTime)*1000.0 << "ms" << endl;
#endif
    }

    // append the page element only if has children
    if ( pageElement.hasChildNodes() )
        parentNode.appendChild( pageElement );
}
