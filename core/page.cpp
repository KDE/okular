/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include <kdebug.h>
#include <QTime>

// local includes
#include "annotations.h"
#include "area.h"
#include "link.h"
#include "page.h"
#include "pagetransition.h"
#include "rotationjob.h"

using namespace Okular;

class TextSelection;

static void deleteObjectRects( QLinkedList< ObjectRect * >& rects, const QSet<ObjectRect::ObjectType>& which )
{
    QLinkedList< ObjectRect * >::iterator it = rects.begin(), end = rects.end();
    for ( ; it != end; )
        if ( which.contains( (*it)->objectType() ) )
        {
            delete *it;
            it = rects.erase( it );
        }
        else
            ++it;
}

/** class Page **/

Page::Page( uint page, double w, double h, int o )
    : QObject( 0 ),
    m_number( page ), m_orientation( o ), m_rotation( 0 ), m_width( w ), m_height( h ),
    m_bookmarked( false ), m_maxuniqueNum( 0 ), m_text( 0 ), m_transition( 0 ),
    m_textSelections( 0 ), m_openingAction( 0 ), m_closingAction( 0 )
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

Page::~Page()
{
    deleteImages();
    deleteRects();
    deleteHighlights();
    deleteAnnotations();
    deleteTextSelections();
    deleteSourceReferences();
    delete m_text;
    delete m_transition;
}


bool Page::hasImage( int id, int width, int height ) const
{
    if ( !m_rotated_images.contains( id ) )
        return false;

    if ( width == -1 || height == -1 )
        return true;

    const QImage &image = m_rotated_images[ id ];

    return (image.width() == width && image.height() == height);
}

bool Page::hasSearchPage() const
{
    return m_text != 0;
}

bool Page::hasBookmark() const
{
    return m_bookmarked;
}

RegularAreaRect * Page::getTextArea ( TextSelection * sel ) const
{
    if (m_text)
	return m_text->getTextArea (sel);
    return 0;
}

bool Page::hasObjectRect( double x, double y, double xScale, double yScale ) const
{
    if ( m_rects.isEmpty() )
        return false;
    QLinkedList< ObjectRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( x, y, xScale, yScale ) )
            return true;
    return false;
}

bool Page::hasHighlights( int s_id ) const
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

bool Page::hasTransition() const
{
    return m_transition != 0;
}


RegularAreaRect * Page::findText( int searchID, const QString & text, SearchDir dir, bool strictCase,
	const RegularAreaRect * lastRect/*, const Generator &generator */) const
{
	RegularAreaRect* ret=0;
	if ( text.isEmpty() )
		return ret;
/*
    if (generator->preferInternalSearch())
	return generator->;*/

	ret=m_text->findText(searchID, text, dir, strictCase,lastRect);
	return ret;
/*

    */
}

QString Page::getText( const RegularAreaRect * area ) const
{
	QString ret;

	if ( !m_text )
		return ret;

	ret = m_text->getText( area );

	return ret;
}

void Page::rotateAt( int orientation )
{
    int neworientation = orientation % 4;
    if ( neworientation == m_rotation )
        return;

    deleteImages();
    deleteRects();
    deleteHighlights();
    deleteTextSelections();
    delete m_text;
    m_text = 0;

    if ( ( m_orientation + m_rotation ) % 2 != ( m_orientation + neworientation ) % 2 )
        qSwap( m_width, m_height );

    m_rotation = neworientation;

    QMapIterator< int, QImage > it( m_images );
    while ( it.hasNext() ) {
        it.next();

        if ( m_rotated_images.contains( it.key() ) )
            m_rotated_images[ it.key() ] = QImage();

        RotationJob::Rotation rotation = RotationJob::Rotation0;
        switch ( m_rotation ) {
            case 1:
                rotation = RotationJob::Rotation90;
                break;
            case 2:
                rotation = RotationJob::Rotation180;
                break;
            case 3:
                rotation = RotationJob::Rotation270;
                break;
            case 0:
            default:
                rotation = RotationJob::Rotation0;
                break;
        };

        RotationJob *job = new RotationJob( it.value(), rotation, it.key() );
        connect( job, SIGNAL( finished() ), this, SLOT( imageRotationDone() ) );
        job->start();
    }
}

