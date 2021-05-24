/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SIGNATUREMODEL_H
#define SIGNATUREMODEL_H

#include <QAbstractItemModel>

namespace Okular
{
class Document;
}

class SignatureModelPrivate;

class SignatureModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum { FormRole = Qt::UserRole + 1000, PageRole };

    explicit SignatureModel(Okular::Document *doc, QObject *parent = nullptr);
    ~SignatureModel() override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    Q_DECLARE_PRIVATE(SignatureModel)
    QScopedPointer<SignatureModelPrivate> d_ptr;
};

#endif
