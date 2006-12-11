/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <QtGui/QApplication>
#include <QtGui/QColor>

#include <kdebug.h>
#include <klocale.h>

// local includes
#include "annotations.h"

using namespace Okular;

//BEGIN AnnotationUtils implementation
Annotation * AnnotationUtils::createAnnotation( const QDomElement & annElement )
{
    // safety check on annotation element
    if ( !annElement.hasAttribute( "type" ) )
        return 0;

    // build annotation of given type
    Annotation * annotation = 0;
    int typeNumber = annElement.attribute( "type" ).toInt();
    switch ( typeNumber )
    {
        case Annotation::AText:
            annotation = new TextAnnotation( annElement );
            break;
        case Annotation::ALine:
            annotation = new LineAnnotation( annElement );
            break;
        case Annotation::AGeom:
            annotation = new GeomAnnotation( annElement );
            break;
        case Annotation::AHighlight:
            annotation = new HighlightAnnotation( annElement );
            break;
        case Annotation::AStamp:
            annotation = new StampAnnotation( annElement );
            break;
        case Annotation::AInk:
            annotation = new InkAnnotation( annElement );
            break;
    }

    // return created annotation
    return annotation;
}

void AnnotationUtils::storeAnnotation( const Annotation * ann, QDomElement & annElement,
    QDomDocument & document )
{
    // save annotation's type as element's attribute
    annElement.setAttribute( "type", (uint)ann->subType() );

    // append all annotation data as children of this node
    ann->store( annElement, document );
}

QDomElement AnnotationUtils::findChildElement( const QDomNode & parentNode,
    const QString & name )
{
    // loop through the whole children and return a 'name' named element
    QDomNode subNode = parentNode.firstChild();
    while( subNode.isElement() )
    {
        QDomElement element = subNode.toElement();
        if ( element.tagName() == name )
            return element;
        subNode = subNode.nextSibling();
    }
    // if the name can't be found, return a dummy null element
    return QDomElement();
}

QString AnnotationUtils::captionForAnnotation( Annotation * ann )
{
    if ( !ann )
        return QString();

    QString ret;
    switch( ann->subType() )
    {
        case Okular::Annotation::AText:
            if( ( (Okular::TextAnnotation*)ann )->textType() == Okular::TextAnnotation::Linked )
                ret = i18n( "Note" );
            else
                ret = i18n( "FreeText" );
            break;
        case Okular::Annotation::ALine:
            ret = i18n( "Line" );
            break;
        case Okular::Annotation::AGeom:
            ret = i18n( "Geom" );
            break;
        case Okular::Annotation::AHighlight:
            ret = i18n( "Highlight" );
            break;
        case Okular::Annotation::AStamp:
            ret = i18n( "Stamp" );
            break;
        case Okular::Annotation::AInk:
            ret = i18n( "Ink" );
            break;
        case Okular::Annotation::A_BASE:
            break;
    }
    return ret;
}

QRect AnnotationUtils::annotationGeometry( const Annotation * ann,
    double scaledWidth, double scaledHeight )
{
    if ( ann->subType() == Annotation::AText && ( ( (TextAnnotation*)ann )->textType() == TextAnnotation::Linked ) )
    {
        return QRect( (int)( ann->transformedBoundingRectangle().left * scaledWidth ),
                      (int)( ann->transformedBoundingRectangle().top * scaledHeight ), 24, 24 );
    }

    return ann->transformedBoundingRectangle().geometry( (int)scaledWidth, (int)scaledHeight );
}
//END AnnotationUtils implementation


//BEGIN Annotation implementation
Annotation::Style::Style()
    : m_opacity( 1.0 ), m_width( 1.0 ), m_style( Solid ), m_xCorners( 0.0 ),
      m_yCorners( 0.0 ), m_marks( 3 ), m_spaces( 0 ), m_effect( NoEffect ),
      m_effectIntensity( 1.0 )
{
}

void Annotation::Style::setColor( const QColor &color )
{
    m_color = color;
}

QColor Annotation::Style::color() const
{
    return m_color;
}

void Annotation::Style::setOpacity( double opacity )
{
    m_opacity = opacity;
}

double Annotation::Style::opacity() const
{
    return m_opacity;
}

void Annotation::Style::setWidth( double width )
{
    m_width = width;
}

double Annotation::Style::width() const
{
    return m_width;
}

void Annotation::Style::setLineStyle( LineStyle style )
{
    m_style = style;
}

Annotation::LineStyle Annotation::Style::lineStyle() const
{
    return m_style;
}

void Annotation::Style::setXCorners( double xCorners )
{
    m_xCorners = xCorners;
}

double Annotation::Style::xCorners() const
{
    return m_xCorners;
}

void Annotation::Style::setYCorners( double yCorners )
{
    m_yCorners = yCorners;
}

double Annotation::Style::yCorners() const
{
    return m_yCorners;
}

void Annotation::Style::setMarks( int marks )
{
    m_marks = marks;
}

int Annotation::Style::marks() const
{
    return m_marks;
}

void Annotation::Style::setSpaces( int spaces )
{
    m_spaces = spaces;
}

int Annotation::Style::spaces() const
{
    return m_spaces;
}

void Annotation::Style::setLineEffect( LineEffect effect )
{
    m_effect = effect;
}

Annotation::LineEffect Annotation::Style::lineEffect() const
{
    return m_effect;
}

void Annotation::Style::setEffectIntensity( double intensity )
{
    m_effectIntensity = intensity;
}