void Page::imageRotationDone()
{
    RotationJob *job = static_cast<RotationJob*>( sender() );

    m_rotated_images[ job->id() ] = job->image();

    job->deleteLater();
}

const ObjectRect * Page::getObjectRect( ObjectRect::ObjectType type, double x, double y, double xScale, double yScale ) const
{
    QLinkedList< ObjectRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        if ( ( (*it)->objectType() == type ) && (*it)->contains( x, y, xScale, yScale ) )
            return *it;
    return 0;
}

const PageTransition * Page::getTransition() const
{
    return m_transition;
}

const Link * Page::getPageAction( PageAction act ) const
{
    switch ( act )
    {
        case Page::Opening:
            return m_openingAction;
            break;
        case Page::Closing:
            return m_closingAction;
            break;
    }
    return 0;
}

void Page::setImage( int id, const QImage &image )
{
    m_images[ id ] = image;

    if ( m_rotated_images.contains( id ) )
        m_rotated_images[ id ] = QImage();

    if ( m_rotation == 0 ) {
        m_rotated_images[ id ] = image;
    } else {
        QMatrix matrix;
        matrix.rotate( m_rotation * 90 );

        m_rotated_images[ id ] = image.transformed( matrix );
    }
}

void Page::setSearchPage( TextPage * tp )
{
    delete m_text;
    m_text = tp;
}

void Page::setBookmark( bool state )
{
    m_bookmarked = state;
}

void Page::setObjectRects( const QLinkedList< ObjectRect * > rects )
{
    QSet<ObjectRect::ObjectType> which;
    which << ObjectRect::Link << ObjectRect::Image;
    deleteObjectRects( m_rects, which );
    m_rects << rects;
}

