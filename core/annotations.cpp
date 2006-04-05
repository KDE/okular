/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qdom.h>
#include <qcolor.h>
#include <qapplication.h>
#include <kdebug.h>

// local includes
#include "annotations.h"


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
//END AnnotationUtils implementation


//BEGIN Annotation implementation
Annotation::Style::Style()
    : opacity( 1.0 ), width( 1.0 ), style( Solid ), xCorners( 0.0 ),
    yCorners( 0.0 ), marks( 3 ), spaces( 0 ), effect( NoEffect ),
    effectIntensity( 1.0 ) {}

Annotation::Window::Window()
    : flags( -1 ), width( 0 ), height( 0 ) {}

Annotation::Revision::Revision()
    : annotation( 0 ), scope( Reply ), type( None ) {}

Annotation::Annotation()
    : flags( 0 ) {}

Annotation::~Annotation()
{
    // delete all children revisions
    if ( revisions.isEmpty() )
        return;
    QLinkedList< Revision >::iterator it = revisions.begin(), end = revisions.end();
    for ( ; it != end; ++it )
        delete (*it).annotation;
}

Annotation::Annotation( const QDomNode & annNode )
    : flags( 0 )
{
    // get the [base] element of the annotation node
    QDomElement e = AnnotationUtils::findChildElement( annNode, "base" );
    if ( e.isNull() )
        return;

    // parse -contents- attributes
    if ( e.hasAttribute( "author" ) )
        author = e.attribute( "author" );
    if ( e.hasAttribute( "contents" ) )
        contents = e.attribute( "contents" );
    if ( e.hasAttribute( "uniqueName" ) )
        uniqueName = e.attribute( "uniqueName" );
    if ( e.hasAttribute( "modifyDate" ) )
        modifyDate = QDateTime::fromString( e.attribute( "modifyDate" ) );
    if ( e.hasAttribute( "creationDate" ) )
        creationDate = QDateTime::fromString( e.attribute( "creationDate" ) );

    // parse -other- attributes
    if ( e.hasAttribute( "flags" ) )
        flags = e.attribute( "flags" ).toInt();
    if ( e.hasAttribute( "color" ) )
        style.color = QColor( e.attribute( "color" ) );
    if ( e.hasAttribute( "opacity" ) )
        style.opacity = e.attribute( "opacity" ).toDouble();

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
            boundary=NormalizedRect(ee.attribute( "l" ).toDouble(),
                ee.attribute( "t" ).toDouble(),
                ee.attribute( "r" ).toDouble(),
                ee.attribute( "b" ).toDouble());
        }
        // parse penStyle if not default
        else if ( ee.tagName() == "penStyle" )
        {
            style.width = ee.attribute( "width" ).toDouble();
            style.style = (LineStyle)ee.attribute( "style" ).toInt();
            style.xCorners = ee.attribute( "xcr" ).toDouble();
            style.yCorners = ee.attribute( "ycr" ).toDouble();
            style.marks = ee.attribute( "marks" ).toInt();
            style.spaces = ee.attribute( "spaces" ).toInt();
        }
        // parse effectStyle if not default
        else if ( ee.tagName() == "penEffect" )
        {
            style.effect = (LineEffect)ee.attribute( "effect" ).toInt();
            style.effectIntensity = ee.attribute( "intensity" ).toDouble();
        }
        // parse window if present
        else if ( ee.tagName() == "window" )
        {
            window.flags = ee.attribute( "flags" ).toInt();
            window.topLeft.x = ee.attribute( "top" ).toDouble();
            window.topLeft.y = ee.attribute( "left" ).toDouble();
            window.width = ee.attribute( "width" ).toInt();
            window.height = ee.attribute( "height" ).toInt();
            window.title = ee.attribute( "title" );
            window.summary = ee.attribute( "summary" );
            // parse window subnodes
            QDomNode winNode = ee.firstChild();
            for ( ; winNode.isElement(); winNode = winNode.nextSibling() )
            {
                QDomElement winElement = winNode.toElement();
                if ( winElement.tagName() == "text" )
                    window.text = winElement.firstChild().toCDATASection().data();
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
        Revision rev;
        rev.scope = (RevScope)revElement.attribute( "revScope" ).toInt();
        rev.type = (RevType)revElement.attribute( "revType" ).toInt();
        rev.annotation = AnnotationUtils::createAnnotation( revElement );

        // if annotation is valid, add revision to internal list
        if ( rev.annotation );
            revisions.append( rev );
    }
}

