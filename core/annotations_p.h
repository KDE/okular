/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_ANNOTATION_P_H
#define OKULAR_ANNOTATION_P_H

#include "area.h"
#include "annotations.h"

// qt/kde includes
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtGui/QColor>

class QMatrix;

namespace Okular {

class AnnotationPrivate
{
    public:
        AnnotationPrivate();

        virtual ~AnnotationPrivate();

        /**
         * Transforms the annotation coordinates with the transformation
         * defined by @p matrix.
         */
        virtual void transform( const QMatrix &matrix );

        QString m_author;
        QString m_contents;
        QString m_uniqueName;
        QDateTime m_modifyDate;
        QDateTime m_creationDate;

        int m_flags;
        NormalizedRect m_boundary;
        NormalizedRect m_transformedBoundary;

        Okular::Annotation::Style m_style;
        Okular::Annotation::Window m_window;
        QLinkedList< Okular::Annotation::Revision > m_revisions;
};

}

#endif
