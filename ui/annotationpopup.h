/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef ANNOTATIONPOPUP_H
#define ANNOTATIONPOPUP_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QPoint>

namespace Okular {
class Annotation;
class Document;
}

class AnnotationPopup : public QObject
{
    Q_OBJECT

    public:
        AnnotationPopup( Okular::Document *document,
                         QWidget *parent = 0 );

        void addAnnotation( Okular::Annotation* annotation, int pageNumber );

        void exec( const QPoint &point = QPoint() );

    Q_SIGNALS:
        void setAnnotationWindow( Okular::Annotation *annotation );
        void removeAnnotationWindow( Okular::Annotation *annotation );

    private:
        QWidget *mParent;
        typedef QPair< Okular::Annotation*, int > AnnotPagePair;
        QList< AnnotPagePair > mAnnotations;
        Okular::Document *mDocument;
};


#endif