void Annotation::store( QDomNode & annNode, QDomDocument & document ) const
{
    // create [base] element of the annotation node
    QDomElement e = document.createElement( "base" );
    annNode.appendChild( e );

    // store -contents- attributes
    if ( !author.isEmpty() )
        e.setAttribute( "author", author );
    if ( !contents.isEmpty() )
        e.setAttribute( "contents", contents );
    if ( !uniqueName.isEmpty() )
        e.setAttribute( "uniqueName", uniqueName );
    if ( modifyDate.isValid() )
        e.setAttribute( "modifyDate", modifyDate.toString() );
    if ( creationDate.isValid() )
        e.setAttribute( "creationDate", creationDate.toString() );

    // store -other- attributes
    if ( flags )
        e.setAttribute( "flags", flags );
    if ( style.color.isValid() && style.color != Qt::black )
        e.setAttribute( "color", style.color.name() );
    if ( style.opacity != 1.0 )
        e.setAttribute( "opacity", style.opacity );

    // Sub-Node-1 - boundary
    QDomElement bE = document.createElement( "boundary" );
    e.appendChild( bE );
    bE.setAttribute( "l", (double)boundary.left );
    bE.setAttribute( "t", (double)boundary.top );
    bE.setAttribute( "r", (double)boundary.right );
    bE.setAttribute( "b", (double)boundary.bottom );

    // Sub-Node-2 - penStyle
    if ( style.width != 1 || style.style != Solid || style.xCorners != 0 ||
         style.yCorners != 0.0 || style.marks != 3 || style.spaces != 0 )
    {
        QDomElement psE = document.createElement( "penStyle" );
        e.appendChild( psE );
        psE.setAttribute( "width", style.width );
        psE.setAttribute( "style", (int)style.style );
        psE.setAttribute( "xcr", style.xCorners );
        psE.setAttribute( "ycr", style.yCorners );
        psE.setAttribute( "marks", style.marks );
        psE.setAttribute( "spaces", style.spaces );
    }

    // Sub-Node-3 - penEffect
    if ( style.effect != NoEffect || style.effectIntensity != 1.0 )
    {
        QDomElement peE = document.createElement( "penEffect" );
        e.appendChild( peE );
        peE.setAttribute( "effect", (int)style.effect );
        peE.setAttribute( "intensity", style.effectIntensity );
    }

    // Sub-Node-4 - window
    if ( window.flags != -1 || !window.title.isEmpty() ||
         !window.summary.isEmpty() || !window.text.isEmpty() )
    {
        QDomElement wE = document.createElement( "window" );
        e.appendChild( wE );
        wE.setAttribute( "flags", window.flags );
        wE.setAttribute( "top", window.topLeft.x );
        wE.setAttribute( "left", window.topLeft.y );
        wE.setAttribute( "width", window.width );
        wE.setAttribute( "height", window.height );
        wE.setAttribute( "title", window.title );
        wE.setAttribute( "summary", window.summary );
        // store window.text as a subnode, because we need escaped data
        if ( !window.text.isEmpty() )
        {
            QDomElement escapedText = document.createElement( "text" );
            wE.appendChild( escapedText );
            QDomCDATASection textCData = document.createCDATASection( window.text );
            escapedText.appendChild( textCData );
        }
    }

    // create [revision] element of the annotation node (if any)
    if ( revisions.isEmpty() )
        return;

    // add all revisions as children of revisions element
    QLinkedList< Revision >::const_iterator it = revisions.begin(), end = revisions.end();
    for ( ; it != end; ++it )
    {
        // create revision element
        const Revision & revision = *it;
        QDomElement r = document.createElement( "revision" );
        annNode.appendChild( r );
        // set element attributes
        r.setAttribute( "revScope", (int)revision.scope );
        r.setAttribute( "revType", (int)revision.type );
        // use revision as the annotation element, so fill it up
        AnnotationUtils::storeAnnotation( revision.annotation, r, document );
    }
}
//END AnnotationUtils implementation


