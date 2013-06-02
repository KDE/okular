/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef TOCMODEL_H
#define TOCMODEL_H

#include <qabstractitemmodel.h>
#include <QVector>

namespace Okular {
class Document;
class DocumentSynopsis;
class DocumentViewport;
}

class TOCModelPrivate;

class TOCModel : public QAbstractItemModel
{
    Q_OBJECT

    public:
        explicit TOCModel( Okular::Document *document, QObject *parent = 0 );
        virtual ~TOCModel();

        // reimplementations from QAbstractItemModel
        virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
        virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
        virtual bool hasChildren( const QModelIndex &parent = QModelIndex() ) const;
        virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
        virtual QModelIndex parent( const QModelIndex &index ) const;
        virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;

        void fill( const Okular::DocumentSynopsis *toc );
        void clear();
        void setCurrentViewport( const Okular::DocumentViewport &viewport );

        bool isEmpty() const;
        bool equals( const TOCModel *model ) const;
        void setOldModelData( TOCModel *model, const QVector<QModelIndex> &list );
        bool hasOldModelData() const;
        TOCModel *clearOldModelData() const;

        QString externalFileNameForIndex( const QModelIndex &index ) const;
        Okular::DocumentViewport viewportForIndex( const QModelIndex &index ) const;
        QString urlForIndex( const QModelIndex &index ) const;

    private:
        // storage
        friend class TOCModelPrivate;
        TOCModelPrivate *const d;
        bool checkequality( const TOCModel *model, const QModelIndex &parentA = QModelIndex(), const QModelIndex &parentB = QModelIndex() ) const;
};

#endif