/*
void Page::setHighlight( int s_id, const QColor & color )
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
void Page::setHighlight( int s_id, RegularAreaRect *rect, const QColor & color )
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

void Page::setTextSelections( RegularAreaRect *r, const QColor & color )
{
    deleteTextSelections();
    HighlightAreaRect * hr = new HighlightAreaRect( r );
    hr->s_id = -1;
    hr->color = color;
    m_textSelections = hr;
}

void Page::setSourceReferences( const QLinkedList< SourceRefObjectRect * > refRects )
{
    deleteSourceReferences();
    foreach( SourceRefObjectRect * rect, refRects )
        m_rects << rect;
}

void Page::addAnnotation( Annotation * annotation )
{
    //uniqueName: okular-PAGENUM-ID
    if(annotation->uniqueName.isEmpty())
    {
        annotation->uniqueName = "okular-";
        annotation->uniqueName += ( QString::number(m_number) + '-' +
                QString::number(++m_maxuniqueNum) );
        kDebug()<<"astario:     inc m_maxuniqueNum="<<m_maxuniqueNum<<endl;
    }
    m_annotations.append( annotation );
    m_rects.append( new AnnotationObjectRect( annotation ) );
}

void Page::modifyAnnotation(Annotation * newannotation )
{
    if(!newannotation)
        return;

    QLinkedList< Annotation * >::iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
    for ( ; aIt != aEnd; ++aIt )
    {
        if((*aIt)==newannotation)
            return; //modified already
        if((*aIt) && (*aIt)->uniqueName==newannotation->uniqueName)
        {
            int rectfound = false;
            QLinkedList< ObjectRect * >::iterator it = m_rects.begin(), end = m_rects.end();
            for ( ; it != end && !rectfound; ++it )
                if ( ( (*it)->objectType() == ObjectRect::OAnnotation ) && ( (*it)->pointer() == (*aIt) ) )
                {
                    delete *it;
                    *it = new AnnotationObjectRect( newannotation );
                    rectfound = true;
                }
            delete *aIt;
            *aIt = newannotation;
            break;
        }
    }
}

bool Page::removeAnnotation( Annotation * annotation )
{
    if ( !annotation || ( annotation->flags & Annotation::DenyDelete ) )
        return false;

    QLinkedList< Annotation * >::iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
    for ( ; aIt != aEnd; ++aIt )
    {
        if((*aIt) && (*aIt)->uniqueName==annotation->uniqueName)
        {
            int rectfound = false;
            QLinkedList< ObjectRect * >::iterator it = m_rects.begin(), end = m_rects.end();
            for ( ; it != end && !rectfound; ++it )
                if ( ( (*it)->objectType() == ObjectRect::OAnnotation ) && ( (*it)->pointer() == (*aIt) ) )
                {
                    delete *it;
                    m_rects.erase( it );
                    rectfound = true;
                }
            kDebug() << "removed annotation: " << annotation->uniqueName << endl;
            delete *aIt;
            m_annotations.erase( aIt );
            break;
        }
    }

    return true;
}

void Page::setTransition( PageTransition * transition )
{
    delete m_transition;
    m_transition = transition;
}

void Page::setPageAction( PageAction act, Link * action )
{
    switch ( act )
    {
        case Page::Opening:
            m_openingAction = action;
            break;
        case Page::Closing:
            m_closingAction = action;
            break;
    }
}

void Page::deleteImage( int id )
{
    m_rotated_images.remove( id );
}

void Page::deleteImages()
{
    m_rotated_images.clear();
}

void Page::deleteRects()
{
    // delete ObjectRects of type Link and Image
    QSet<ObjectRect::ObjectType> which;
    which << ObjectRect::Link << ObjectRect::Image;
    deleteObjectRects( m_rects, which );
}

void Page::deleteHighlights( int s_id )
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

void Page::deleteTextSelections()
{
    if (m_textSelections)
    {
        qDeleteAll(*m_textSelections);
        delete m_textSelections;
        m_textSelections = 0;
    }
}

void Page::deleteSourceReferences()
{
    deleteObjectRects( m_rects, QSet<ObjectRect::ObjectType>() << ObjectRect::SourceRef );
}

void Page::deleteAnnotations()
{
    // delete ObjectRects of type Annotation
    deleteObjectRects( m_rects, QSet<ObjectRect::ObjectType>() << ObjectRect::OAnnotation );
    // delete all stored annotations
    QLinkedList< Annotation * >::iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
    for ( ; aIt != aEnd; ++aIt )
        delete *aIt;
    m_annotations.clear();
}

void Page::restoreLocalContents( const QDomNode & pageNode )
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
            QTime time;
            time.start();

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
                {
                    m_annotations.append( annotation );
                    m_rects.append( new AnnotationObjectRect( annotation ) );
                    int pos = annotation->uniqueName.lastIndexOf("-");
                    if(pos != -1)
                    {
                        int uniqID=annotation->uniqueName.right(annotation->uniqueName.length()-pos-1).toInt();
                        if(m_maxuniqueNum<uniqID)
                            m_maxuniqueNum=uniqID;
                    }

                    kDebug()<<"astario:  restored annot:"<<annotation->uniqueName<<endl;
                }
                else
                    kWarning() << "page (" << m_number << "): can't restore an annotation from XML." << endl;
            }
            kDebug() << "annots: XML Load time: " << time.elapsed() << "ms" << endl;
        }

        // parse bookmark child element
        else if ( childElement.tagName() == "bookmark" )
            m_bookmarked = true;
    }
}

void Page::saveLocalContents( QDomNode & parentNode, QDomDocument & document )
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
        // create the annotationList
        QDomElement annotListElement = document.createElement( "annotationList" );

        // add every annotation to the annotationList
        QLinkedList< Annotation * >::iterator aIt = m_annotations.begin(), aEnd = m_annotations.end();
        for ( ; aIt != aEnd; ++aIt )
        {
            // get annotation
            const Annotation * a = *aIt;
            // only save okular annotations (not the embedded in file ones)
            if ( !(a->flags & Annotation::External) )
            {
                // append an filled-up element called 'annotation' to the list
                QDomElement annElement = document.createElement( "annotation" );
                AnnotationUtils::storeAnnotation( a, annElement, document );
                annotListElement.appendChild( annElement );
                kDebug()<<"astario:  save annot:"<<a->uniqueName<<endl;
            }
        }

        // append the annotationList element if annotations have been set
        if ( annotListElement.hasChildNodes() )
            pageElement.appendChild( annotListElement );
    }

    // append the page element only if has children
    if ( pageElement.hasChildNodes() )
        parentNode.appendChild( pageElement );
}

#include "page.moc"