/** TextAnnotation [Annotation] */

TextAnnotation::TextAnnotation()
    : Annotation(), textType( Linked ), textIcon( "Comment" ),
    inplaceAlign( 0 ), inplaceIntent( Unknown )
{}

TextAnnotation::TextAnnotation( const QDomNode & node )
    : Annotation( node ), textType( Linked ), textIcon( "Comment" ),
    inplaceAlign( 0 ), inplaceIntent( Unknown )
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
            textType = (TextAnnotation::TextType)e.attribute( "type" ).toInt();
        if ( e.hasAttribute( "icon" ) )
            textIcon = e.attribute( "icon" );
        if ( e.hasAttribute( "font" ) )
            textFont.fromString( e.attribute( "font" ) );
        if ( e.hasAttribute( "align" ) )
            inplaceAlign = e.attribute( "align" ).toInt();
        if ( e.hasAttribute( "intent" ) )
            inplaceIntent = (TextAnnotation::InplaceIntent)e.attribute( "intent" ).toInt();

        // parse the subnodes
        QDomNode eSubNode = e.firstChild();
        while ( eSubNode.isElement() )
        {
            QDomElement ee = eSubNode.toElement();
            eSubNode = eSubNode.nextSibling();

            if ( ee.tagName() == "escapedText" )
            {
                inplaceText = ee.firstChild().toCDATASection().data();
            }
            else if ( ee.tagName() == "callout" )
            {
                inplaceCallout[0].x = ee.attribute( "ax" ).toDouble();
                inplaceCallout[0].y = ee.attribute( "ay" ).toDouble();
                inplaceCallout[1].x = ee.attribute( "bx" ).toDouble();
                inplaceCallout[1].y = ee.attribute( "by" ).toDouble();
                inplaceCallout[2].x = ee.attribute( "cx" ).toDouble();
                inplaceCallout[2].y = ee.attribute( "cy" ).toDouble();
            }
        }

        // loading complete
        break;
    }
}

void TextAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [text] element
    QDomElement textElement = document.createElement( "text" );
    node.appendChild( textElement );

    // store the optional attributes
    if ( textType != Linked )
        textElement.setAttribute( "type", (int)textType );
    if ( textIcon != "Comment" )
        textElement.setAttribute( "icon", textIcon );
    if ( textFont != QApplication::font() )
        textElement.setAttribute( "font", textFont.toString() );
    if ( inplaceAlign )
        textElement.setAttribute( "align", inplaceAlign );
    if ( inplaceIntent != Unknown )
        textElement.setAttribute( "intent", (int)inplaceIntent );

    // Sub-Node-1 - escapedText
    if ( !inplaceText.isEmpty() )
    {
        QDomElement escapedText = document.createElement( "escapedText" );
        textElement.appendChild( escapedText );
        QDomCDATASection textCData = document.createCDATASection( inplaceText );
        escapedText.appendChild( textCData );
    }

    // Sub-Node-2 - callout
    if ( inplaceCallout[0].x != 0.0 )
    {
        QDomElement calloutElement = document.createElement( "callout" );
        textElement.appendChild( calloutElement );
        calloutElement.setAttribute( "ax", inplaceCallout[0].x );
        calloutElement.setAttribute( "ay", inplaceCallout[0].y );
        calloutElement.setAttribute( "bx", inplaceCallout[1].x );
        calloutElement.setAttribute( "by", inplaceCallout[1].y );
        calloutElement.setAttribute( "cx", inplaceCallout[2].x );
        calloutElement.setAttribute( "cy", inplaceCallout[2].y );
    }
}


/** LineAnnotation [Annotation] */

LineAnnotation::LineAnnotation()
    : Annotation(), lineStartStyle( None ), lineEndStyle( None ),
    lineClosed( false ), lineLeadingFwdPt( 0 ), lineLeadingBackPt( 0 ),
    lineShowCaption( false ), lineIntent( Unknown )
{}

