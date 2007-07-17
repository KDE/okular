/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "annotationguiutils.h"

// qt/kde includes
#include <klocale.h>

// local includes
#include "core/annotations.h"

QString AnnotationGuiUtils::captionForAnnotation( Okular::Annotation * ann )
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

QString AnnotationGuiUtils::contents( const Okular::Annotation * ann )
{
    if ( !ann )
        return QString();

    // 1. window text
    QString ret = ann->window().text();
    if ( !ret.isEmpty() )
        return ret;
    // 2. if Text and InPlace, the inplace text
    if ( ann->subType() == Okular::Annotation::AText )
    {
        const Okular::TextAnnotation * txtann = static_cast< const Okular::TextAnnotation * >( ann );
        if ( txtann->textType() == Okular::TextAnnotation::InPlace )
        {
            ret = txtann->inplaceText();
            if ( !ret.isEmpty() )
                return ret;
        }
    }

    // 3. contents
    ret = ann->contents();

    return ret;
}

QString AnnotationGuiUtils::contentsHtml( const Okular::Annotation * ann )
{
    return contents( ann ).replace( "\n", "<br>" );
}

bool AnnotationGuiUtils::canBeMoved( Okular::Annotation * ann )
{
    if ( !ann )
        return false;

    switch( ann->subType() )
    {
        case Okular::Annotation::ALine:
        case Okular::Annotation::AStamp:
        case Okular::Annotation::AGeom:
        case Okular::Annotation::AInk:
            return true;
            break;
        default:
            break;
    }
    return false;
}

