/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_ANNOTATIONS_P_H
#define OKULAR_ANNOTATIONS_P_H

#include "area.h"
#include "annotations.h"

// qt/kde includes
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtGui/QColor>

class QTransform;

namespace Okular {

class PagePrivate;

class AnnotationPrivate
{
    public:
        AnnotationPrivate();

        virtual ~AnnotationPrivate();

        /**
         * Transforms the annotation coordinates with the transformation
         * defined by @p matrix.
         */
        void annotationTransform( const QTransform &matrix );

        virtual void transform( const QTransform &matrix );
        virtual void baseTransform( const QTransform &matrix );
        virtual void resetTransformation();
        virtual void translate( const NormalizedPoint &coord );
        virtual void adjust( const NormalizedPoint & deltaCoord1, const NormalizedPoint & deltaCoord2 );
        virtual bool openDialogAfterCreation() const;
        virtual void setAnnotationProperties( const QDomNode& node );
        virtual bool canBeResized() const;
        virtual AnnotationPrivate* getNewAnnotationPrivate() = 0;

        /**
         * Determines the distance of the closest point of the annotation to the
         * given point @p x @p y @p xScale @p yScale
         * @since 0.17
         */
        virtual double distanceSqr( double x, double y, double xScale, double yScale );

        PagePrivate * m_page;

        QString m_author;
        QString m_contents;
        QString m_key;        
        QString m_uniqueName;
        QDateTime m_modifyDate;
        QDateTime m_creationDate;

        int m_flags;
        NormalizedRect m_boundary;
        NormalizedRect m_transformedBoundary;

        Okular::Annotation::Style m_style;
        Okular::Annotation::Window m_window;
        QLinkedList< Okular::Annotation::Revision > m_revisions;

        Annotation::DisposeDataFunction m_disposeFunc;
        QVariant m_nativeId;
};

}

#endif
