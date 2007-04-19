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
#include <QtCore/QPoint>

namespace Okular {
class Annotation;
class Document;
}

class AnnotationPopup : public QObject
{
    Q_OBJECT

    public:
        AnnotationPopup( Okular::Annotation *annotation,
                         Okular::Document *document,
                         QWidget *parent = 0 );

        void setPageNumber( int pageNumber );

        void exec( const QPoint &point = QPoint() );

    Q_SIGNALS:
        void setAnnotationWindow( Okular::Annotation *annotation );
        void removeAnnotationWindow( Okular::Annotation *annotation );

    private:
        QWidget *mParent;
        Okular::Annotation *mAnnotation;
        Okular::Document *mDocument;
        int mPageNumber;
};


#endif
