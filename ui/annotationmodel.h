/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef ANNOTATIONMODEL_H
#define ANNOTATIONMODEL_H

#include <qabstractitemmodel.h>

namespace Okular {
class Annotation;
class Document;
}

class AnnotationModelPrivate;

class AnnotationModel : public QAbstractItemModel
{
    Q_OBJECT

    public:
        enum {
            AuthorRole = Qt::UserRole + 1000,
            PageRole
        };

        explicit AnnotationModel( Okular::Document *document, QObject *parent = 0 );
        virtual ~AnnotationModel();

        // reimplementations from QAbstractItemModel
        int columnCount( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;
        QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
        bool hasChildren( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;
        QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
        QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;
        QModelIndex parent( const QModelIndex &index ) const Q_DECL_OVERRIDE;
        int rowCount( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;

        bool isAnnotation( const QModelIndex &index ) const;
        Okular::Annotation* annotationForIndex( const QModelIndex &index ) const;

    private:
        // storage
        friend class AnnotationModelPrivate;
        AnnotationModelPrivate *const d;
};



#endif
