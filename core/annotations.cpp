/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qcolor.h>
#include <qapplication.h>

// local includes
#include "annotations.h"


/** Annotation **/

Annotation::Border::Border()
    : width( 1 ), xCornerRadius( 0 ), yCornerRadius( 0 ), style( Solid ),
    dashMarks( 3 ), dashSpaces( 0 ), effect( NoEffect ), effectIntensity( 0 ) {}

Annotation::Annotation()
    : modifyDate( QDateTime::currentDateTime() ), flags( 0 )
{
}

Annotation::Annotation( const QDomElement & /*node*/ )
{
}

void Annotation::store( QDomNode & /*node*/, QDomDocument & /*document*/ ) const
{
}


/** MarkupAnnotation **/

MarkupAnnotation::InReplyTo::InReplyTo()
    : ID( -1 ), scope( Reply ), stateModel( Mark ), state( Unmarked ) {}

MarkupAnnotation::MarkupAnnotation()
    : Annotation(), markupOpacity( 1.0 ), markupPopupID( -1 ),
    markupCreationDate( QDateTime::currentDateTime() )
{
}

MarkupAnnotation::MarkupAnnotation( const QDomElement & node )
    : Annotation( node )
{
    // ... load stuff ...
}

void MarkupAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // ... save stuff ...
    Annotation::store( node, document );
}


/** PopupAnnotation **/

PopupAnnotation::PopupAnnotation()
    : Annotation(), popupMarkupParentID( -1 ), popupOpened( false )
{
}

PopupAnnotation::PopupAnnotation( const QDomElement & node )
    : Annotation( node )
{
    // ... load stuff ...
}

void PopupAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // ... save stuff ...
    Annotation::store( node, document );
}


/** TextAnnotation **/

TextAnnotation::TextAnnotation()
    : MarkupAnnotation(), textType( Popup ), textFont( QApplication::font() ),
    popupOpened( false ), popupIcon( "Note" ), inplaceAlign( 0 ),
    inplaceIntent( Unknown )
{
}

TextAnnotation::TextAnnotation( const QDomElement & node )
    : MarkupAnnotation( node )
{
    // ... load stuff ...
}

void TextAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // ... save stuff ...
    MarkupAnnotation::store( node, document );
}


/** LineAnnotation **/

LineAnnotation::LineAnnotation()
    : MarkupAnnotation(), lineStartStyle( None ), lineEndStyle( None ),
    lineClosed( false ), lineLeadingFwdPt( 0 ), lineLeadingBackPt( 0 ),
    lineShowCaption( false ), lineIntent( Unknown )
{
}

LineAnnotation::LineAnnotation( const QDomElement & node )
    : MarkupAnnotation( node )
{
    // ... load stuff ...
}

void LineAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // ... save stuff ...
    MarkupAnnotation::store( node, document );
}


/** GeomAnnotation **/

GeomAnnotation::GeomAnnotation()
    : MarkupAnnotation(), geomType( InscribedSquare ), geomWidthPt( 18 )
{
}

GeomAnnotation::GeomAnnotation( const QDomElement & node )
    : MarkupAnnotation( node )
{
    // ... load stuff ...
}

void GeomAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // ... save stuff ...
    MarkupAnnotation::store( node, document );
}


/** HighlightAnnotation **/

HighlightAnnotation::HighlightAnnotation()
    : MarkupAnnotation(), highlightType( Highlight )
{
}

HighlightAnnotation::HighlightAnnotation( const QDomElement & node )
    : MarkupAnnotation( node )
{
    // ... load stuff ...
}

void HighlightAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // ... save stuff ...
    MarkupAnnotation::store( node, document );
}


/** StampAnnotation **/

StampAnnotation::StampAnnotation()
    : MarkupAnnotation(), stampIconName( "kpdf" )
{
}

StampAnnotation::StampAnnotation( const QDomElement & node )
    : MarkupAnnotation( node )
{
    // ... load stuff ...
}

void StampAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // ... save stuff ...
    MarkupAnnotation::store( node, document );
}


/** InkAnnotation **/

InkAnnotation::InkAnnotation()
    : MarkupAnnotation()
{
}

InkAnnotation::InkAnnotation( const QDomElement & node )
    : MarkupAnnotation( node )
{
    // ... load stuff ...
}

void InkAnnotation::store( QDomNode & node, QDomDocument & document ) const
{
    // recurse saving parent's attributes
    MarkupAnnotation::store( node, document );

    // save local attributes
}