LineAnnotation::LineAnnotation( const QDomNode & node )
    : Annotation( node ), lineStartStyle( None ), lineEndStyle( None ),
    lineClosed( false ), lineLeadingFwdPt( 0 ), lineLeadingBackPt( 0 ),
    lineShowCaption( false ), lineIntent( Unknown )
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
            lineStartStyle = (LineAnnotation::TermStyle)e.attribute( "startStyle" ).toInt();
        if ( e.hasAttribute( "endStyle" ) )
            lineEndStyle = (LineAnnotation::TermStyle)e.attribute( "endStyle" ).toInt();
        if ( e.hasAttribute( "closed" ) )
            lineClosed = e.attribute( "closed" ).toInt();
        if ( e.hasAttribute( "innerColor" ) )
            lineInnerColor = QColor( e.attribute( "innerColor" ) );
        if ( e.hasAttribute( "leadFwd" ) )
            lineLeadingFwdPt = e.attribute( "leadFwd" ).toDouble();
        if ( e.hasAttribute( "leadBack" ) )
            lineLeadingBackPt = e.attribute( "leadBack" ).toDouble();
        if ( e.hasAttribute( "showCaption" ) )
            lineShowCaption = e.attribute( "showCaption" ).toInt();
        if ( e.hasAttribute( "intent" ) )
            lineIntent = (LineAnnotation::LineIntent)e.attribute( "intent" ).toInt();

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
            linePoints.append( p );
        }

        // loading complete
        break;
    }
}

void LineAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [line] element
    QDomElement lineElement = document.createElement( "line" );
    node.appendChild( lineElement );

    // store the attributes
    if ( lineStartStyle != None )
        lineElement.setAttribute( "startStyle", (int)lineStartStyle );
    if ( lineEndStyle != None )
        lineElement.setAttribute( "endStyle", (int)lineEndStyle );
    if ( lineClosed )
        lineElement.setAttribute( "closed", lineClosed );
    if ( lineInnerColor.isValid() )
        lineElement.setAttribute( "innerColor", lineInnerColor.name() );
    if ( lineLeadingFwdPt != 0.0 )
        lineElement.setAttribute( "leadFwd", lineLeadingFwdPt );
    if ( lineLeadingBackPt != 0.0 )
        lineElement.setAttribute( "leadBack", lineLeadingBackPt );
    if ( lineShowCaption )
        lineElement.setAttribute( "showCaption", lineShowCaption );
    if ( lineIntent != Unknown )
        lineElement.setAttribute( "intent", lineIntent );

    // append the list of points
    int points = linePoints.count();
    if ( points > 1 )
    {
        QLinkedList<NormalizedPoint>::const_iterator it = linePoints.begin(), end = linePoints.end();
        while ( it != end )
        {
            const NormalizedPoint & p = *it;
            QDomElement pElement = document.createElement( "point" );
            lineElement.appendChild( pElement );
            pElement.setAttribute( "x", p.x );
            pElement.setAttribute( "y", p.y );
        }
    }
}


/** GeomAnnotation [Annotation] */

GeomAnnotation::GeomAnnotation()
    : Annotation(), geomType( InscribedSquare ), geomWidthPt( 18 )
{}

GeomAnnotation::GeomAnnotation( const QDomNode & node )
    : Annotation( node ), geomType( InscribedSquare ), geomWidthPt( 18 )
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
            geomType = (GeomAnnotation::GeomType)e.attribute( "type" ).toInt();
        if ( e.hasAttribute( "color" ) )
            geomInnerColor = QColor( e.attribute( "color" ) );
        if ( e.hasAttribute( "width" ) )
            geomWidthPt = e.attribute( "width" ).toInt();

        // loading complete
        break;
    }
}

void GeomAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [geom] element
    QDomElement geomElement = document.createElement( "geom" );
    node.appendChild( geomElement );

    // append the optional attributes
    if ( geomType != InscribedSquare )
        geomElement.setAttribute( "type", (int)geomType );
    if ( geomInnerColor.isValid() )
        geomElement.setAttribute( "color", geomInnerColor.name() );
    if ( geomWidthPt != 18 )
        geomElement.setAttribute( "width", geomWidthPt );
}


/** HighlightAnnotation [Annotation] */