double Annotation::Style::effectIntensity() const
{
    return m_effectIntensity;
}




Annotation::Window::Window()
    : m_flags( -1 ), m_width( 0 ), m_height( 0 )
{
}

void Annotation::Window::setFlags( int flags )
{
    m_flags = flags;
}

int Annotation::Window::flags() const
{
    return m_flags;
}

void Annotation::Window::setTopLeft( const NormalizedPoint &point )
{
    m_topLeft = point;
}

NormalizedPoint Annotation::Window::topLeft() const
{
    return m_topLeft;
}

void Annotation::Window::setWidth( int width )
{
    m_width = width;
}

int Annotation::Window::width() const
{
    return m_width;
}

void Annotation::Window::setHeight( int height )
{
    m_height = height;
}

int Annotation::Window::height() const
{
    return m_height;
}

void Annotation::Window::setTitle( const QString &title )
{
    m_title = title;
}

QString Annotation::Window::title() const
{
    return m_title;
}

void Annotation::Window::setSummary( const QString &summary )
{
    m_summary = summary;
}

QString Annotation::Window::summary() const
{
    return m_summary;
}

void Annotation::Window::setText( const QString &text )
{
    m_text = text;
}

QString Annotation::Window::text() const
{
    return m_text;
}



Annotation::Revision::Revision()
    : m_annotation( 0 ), m_scope( Reply ), m_type( None )
{
}

void Annotation::Revision::setAnnotation( Annotation *annotation )
{
    m_annotation = annotation;
}

Annotation *Annotation::Revision::annotation() const
{
    return m_annotation;
}

void Annotation::Revision::setScope( RevisionScope scope )
{
    m_scope = scope;
}

Annotation::RevisionScope Annotation::Revision::scope() const
{
    return m_scope;
}

void Annotation::Revision::setType( RevisionType type )
{
    m_type = type;
}

Annotation::RevisionType Annotation::Revision::type() const
{
    return m_type;
}



Annotation::Annotation()
    : m_flags( 0 )
{
}

Annotation::~Annotation()
{
    // delete all children revisions
    if ( m_revisions.isEmpty() )
        return;

    QLinkedList< Revision >::iterator it = m_revisions.begin(), end = m_revisions.end();
    for ( ; it != end; ++it )
        delete (*it).annotation();
}

Annotation::Annotation( const QDomNode & annNode )
    : m_flags( 0 )
{
    // get the [base] element of the annotation node
    QDomElement e = AnnotationUtils::findChildElement( annNode, "base" );
    if ( e.isNull() )
        return;

    // parse -contents- attributes
    if ( e.hasAttribute( "author" ) )
        m_author = e.attribute( "author" );
    if ( e.hasAttribute( "contents" ) )
        m_contents = e.attribute( "contents" );
    if ( e.hasAttribute( "uniqueName" ) )
        m_uniqueName = e.attribute( "uniqueName" );
    if ( e.hasAttribute( "modifyDate" ) )
        m_modifyDate = QDateTime::fromString( e.attribute("modifyDate"), Qt::ISODate );
    if ( e.hasAttribute( "creationDate" ) )
        m_creationDate = QDateTime::fromString( e.attribute("creationDate"), Qt::ISODate );

    // parse -other- attributes
    if ( e.hasAttribute( "flags" ) )
        m_flags = e.attribute( "flags" ).toInt();
    if ( e.hasAttribute( "color" ) )
        m_style.setColor( QColor( e.attribute( "color" ) ) );
    if ( e.hasAttribute( "opacity" ) )
        m_style.setOpacity( e.attribute( "opacity" ).toDouble() );

    // parse -the-subnodes- (describing Style, Window, Revision(s) structures)
    // Note: all subnodes if present must be 'attributes complete'
    QDomNode eSubNode = e.firstChild();
    while ( eSubNode.isElement() )
    {
        QDomElement ee = eSubNode.toElement();
        eSubNode = eSubNode.nextSibling();

        // parse boundary
        if ( ee.tagName() == "boundary" )
        {
            m_boundary=NormalizedRect(ee.attribute( "l" ).toDouble(),
                ee.attribute( "t" ).toDouble(),
                ee.attribute( "r" ).toDouble(),
                ee.attribute( "b" ).toDouble());
        }
        // parse penStyle if not default
        else if ( ee.tagName() == "penStyle" )
        {
            m_style.setWidth( ee.attribute( "width" ).toDouble() );
            m_style.setLineStyle( (LineStyle)ee.attribute( "style" ).toInt() );
            m_style.setXCorners( ee.attribute( "xcr" ).toDouble() );
            m_style.setYCorners( ee.attribute( "ycr" ).toDouble() );
            m_style.setMarks( ee.attribute( "marks" ).toInt() );
            m_style.setSpaces( ee.attribute( "spaces" ).toInt() );
        }
        // parse effectStyle if not default
        else if ( ee.tagName() == "penEffect" )
        {
            m_style.setLineEffect( (LineEffect)ee.attribute( "effect" ).toInt() );
            m_style.setEffectIntensity( ee.attribute( "intensity" ).toDouble() );
        }
        // parse window if present
        else if ( ee.tagName() == "window" )
        {
            m_window.setFlags( ee.attribute( "flags" ).toInt() );
            m_window.setTopLeft( NormalizedPoint( ee.attribute( "top" ).toDouble(),
                                                  ee.attribute( "left" ).toDouble() ) );
            m_window.setWidth( ee.attribute( "width" ).toInt() );
            m_window.setHeight( ee.attribute( "height" ).toInt() );
            m_window.setTitle( ee.attribute( "title" ) );
            m_window.setSummary( ee.attribute( "summary" ) );
            // parse window subnodes
            QDomNode winNode = ee.firstChild();
            for ( ; winNode.isElement(); winNode = winNode.nextSibling() )
            {
                QDomElement winElement = winNode.toElement();
                if ( winElement.tagName() == "text" )
                    m_window.setText( winElement.firstChild().toCDATASection().data() );
            }
        }
    }

    // get the [revisions] element of the annotation node
    QDomNode revNode = annNode.firstChild();
    for ( ; revNode.isElement(); revNode = revNode.nextSibling() )
    {
        QDomElement revElement = revNode.toElement();
        if ( revElement.tagName() != "revision" )
            continue;

        // compile the Revision structure crating annotation
        Revision revision;
        revision.setScope( (RevisionScope)revElement.attribute( "revScope" ).toInt() );
        revision.setType( (RevisionType)revElement.attribute( "revType" ).toInt() );
        revision.setAnnotation( AnnotationUtils::createAnnotation( revElement ) );

        // if annotation is valid, add revision to internal list
        if ( revision.annotation() );
            m_revisions.append( revision );
    }

    m_transformedBoundary = m_boundary;
}

