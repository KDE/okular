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
        explicit AnnotationPopup( Okular::Document *document,
                                  QWidget *parent = 0 );

        void addAnnotation( Okular::Annotation* annotation, int pageNumber );

        void exec( const QPoint &point = QPoint() );

    Q_SIGNALS:
        void openAnnotationWindow( Okular::Annotation *annotation, int pageNumber );

    public:
        struct AnnotPagePair {
            AnnotPagePair() : annotation( 0 ),  pageNumber( -1 )
            { }

            AnnotPagePair( Okular::Annotation *a, int pn ) : annotation( a ),  pageNumber( pn )
            { }
            
            AnnotPagePair( const AnnotPagePair & pair ) : annotation( pair.annotation ),  pageNumber( pair.pageNumber )
            { }
            
            bool operator==( const AnnotPagePair & pair ) const
            { return annotation == pair.annotation && pageNumber == pair.pageNumber; }
            
            Okular::Annotation* annotation;
            int pageNumber;
        };

    private:
        QWidget *mParent;

        QList< AnnotPagePair > mAnnotations;
        Okular::Document *mDocument;
};


#endif
