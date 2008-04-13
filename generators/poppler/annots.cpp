/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <poppler-annotation.h>

// qt/kde includes
#include <qvariant.h>

#include <okular/core/annotations.h>

Q_DECLARE_METATYPE( Poppler::Annotation* )

static void disposeAnnotation( const Okular::Annotation *ann )
{
    Poppler::Annotation *popplerAnn = qvariant_cast< Poppler::Annotation * >( ann->nativeId() );
    delete popplerAnn;
}

Okular::Annotation* createAnnotationFromPopplerAnnotation( Poppler::Annotation *ann, bool *doDelete )
{
    Okular::Annotation *annotation = 0;
    *doDelete = true;
    bool tieToOkularAnn = false;
    switch ( ann->subType() )
    {
        default:
        {
            // this is uber ugly but i don't know a better way to do it without introducing a poppler::annotation dependency on core
            QDomDocument doc;
            QDomElement root = doc.createElement( "root" );
            doc.appendChild( root );
            Poppler::AnnotationUtils::storeAnnotation( ann, root, doc );
            annotation = Okular::AnnotationUtils::createAnnotation( root );
            return annotation;
        }
    }
    if ( annotation )
    {
        annotation->setAuthor( ann->author() );
        annotation->setContents( ann->contents() );
        annotation->setUniqueName( ann->uniqueName() );
        annotation->setModificationDate( ann->modificationDate() );
        annotation->setCreationDate( ann->creationDate() );
        annotation->setFlags( ann->flags() );
        annotation->setBoundingRectangle( Okular::NormalizedRect::fromQRectF( ann->boundary() ) );
        // TODO clone style
        // TODO clone window
        // TODO clone revisions
        if ( tieToOkularAnn )
        {
            annotation->setNativeId( qVariantFromValue( ann ) );
            annotation->setDisposeDataFunction( disposeAnnotation );
        }
    }
    return annotation;
}