void Annotation::setAuthor( const QString &author )
{
    m_author = author;
}

QString Annotation::author() const
{
    return m_author;
}

void Annotation::setContents( const QString &contents )
{
    m_contents = contents;
}

QString Annotation::contents() const
{
    return m_contents;
}

void Annotation::setUniqueName( const QString &name )
{
    m_uniqueName = name;
}

QString Annotation::uniqueName() const
{
    return m_uniqueName;
}

void Annotation::setModificationDate( const QDateTime &date )
{
    m_modifyDate = date;
}

QDateTime Annotation::modificationDate() const
{
    return m_modifyDate;
}

void Annotation::setCreationDate( const QDateTime &date )
{
    m_creationDate = date;
}

QDateTime Annotation::creationDate() const
{
    return m_creationDate;
}

void Annotation::setFlags( int flags )
{
    m_flags = flags;
}

int Annotation::flags() const
{
    return m_flags;
}

void Annotation::setBoundingRectangle( const NormalizedRect &rectangle )
{
    m_boundary = rectangle;
}

NormalizedRect Annotation::boundingRectangle() const
{
    return m_boundary;
}

NormalizedRect Annotation::transformedBoundingRectangle() const
{
    return m_transformedBoundary;
}

Annotation::SubType Annotation::subType() const
{
    return A_BASE;
}

Annotation::Style & Annotation::style()
{
    return m_style;
}

const Annotation::Style & Annotation::style() const
{
    return m_style;
}

Annotation::Window & Annotation::window()
{
    return m_window;
}

const Annotation::Window & Annotation::window() const
{
    return m_window;
}

QLinkedList< Annotation::Revision > & Annotation::revisions()
{
    return m_revisions;
}

const QLinkedList< Annotation::Revision > & Annotation::revisions() const
{
    return m_revisions;
}

void Annotation::store( QDomNode & annNode, QDomDocument & document ) const
{
    // create [base] element of the annotation node
    QDomElement e = document.createElement( "base" );
    annNode.appendChild( e );

    // store -contents- attributes
    if ( !m_author.isEmpty() )
        e.setAttribute( "author", m_author );
    if ( !m_contents.isEmpty() )
        e.setAttribute( "contents", m_contents );
    if ( !m_uniqueName.isEmpty() )
        e.setAttribute( "uniqueName", m_uniqueName );
    if ( m_modifyDate.isValid() )
        e.setAttribute( "modifyDate", m_modifyDate.toString(Qt::ISODate) );
    if ( m_creationDate.isValid() )
        e.setAttribute( "creationDate", m_creationDate.toString(Qt::ISODate) );

    // store -other- attributes
    if ( m_flags )
        e.setAttribute( "flags", m_flags );
    if ( m_style.color().isValid() )
        e.setAttribute( "color", m_style.color().name() );
    if ( m_style.opacity() != 1.0 )
        e.setAttribute( "opacity", m_style.opacity() );

    // Sub-Node-1 - boundary
    QDomElement bE = document.createElement( "boundary" );
    e.appendChild( bE );
    bE.setAttribute( "l", (double)m_boundary.left );
    bE.setAttribute( "t", (double)m_boundary.top );
    bE.setAttribute( "r", (double)m_boundary.right );
    bE.setAttribute( "b", (double)m_boundary.bottom );

    // Sub-Node-2 - penStyle
    if ( m_style.width() != 1 || m_style.lineStyle() != Solid || m_style.xCorners() != 0 ||
         m_style.yCorners() != 0.0 || m_style.marks() != 3 || m_style.spaces() != 0 )
    {
        QDomElement psE = document.createElement( "penStyle" );
        e.appendChild( psE );
        psE.setAttribute( "width", m_style.width() );
        psE.setAttribute( "style", (int)m_style.lineStyle() );
        psE.setAttribute( "xcr", m_style.xCorners() );
        psE.setAttribute( "ycr", m_style.yCorners() );
        psE.setAttribute( "marks", m_style.marks() );
        psE.setAttribute( "spaces", m_style.spaces() );
    }

    // Sub-Node-3 - penEffect
    if ( m_style.lineEffect() != NoEffect || m_style.effectIntensity() != 1.0 )
    {
        QDomElement peE = document.createElement( "penEffect" );
        e.appendChild( peE );
        peE.setAttribute( "effect", (int)m_style.lineEffect() );
        peE.setAttribute( "intensity", m_style.effectIntensity() );
    }

    // Sub-Node-4 - window
    if ( m_window.flags() != -1 || !m_window.title().isEmpty() ||
         !m_window.summary().isEmpty() || !m_window.text().isEmpty() )
    {
        QDomElement wE = document.createElement( "window" );
        e.appendChild( wE );
        wE.setAttribute( "flags", m_window.flags() );
        wE.setAttribute( "top", m_window.topLeft().x );
        wE.setAttribute( "left", m_window.topLeft().y );
        wE.setAttribute( "width", m_window.width() );
        wE.setAttribute( "height", m_window.height() );
        wE.setAttribute( "title", m_window.title() );
        wE.setAttribute( "summary", m_window.summary() );
        // store window.text as a subnode, because we need escaped data
        if ( !m_window.text().isEmpty() )
        {
            QDomElement escapedText = document.createElement( "text" );
            wE.appendChild( escapedText );
            QDomCDATASection textCData = document.createCDATASection( m_window.text() );
            escapedText.appendChild( textCData );
        }
    }

    // create [revision] element of the annotation node (if any)
    if ( m_revisions.isEmpty() )
        return;

    // add all revisions as children of revisions element
    QLinkedList< Revision >::const_iterator it = m_revisions.begin(), end = m_revisions.end();
    for ( ; it != end; ++it )
    {
        // create revision element
        const Revision & revision = *it;
        QDomElement r = document.createElement( "revision" );
        annNode.appendChild( r );
        // set element attributes
        r.setAttribute( "revScope", (int)revision.scope() );
        r.setAttribute( "revType", (int)revision.type() );
        // use revision as the annotation element, so fill it up
        AnnotationUtils::storeAnnotation( revision.annotation(), r, document );
    }
}

