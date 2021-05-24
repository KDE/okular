/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ANNOTATIONPROXYMODEL_H
#define ANNOTATIONPROXYMODEL_H

#include <QPair>
#include <QSortFilterProxyModel>

/**
 * A proxy model, which filters out all pages except the
 * current one.
 */
class PageFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    /**
     * Creates a new page filter proxy model.
     *
     * @param parent The parent object.
     */
    explicit PageFilterProxyModel(QObject *parent = nullptr);

    /**
     * Reimplemented from QSortFilterProxy.
     */
    bool filterAcceptsRow(int, const QModelIndex &) const override;

public Q_SLOTS:
    /**
     * Sets whether the proxy model shall filter
     * by current page.
     */
    void groupByCurrentPage(bool value);

    /**
     * Sets the current page.
     */
    void setCurrentPage(int page);

private:
    bool mGroupByCurrentPage;
    int mCurrentPage;
};

/**
 * A proxy model which either groups the annotations by
 * pages or shows them all as list.
 */
class PageGroupProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    /**
     * Creates a new page group proxy model.
     *
     * @param parent The parent object.
     */
    explicit PageGroupProxyModel(QObject *parent = nullptr);

    int columnCount(const QModelIndex &parentIndex) const override;
    int rowCount(const QModelIndex &parentIndex) const override;

    QModelIndex index(int row, int column, const QModelIndex &parentIndex = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &idx) const override;

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

    void setSourceModel(QAbstractItemModel *model) override;

public Q_SLOTS:
    /**
     * Sets whether the proxy model shall group
     * the annotations by page.
     */
    void groupByPage(bool value);

private Q_SLOTS:
    void rebuildIndexes();
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

private:
    bool mGroupByPage;
    QList<QModelIndex> mIndexes;
    QList<QPair<QModelIndex, QList<QModelIndex>>> mTreeIndexes;
};

/**
 * A proxy model which groups the annotations by author.
 */
class AuthorGroupProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    /**
     * Creates a new author group proxy model.
     *
     * @param parent The parent object.
     */
    explicit AuthorGroupProxyModel(QObject *parent = nullptr);
    ~AuthorGroupProxyModel() override;

    int columnCount(const QModelIndex &parentIndex) const override;
    int rowCount(const QModelIndex &parentIndex) const override;

    QModelIndex index(int row, int column, const QModelIndex &parentIndex = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

    void setSourceModel(QAbstractItemModel *model) override;

    QItemSelection mapSelectionToSource(const QItemSelection &selection) const override;
    QItemSelection mapSelectionFromSource(const QItemSelection &selection) const override;
    QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const override;
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

public Q_SLOTS:
    /**
     * Sets whether the proxy model shall group
     * the annotations by author.
     */
    void groupByAuthor(bool value);

private Q_SLOTS:
    void rebuildIndexes();
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

private:
    class Private;
    Private *const d;
};

#endif
