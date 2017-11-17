/***************************************************************************
 *   Copyright (C) 2012 by Fabio D'Urso <fabiodurso@hotmail.it>            *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_PDF_ANNOTS_H_
#define _OKULAR_GENERATOR_PDF_ANNOTS_H_

#include <poppler-annotation.h>
#include <poppler-qt5.h>

#include <qmutex.h>

#include "core/annotations.h"
#include "config-okular-poppler.h"

extern Okular::Annotation* createAnnotationFromPopplerAnnotation( Poppler::Annotation *ann, bool * doDelete );

class PopplerAnnotationProxy : public Okular::AnnotationProxy
{
    public:
        PopplerAnnotationProxy( Poppler::Document *doc, QMutex *userMutex, QHash<Okular::Annotation*, Poppler::Annotation*> *annotsOnOpenHash );
        ~PopplerAnnotationProxy();

        bool supports( Capability capability ) const override;
        void notifyAddition( Okular::Annotation *annotation, int page ) override;
        void notifyModification( const Okular::Annotation *annotation, int page, bool appearanceChanged ) override;
        void notifyRemoval( Okular::Annotation *annotation, int page ) override;
    private:
        Poppler::Document *ppl_doc;
        QMutex *mutex;
        QHash<Okular::Annotation*, Poppler::Annotation*> *annotationsOnOpenHash;
};

#endif