void Annotation::transform( const QMatrix &matrix )
{
    m_transformedBoundary = m_boundary;
    m_transformedBoundary.transform( matrix );
}

//END Annotation implementation


/** TextAnnotation [Annotation] */
/*
  The default textIcon for text annotation is Note as the PDF Reference says
*/
TextAnnotation::TextAnnotation()
    : Annotation(),
      m_textType( Linked ), m_textIcon( "Note" ),
      m_inplaceAlign( 0 ), m_inplaceIntent( Unknown )
{
}

TextAnnotation::TextAnnotation( const QDomNode & node )
    : Annotation( node ),
      m_textType( Linked ), m_textIcon( "Note" ),
      m_inplaceAlign( 0 ), m_inplaceIntent( Unknown )
{
    // loop through the whole children looking for a 'text' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "text" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "type" ) )
            m_textType = (TextAnnotation::TextType)e.attribute( "type" ).toInt();
        if ( e.hasAttribute( "icon" ) )
            m_textIcon = e.attribute( "icon" );
        if ( e.hasAttribute( "font" ) )
            m_textFont.fromString( e.attribute( "font" ) );
        if ( e.hasAttribute( "align" ) )
            m_inplaceAlign = e.attribute( "align" ).toInt();
        if ( e.hasAttribute( "intent" ) )
            m_inplaceIntent = (TextAnnotation::InplaceIntent)e.attribute( "intent" ).toInt();

        // parse the subnodes
        QDomNode eSubNode = e.firstChild();
        while ( eSubNode.isElement() )
        {
            QDomElement ee = eSubNode.toElement();
            eSubNode = eSubNode.nextSibling();

            if ( ee.tagName() == "escapedText" )
            {
                m_inplaceText = ee.firstChild().toCDATASection().data();
            }
            else if ( ee.tagName() == "callout" )
            {
                m_inplaceCallout[0].x = ee.attribute( "ax" ).toDouble();
                m_inplaceCallout[0].y = ee.attribute( "ay" ).toDouble();
                m_inplaceCallout[1].x = ee.attribute( "bx" ).toDouble();
                m_inplaceCallout[1].y = ee.attribute( "by" ).toDouble();
                m_inplaceCallout[2].x = ee.attribute( "cx" ).toDouble();
                m_inplaceCallout[2].y = ee.attribute( "cy" ).toDouble();
            }
        }

        // loading complete
        break;
    }

    for ( int i = 0; i < 3; ++i )
        m_transformedInplaceCallout[i] = m_inplaceCallout[i];
}

TextAnnotation::~TextAnnotation()
{
}

void TextAnnotation::setTextType( TextType textType )
{
    m_textType = textType;
}

TextAnnotation::TextType TextAnnotation::textType() const
{
    return m_textType;
}

void TextAnnotation::setTextIcon( const QString &icon )
{
    m_textIcon = icon;
}

QString TextAnnotation::textIcon() const
{
    return m_textIcon;
}

void TextAnnotation::setTextFont( const QFont &font )
{
    m_textFont = font;
}

QFont TextAnnotation::textFont() const
{
    return m_textFont;
}

void TextAnnotation::setInplaceAlignment( int alignment )
{
    m_inplaceAlign = alignment;
}

int TextAnnotation::inplaceAlignment() const
{
    return m_inplaceAlign;
}

void TextAnnotation::setInplaceText( const QString &text )
{
    m_inplaceText = text;
}

QString TextAnnotation::inplaceText() const
{
    return m_inplaceText;
}

