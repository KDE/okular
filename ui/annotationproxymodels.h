/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef ANNOTATIONPROXYMODEL_H
#define ANNOTATIONPROXYMODEL_H

#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QPair>

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
    explicit PageFilterProxyModel( QObject *parent = nullptr );

    /**
     * Reimplemented from QSortFilterProxy.
     */
    bool filterAcceptsRow( int, const QModelIndex& ) const Q_DECL_OVERRIDE;

  public Q_SLOTS:
    /**
     * Sets whether the proxy model shall filter
     * by current page.
     */
    void groupByCurrentPage( bool value );

    /**
     * Sets the current page.
     */
    void setCurrentPage( int page );

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
    explicit PageGroupProxyModel( QObject *parent = nullptr );

    int columnCount( const QModelIndex &parentIndex ) const Q_DECL_OVERRIDE;
    int rowCount( const QModelIndex &parentIndex ) const Q_DECL_OVERRIDE;

    QModelIndex index( int row, int column, const QModelIndex &parentIndex = QModelIndex() ) const Q_DECL_OVERRIDE;
    QModelIndex parent( const QModelIndex &index ) const Q_DECL_OVERRIDE;

    QModelIndex mapFromSource( const QModelIndex &sourceIndex ) const Q_DECL_OVERRIDE;
    QModelIndex mapToSource( const QModelIndex &proxyIndex ) const Q_DECL_OVERRIDE;

    void setSourceModel( QAbstractItemModel *model ) Q_DECL_OVERRIDE;

  public Q_SLOTS:
    /**
     * Sets whether the proxy model shall group
     * the annotations by page.
     */
    void groupByPage( bool value );

  private Q_SLOTS:
    void rebuildIndexes();

  private:
    bool mGroupByPage;
    QList<QModelIndex> mIndexes;
    QList<QPair< QModelIndex, QList<QModelIndex> > > mTreeIndexes;
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
        explicit AuthorGroupProxyModel( QObject *parent = nullptr );
        ~AuthorGroupProxyModel();

        int columnCount( const QModelIndex &parentIndex ) const Q_DECL_OVERRIDE;
        int rowCount( const QModelIndex &parentIndex ) const Q_DECL_OVERRIDE;

        QModelIndex index( int row, int column, const QModelIndex &parentIndex = QModelIndex() ) const Q_DECL_OVERRIDE;
        QModelIndex parent( const QModelIndex &index ) const Q_DECL_OVERRIDE;

        QModelIndex mapFromSource( const QModelIndex &sourceIndex ) const Q_DECL_OVERRIDE;
        QModelIndex mapToSource( const QModelIndex &proxyIndex ) const Q_DECL_OVERRIDE;

        void setSourceModel( QAbstractItemModel *model ) Q_DECL_OVERRIDE;

        QItemSelection mapSelectionToSource(const QItemSelection &selection) const Q_DECL_OVERRIDE;
        QItemSelection mapSelectionFromSource(const QItemSelection &selection) const Q_DECL_OVERRIDE;
        QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
        QMap<int, QVariant> itemData(const QModelIndex &index) const Q_DECL_OVERRIDE;
        Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

    public Q_SLOTS:
        /**
         * Sets whether the proxy model shall group
         * the annotations by author.
         */
        void groupByAuthor( bool value );

    private Q_SLOTS:
        void rebuildIndexes();

    private:
        class Private;
        Private* const d;
};

#endif
