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

#include <QAbstractItemModel>

namespace Okular
{
class Annotation;
class Document;
}

class AnnotationModelPrivate;

class AnnotationModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum { AuthorRole = Qt::UserRole + 1000, PageRole };

    explicit AnnotationModel(Okular::Document *document, QObject *parent = nullptr);
    ~AnnotationModel() override;

    // reimplementations from QAbstractItemModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool isAnnotation(const QModelIndex &index) const;
    Okular::Annotation *annotationForIndex(const QModelIndex &index) const;

private:
    // storage
    friend class AnnotationModelPrivate;
    AnnotationModelPrivate *const d;
};

#endif