void TextAnnotation::setInplaceCallout( const NormalizedPoint &point, int index )
{
    if ( index < 0 || index > 2 )
        return;

    m_inplaceCallout[ index ] = point;
}

NormalizedPoint TextAnnotation::inplaceCallout( int index ) const
{
    if ( index < 0 || index > 2 )
        return NormalizedPoint();

    return m_inplaceCallout[ index ];
}

NormalizedPoint TextAnnotation::transformedInplaceCallout( int index ) const
{
    if ( index < 0 || index > 2 )
        return NormalizedPoint();

    return m_transformedInplaceCallout[ index ];
}

void TextAnnotation::setInplaceIntent( InplaceIntent intent )
{
    m_inplaceIntent = intent;
}

TextAnnotation::InplaceIntent TextAnnotation::inplaceIntent() const
{
    return m_inplaceIntent;
}

Annotation::SubType TextAnnotation::subType() const
{
    return AText;
}

void TextAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [text] element
    QDomElement textElement = document.createElement( "text" );
    node.appendChild( textElement );

    // store the optional attributes
    if ( m_textType != Linked )
        textElement.setAttribute( "type", (int)m_textType );
    if ( m_textIcon != "Comment" )
        textElement.setAttribute( "icon", m_textIcon );
    if ( m_textFont != QApplication::font() )
        textElement.setAttribute( "font", m_textFont.toString() );
    if ( m_inplaceAlign )
        textElement.setAttribute( "align", m_inplaceAlign );
    if ( m_inplaceIntent != Unknown )
        textElement.setAttribute( "intent", (int)m_inplaceIntent );

    // Sub-Node-1 - escapedText
    if ( !m_inplaceText.isEmpty() )
    {
        QDomElement escapedText = document.createElement( "escapedText" );
        textElement.appendChild( escapedText );
        QDomCDATASection textCData = document.createCDATASection( m_inplaceText );
        escapedText.appendChild( textCData );
    }

    // Sub-Node-2 - callout
    if ( m_inplaceCallout[0].x != 0.0 )
    {
        QDomElement calloutElement = document.createElement( "callout" );
        textElement.appendChild( calloutElement );
        calloutElement.setAttribute( "ax", m_inplaceCallout[0].x );
        calloutElement.setAttribute( "ay", m_inplaceCallout[0].y );
        calloutElement.setAttribute( "bx", m_inplaceCallout[1].x );
        calloutElement.setAttribute( "by", m_inplaceCallout[1].y );
        calloutElement.setAttribute( "cx", m_inplaceCallout[2].x );
        calloutElement.setAttribute( "cy", m_inplaceCallout[2].y );
    }
}

void TextAnnotation::transform( const QMatrix &matrix )
{
    Annotation::transform( matrix );

    for ( int i = 0; i < 3; ++i ) {
        m_transformedInplaceCallout[i] = m_inplaceCallout[i];
        m_transformedInplaceCallout[i].transform( matrix );
    }
}

/** LineAnnotation [Annotation] */

LineAnnotation::LineAnnotation()
    : Annotation(),
      m_lineStartStyle( None ), m_lineEndStyle( None ),
      m_lineClosed( false ), m_lineLeadingFwdPt( 0 ), m_lineLeadingBackPt( 0 ),
      m_lineShowCaption( false ), m_lineIntent( Unknown )
{
}

LineAnnotation::LineAnnotation( const QDomNode & node )
    : Annotation( node ),
      m_lineStartStyle( None ), m_lineEndStyle( None ),
      m_lineClosed( false ), m_lineLeadingFwdPt( 0 ), m_lineLeadingBackPt( 0 ),
      m_lineShowCaption( false ), m_lineIntent( Unknown )
{
    // loop through the whole children looking for a 'line' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "line" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "startStyle" ) )
            m_lineStartStyle = (LineAnnotation::TermStyle)e.attribute( "startStyle" ).toInt();
        if ( e.hasAttribute( "endStyle" ) )
            m_lineEndStyle = (LineAnnotation::TermStyle)e.attribute( "endStyle" ).toInt();
        if ( e.hasAttribute( "closed" ) )
            m_lineClosed = e.attribute( "closed" ).toInt();
        if ( e.hasAttribute( "innerColor" ) )
            m_lineInnerColor = QColor( e.attribute( "innerColor" ) );
        if ( e.hasAttribute( "leadFwd" ) )
            m_lineLeadingFwdPt = e.attribute( "leadFwd" ).toDouble();
        if ( e.hasAttribute( "leadBack" ) )
            m_lineLeadingBackPt = e.attribute( "leadBack" ).toDouble();
        if ( e.hasAttribute( "showCaption" ) )
            m_lineShowCaption = e.attribute( "showCaption" ).toInt();
        if ( e.hasAttribute( "intent" ) )
            m_lineIntent = (LineAnnotation::LineIntent)e.attribute( "intent" ).toInt();

        // parse all 'point' subnodes
        QDomNode pointNode = e.firstChild();
        while ( pointNode.isElement() )
        {
            QDomElement pe = pointNode.toElement();
            pointNode = pointNode.nextSibling();

            if ( pe.tagName() != "point" )
                continue;

            NormalizedPoint p;
            p.x = pe.attribute( "x", "0.0" ).toDouble();
            p.y = pe.attribute( "y", "0.0" ).toDouble();
            m_linePoints.append( p );
        }

        // loading complete
        break;
    }

    m_transformedLinePoints = m_linePoints;
}

LineAnnotation::~LineAnnotation()
{
}

void LineAnnotation::setLinePoints( const QLinkedList<NormalizedPoint> &points )
{
    m_linePoints = points;
}