HighlightAnnotation::HighlightAnnotation()
    : Annotation(), highlightType( Highlight )
{}

HighlightAnnotation::HighlightAnnotation( const QDomNode & node )
    : Annotation( node ), highlightType( Highlight )
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
            highlightType = (HighlightAnnotation::HighlightType)e.attribute( "type" ).toInt();

        // parse all 'quad' subnodes
        QDomNode quadNode = e.firstChild();
        for ( ; quadNode.isElement(); quadNode = quadNode.nextSibling() )
        {
            QDomElement qe = quadNode.toElement();
            if ( qe.tagName() != "quad" )
                continue;

            Quad q;
            q.points[0].x = qe.attribute( "ax", "0.0" ).toDouble();
            q.points[0].y = qe.attribute( "ay", "0.0" ).toDouble();
            q.points[1].x = qe.attribute( "bx", "0.0" ).toDouble();
            q.points[1].y = qe.attribute( "by", "0.0" ).toDouble();
            q.points[2].x = qe.attribute( "cx", "0.0" ).toDouble();
            q.points[2].y = qe.attribute( "cy", "0.0" ).toDouble();
            q.points[3].x = qe.attribute( "dx", "0.0" ).toDouble();
            q.points[3].y = qe.attribute( "dy", "0.0" ).toDouble();
            q.capStart = qe.hasAttribute( "start" );
            q.capEnd = qe.hasAttribute( "end" );
            q.feather = qe.attribute( "feather", "0.1" ).toDouble();
            highlightQuads.append( q );
        }

        // loading complete
        break;
    }
}

void HighlightAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [hl] element
    QDomElement hlElement = document.createElement( "hl" );
    node.appendChild( hlElement );

    // append the optional attributes
    if ( highlightType != Highlight )
        hlElement.setAttribute( "type", (int)highlightType );
    if ( highlightQuads.count() < 1 )
        return;
    // append highlight quads, all children describe quads
    QList< Quad >::const_iterator it = highlightQuads.begin(), end = highlightQuads.end();
    for ( ; it != end; ++it )
    {
        QDomElement quadElement = document.createElement( "quad" );
        hlElement.appendChild( quadElement );
        const Quad & q = *it;
        quadElement.setAttribute( "ax", q.points[0].x );
        quadElement.setAttribute( "ay", q.points[0].y );
        quadElement.setAttribute( "bx", q.points[1].x );
        quadElement.setAttribute( "by", q.points[1].y );
        quadElement.setAttribute( "cx", q.points[2].x );
        quadElement.setAttribute( "cy", q.points[2].y );
        quadElement.setAttribute( "dx", q.points[3].x );
        quadElement.setAttribute( "dy", q.points[3].y );
        if ( q.capStart )
            quadElement.setAttribute( "start", 1 );
        if ( q.capEnd )
            quadElement.setAttribute( "end", 1 );
        quadElement.setAttribute( "feather", q.feather );
    }
}


/** StampAnnotation [Annotation] */

StampAnnotation::StampAnnotation()
    : Annotation(), stampIconName( "oKular" )
{}

StampAnnotation::StampAnnotation( const QDomNode & node )
    : Annotation( node ), stampIconName( "oKular" )
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
            stampIconName = e.attribute( "icon" );

        // loading complete
        break;
    }
}

void StampAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [stamp] element
    QDomElement stampElement = document.createElement( "stamp" );
    node.appendChild( stampElement );

    // append the optional attributes
    if ( stampIconName != "oKular" )
        stampElement.setAttribute( "icon", stampIconName );
}


/** InkAnnotation [Annotation] */

InkAnnotation::InkAnnotation()
    : Annotation()
{}

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
                inkPaths.append( path );
        }

        // loading complete
        break;
    }
}

void InkAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse to parent objects storing properties
    Annotation::store( node, document );

    // create [ink] element
    QDomElement inkElement = document.createElement( "ink" );
    node.appendChild( inkElement );

    // append the optional attributes
    if ( inkPaths.count() < 1 )
        return;
    QList< QLinkedList<NormalizedPoint> >::const_iterator pIt = inkPaths.begin(), pEnd = inkPaths.end();
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
