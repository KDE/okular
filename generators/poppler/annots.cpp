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

#include <core/annotations.h>

#include "popplerembeddedfile.h"
#include "config-okular-poppler.h"

Q_DECLARE_METATYPE( Poppler::Annotation* )

extern Okular::Sound* createSoundFromPopplerSound( const Poppler::SoundObject *popplerSound );
extern Okular::Movie* createMovieFromPopplerMovie( const Poppler::MovieObject *popplerMovie );

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
        case Poppler::Annotation::AFileAttachment:
        {
            Poppler::FileAttachmentAnnotation * attachann = static_cast< Poppler::FileAttachmentAnnotation * >( ann );
            Okular::FileAttachmentAnnotation * f = new Okular::FileAttachmentAnnotation();
            annotation = f;
            tieToOkularAnn = true;
            *doDelete = false;

            f->setFileIconName( attachann->fileIconName() );
            f->setEmbeddedFile( new PDFEmbeddedFile( attachann->embeddedFile() ) );

            break;
        }
        case Poppler::Annotation::ASound:
        {
            Poppler::SoundAnnotation * soundann = static_cast< Poppler::SoundAnnotation * >( ann );
            Okular::SoundAnnotation * s = new Okular::SoundAnnotation();
            annotation = s;

            s->setSoundIconName( soundann->soundIconName() );
            s->setSound( createSoundFromPopplerSound( soundann->sound() ) );

            break;
        }
        case Poppler::Annotation::AMovie:
        {
            Poppler::MovieAnnotation * movieann = static_cast< Poppler::MovieAnnotation * >( ann );
            Okular::MovieAnnotation * m = new Okular::MovieAnnotation();
            annotation = m;

            m->setMovie( createMovieFromPopplerMovie( movieann->movie() ) );

            break;
        }
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