QLinkedList<NormalizedPoint> LineAnnotation::linePoints() const
{
    return m_linePoints;
}

QLinkedList<NormalizedPoint> LineAnnotation::transformedLinePoints() const
{
    return m_transformedLinePoints;
}

void LineAnnotation::setLineStartStyle( TermStyle style )
{
    m_lineStartStyle = style;
}

LineAnnotation::TermStyle LineAnnotation::lineStartStyle() const
{
    return m_lineStartStyle;
}

void LineAnnotation::setLineEndStyle( TermStyle style )
{
    m_lineEndStyle = style;
}

LineAnnotation::TermStyle LineAnnotation::lineEndStyle() const
{
    return m_lineEndStyle;
}

void LineAnnotation::setLineClosed( bool closed )
{
    m_lineClosed = closed;
}

bool LineAnnotation::lineClosed() const
{
    return m_lineClosed;
}

void LineAnnotation::setLineInnerColor( const QColor &color )
{
    m_lineInnerColor = color;
}

QColor LineAnnotation::lineInnerColor() const
{
    return m_lineInnerColor;
}

void LineAnnotation::setLineLeadingForwardPoint( double point )
{
    m_lineLeadingFwdPt = point;
}

double LineAnnotation::lineLeadingForwardPoint() const
{
    return m_lineLeadingFwdPt;
}

void LineAnnotation::setLineLeadingBackwardPoint( double point )
{
    m_lineLeadingBackPt = point;
}

double LineAnnotation::lineLeadingBackwardPoint() const
{
    return m_lineLeadingBackPt;
}

void LineAnnotation::setShowCaption( bool show )
{
    m_lineShowCaption = show;
}

bool LineAnnotation::showCaption() const
{
    return m_lineShowCaption;
}

void LineAnnotation::setLineIntent( LineIntent intent )
{
    m_lineIntent = intent;
}

LineAnnotation::LineIntent LineAnnotation::lineIntent() const
{
    return m_lineIntent;
}

Annotation::SubType LineAnnotation::subType() const
{
    return ALine;
}

void LineAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [line] element
    QDomElement lineElement = document.createElement( "line" );
    node.appendChild( lineElement );

    // store the attributes
    if ( m_lineStartStyle != None )
        lineElement.setAttribute( "startStyle", (int)m_lineStartStyle );
    if ( m_lineEndStyle != None )
        lineElement.setAttribute( "endStyle", (int)m_lineEndStyle );
    if ( m_lineClosed )
        lineElement.setAttribute( "closed", m_lineClosed );
    if ( m_lineInnerColor.isValid() )
        lineElement.setAttribute( "innerColor", m_lineInnerColor.name() );
    if ( m_lineLeadingFwdPt != 0.0 )
        lineElement.setAttribute( "leadFwd", m_lineLeadingFwdPt );
    if ( m_lineLeadingBackPt != 0.0 )
        lineElement.setAttribute( "leadBack", m_lineLeadingBackPt );
    if ( m_lineShowCaption )
        lineElement.setAttribute( "showCaption", m_lineShowCaption );
    if ( m_lineIntent != Unknown )
        lineElement.setAttribute( "intent", m_lineIntent );

    // append the list of points
    int points = m_linePoints.count();
    if ( points > 1 )
    {
        QLinkedList<NormalizedPoint>::const_iterator it = m_linePoints.begin(), end = m_linePoints.end();
        while ( it != end )
        {
            const NormalizedPoint & p = *it;
            QDomElement pElement = document.createElement( "point" );
            lineElement.appendChild( pElement );
            pElement.setAttribute( "x", p.x );
            pElement.setAttribute( "y", p.y );
            it++; //to avoid loop
        }
    }
}

void LineAnnotation::transform( const QMatrix &matrix )
{
    Annotation::transform( matrix );

    m_transformedLinePoints = m_linePoints;

    QMutableLinkedListIterator<NormalizedPoint> it( m_transformedLinePoints );
    while ( it.hasNext() )
        it.next().transform( matrix );
}

/** GeomAnnotation [Annotation] */

GeomAnnotation::GeomAnnotation()
    : Annotation(),
      m_geomType( InscribedSquare ), m_geomWidthPt( 18 )
{}

GeomAnnotation::GeomAnnotation( const QDomNode & node )
    : Annotation( node ),
      m_geomType( InscribedSquare ), m_geomWidthPt( 18 )
{
    // loop through the whole children looking for a 'geom' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "geom" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "type" ) )
            m_geomType = (GeomAnnotation::GeomType)e.attribute( "type" ).toInt();
        if ( e.hasAttribute( "color" ) )
            m_geomInnerColor = QColor( e.attribute( "color" ) );
        if ( e.hasAttribute( "width" ) )
            m_geomWidthPt = e.attribute( "width" ).toInt();

        // loading complete
        break;
    }
}

GeomAnnotation::~GeomAnnotation()
{
}

void GeomAnnotation::setGeometricalType( GeomType type )
{
    m_geomType = type;
}

GeomAnnotation::GeomType GeomAnnotation::geometricalType() const
{
    return m_geomType;
}

void GeomAnnotation::setGeometricalInnerColor( const QColor &color )
{
    m_geomInnerColor = color;
}

QColor GeomAnnotation::geometricalInnerColor() const
{
    return m_geomInnerColor;
}

void GeomAnnotation::setGeometricalPointWidth( int width )
{
    m_geomWidthPt = width;
}

int GeomAnnotation::geometricalPointWidth() const
{
    return m_geomWidthPt;
}

Annotation::SubType GeomAnnotation::subType() const
{
    return AGeom;
}

void GeomAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [geom] element
    QDomElement geomElement = document.createElement( "geom" );
    node.appendChild( geomElement );

    // append the optional attributes
    if ( m_geomType != InscribedSquare )
        geomElement.setAttribute( "type", (int)m_geomType );
    if ( m_geomInnerColor.isValid() )
        geomElement.setAttribute( "color", m_geomInnerColor.name() );
    if ( m_geomWidthPt != 18 )
        geomElement.setAttribute( "width", m_geomWidthPt );
}

void GeomAnnotation::transform( const QMatrix &matrix )
{
    Annotation::transform( matrix );
}

/** HighlightAnnotation [Annotation] */

HighlightAnnotation::Quad::Quad()
{
}

void HighlightAnnotation::Quad::setPoint( const NormalizedPoint &point, int index )
{
    if ( index < 0 || index > 3 )
        return;

    m_points[ index ] = point;
}

NormalizedPoint HighlightAnnotation::Quad::point( int index ) const
{
    if ( index < 0 || index > 3 )
        return NormalizedPoint();

    return m_points[ index ];
}

NormalizedPoint HighlightAnnotation::Quad::transformedPoint( int index ) const
{
    if ( index < 0 || index > 3 )
        return NormalizedPoint();

    return m_transformedPoints[ index ];
}

void HighlightAnnotation::Quad::setCapStart( bool value )
{
    m_capStart = value;
}

bool HighlightAnnotation::Quad::capStart() const
{
    return m_capStart;
}

void HighlightAnnotation::Quad::setCapEnd( bool value )
{
    m_capEnd = value;
}

bool HighlightAnnotation::Quad::capEnd() const
{
    return m_capEnd;
}

void HighlightAnnotation::Quad::setFeather( double width )
{
    m_feather = width;
}

double HighlightAnnotation::Quad::feather() const
{
    return m_feather;
}

void HighlightAnnotation::Quad::transform( const QMatrix &matrix )
{
    for ( int i = 0; i < 4; ++i ) {
        m_transformedPoints[ i ] = m_points[ i ];
        m_transformedPoints[ i ].transform( matrix );
    }
}

HighlightAnnotation::HighlightAnnotation()
    : Annotation(),
      m_highlightType( Highlight )
{
}

HighlightAnnotation::HighlightAnnotation( const QDomNode & node )
    : Annotation( node ),
      m_highlightType( Highlight )
{
    // loop through the whole children looking for a 'hl' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "hl" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "type" ) )
            m_highlightType = (HighlightAnnotation::HighlightType)e.attribute( "type" ).toInt();

        // parse all 'quad' subnodes
        QDomNode quadNode = e.firstChild();
        for ( ; quadNode.isElement(); quadNode = quadNode.nextSibling() )
        {
            QDomElement qe = quadNode.toElement();
            if ( qe.tagName() != "quad" )
                continue;

            Quad q;
            q.setPoint( NormalizedPoint( qe.attribute( "ax", "0.0" ).toDouble(), qe.attribute( "ay", "0.0" ).toDouble() ), 0 );
            q.setPoint( NormalizedPoint( qe.attribute( "bx", "0.0" ).toDouble(), qe.attribute( "by", "0.0" ).toDouble() ), 1 );
            q.setPoint( NormalizedPoint( qe.attribute( "cx", "0.0" ).toDouble(), qe.attribute( "cy", "0.0" ).toDouble() ), 2 );
            q.setPoint( NormalizedPoint( qe.attribute( "dx", "0.0" ).toDouble(), qe.attribute( "dy", "0.0" ).toDouble() ), 3 );
            q.setCapStart( qe.hasAttribute( "start" ) );
            q.setCapEnd( qe.hasAttribute( "end" ) );
            q.setFeather( qe.attribute( "feather", "0.1" ).toDouble() );

            q.transform( QMatrix() );

            m_highlightQuads.append( q );
        }

        // loading complete
        break;
    }
}

HighlightAnnotation::~HighlightAnnotation()
{
}

void HighlightAnnotation::setHighlightType( HighlightType type )
{
    m_highlightType = type;
}

HighlightAnnotation::HighlightType HighlightAnnotation::highlightType() const
{
    return m_highlightType;
}

QList< HighlightAnnotation::Quad > & HighlightAnnotation::highlightQuads()
{
    return m_highlightQuads;
}

void HighlightAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [hl] element
    QDomElement hlElement = document.createElement( "hl" );
    node.appendChild( hlElement );

    // append the optional attributes
    if ( m_highlightType != Highlight )
        hlElement.setAttribute( "type", (int)m_highlightType );
    if ( m_highlightQuads.count() < 1 )
        return;
    // append highlight quads, all children describe quads
    QList< Quad >::const_iterator it = m_highlightQuads.begin(), end = m_highlightQuads.end();
    for ( ; it != end; ++it )
    {
        QDomElement quadElement = document.createElement( "quad" );
        hlElement.appendChild( quadElement );
        const Quad & q = *it;
        quadElement.setAttribute( "ax", q.point( 0 ).x );
        quadElement.setAttribute( "ay", q.point( 0 ).y );
        quadElement.setAttribute( "bx", q.point( 1 ).x );
        quadElement.setAttribute( "by", q.point( 1 ).y );
        quadElement.setAttribute( "cx", q.point( 2 ).x );
        quadElement.setAttribute( "cy", q.point( 2 ).y );
        quadElement.setAttribute( "dx", q.point( 3 ).x );
        quadElement.setAttribute( "dy", q.point( 3 ).y );
        if ( q.capStart() )
            quadElement.setAttribute( "start", 1 );
        if ( q.capEnd() )
            quadElement.setAttribute( "end", 1 );
        quadElement.setAttribute( "feather", q.feather() );
    }
}

Annotation::SubType HighlightAnnotation::subType() const
{
    return AHighlight;
}

void HighlightAnnotation::transform( const QMatrix &matrix )
{
    Annotation::transform( matrix );

    QMutableListIterator<Quad> it( m_highlightQuads );
    while ( it.hasNext() )
        it.next().transform( matrix );
}

/** StampAnnotation [Annotation] */

StampAnnotation::StampAnnotation()
    : Annotation(),
      m_stampIconName( "Draft" )
{
}

StampAnnotation::StampAnnotation( const QDomNode & node )
    : Annotation( node ),
      m_stampIconName( "Draft" )
{
    // loop through the whole children looking for a 'stamp' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "stamp" )
            continue;

        // parse the attributes
        if ( e.hasAttribute( "icon" ) )
            m_stampIconName = e.attribute( "icon" );

        // loading complete
        break;
    }
}

StampAnnotation::~StampAnnotation()
{
}

void StampAnnotation::setStampIconName( const QString &name )
{
    m_stampIconName = name;
}

QString StampAnnotation::stampIconName() const
{
    return m_stampIconName;
}

Annotation::SubType StampAnnotation::subType() const
{
    return AStamp;
}

void StampAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [stamp] element
    QDomElement stampElement = document.createElement( "stamp" );
    node.appendChild( stampElement );

    // append the optional attributes
    if ( m_stampIconName != "Draft" )
        stampElement.setAttribute( "icon", m_stampIconName );
}

void StampAnnotation::transform( const QMatrix &matrix )
{
    Annotation::transform( matrix );
}

/** InkAnnotation [Annotation] */

InkAnnotation::InkAnnotation()
    : Annotation()
{
}

InkAnnotation::InkAnnotation( const QDomNode & node )
    : Annotation( node )
{
    // loop through the whole children looking for a 'ink' element
    QDomNode subNode = node.firstChild();
    while( subNode.isElement() )
    {
        QDomElement e = subNode.toElement();
        subNode = subNode.nextSibling();
        if ( e.tagName() != "ink" )
            continue;

        // parse the 'path' subnodes
        QDomNode pathNode = e.firstChild();
        while ( pathNode.isElement() )
        {
            QDomElement pathElement = pathNode.toElement();
            pathNode = pathNode.nextSibling();

            if ( pathElement.tagName() != "path" )
                continue;

            // build each path parsing 'point' subnodes
            QLinkedList<NormalizedPoint> path;
            QDomNode pointNode = pathElement.firstChild();
            while ( pointNode.isElement() )
            {
                QDomElement pointElement = pointNode.toElement();
                pointNode = pointNode.nextSibling();

                if ( pointElement.tagName() != "point" )
                    continue;

                NormalizedPoint p;
                p.x = pointElement.attribute( "x", "0.0" ).toDouble();
                p.y = pointElement.attribute( "y", "0.0" ).toDouble();
                path.append( p );
            }

            // add the path to the path list if it contains at least 2 nodes
            if ( path.count() >= 2 )
                m_inkPaths.append( path );
        }

        // loading complete
        break;
    }

    m_transformedInkPaths = m_inkPaths;
}

InkAnnotation::~InkAnnotation()
{
}

void InkAnnotation::setInkPaths( const QList< QLinkedList<NormalizedPoint> > &paths )
{
    m_inkPaths = paths;
}

QList< QLinkedList<NormalizedPoint> > InkAnnotation::inkPaths() const
{
    return m_inkPaths;
}

QList< QLinkedList<NormalizedPoint> > InkAnnotation::transformedInkPaths() const
{
    return m_transformedInkPaths;
}

Annotation::SubType InkAnnotation::subType() const
{
    return AInk;
}

void InkAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [ink] element
    QDomElement inkElement = document.createElement( "ink" );
    node.appendChild( inkElement );

    // append the optional attributes
    if ( m_inkPaths.count() < 1 )
        return;

    QList< QLinkedList<NormalizedPoint> >::const_iterator pIt = m_inkPaths.begin(), pEnd = m_inkPaths.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        QDomElement pathElement = document.createElement( "path" );
        inkElement.appendChild( pathElement );
        const QLinkedList<NormalizedPoint> & path = *pIt;
        QLinkedList<NormalizedPoint>::const_iterator iIt = path.begin(), iEnd = path.end();
        for ( ; iIt != iEnd; ++iIt )
        {
            const NormalizedPoint & point = *iIt;
            QDomElement pointElement = document.createElement( "point" );
            pathElement.appendChild( pointElement );
            pointElement.setAttribute( "x", point.x );
            pointElement.setAttribute( "y", point.y );
        }
    }
}

void InkAnnotation::transform( const QMatrix &matrix )
{
    Annotation::transform( matrix );

    m_transformedInkPaths = m_inkPaths;

    for ( int i = 0; i < m_transformedInkPaths.count(); ++i ) {
        QMutableLinkedListIterator<NormalizedPoint> it( m_transformedInkPaths[ i ] );
        while ( it.hasNext() )
            it.next().transform( matrix );
    }
}
