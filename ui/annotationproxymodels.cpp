/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QList>

#include "annotationmodel.h"

#include "annotationproxymodels.h"

PageFilterProxyModel::PageFilterProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    mGroupByCurrentPage( false ),
    mCurrentPage( -1 )
{
  setDynamicSortFilter( true );
}

void PageFilterProxyModel::groupByCurrentPage( bool value )
{
  if ( mGroupByCurrentPage == value )
    return;

  mGroupByCurrentPage = value;

  invalidateFilter();

  emit layoutChanged();
}

void PageFilterProxyModel::setCurrentPage( int page )
{
  if ( mCurrentPage == page )
    return;

  mCurrentPage = page;

  invalidateFilter();

  emit layoutChanged();
}

bool PageFilterProxyModel::filterAcceptsRow( int row, const QModelIndex &sourceParent ) const
{
  if ( !mGroupByCurrentPage )
    return true;

  const QModelIndex pageIndex = sourceModel()->index( row, 0, sourceParent );
  int page = sourceModel()->data( pageIndex, AnnotationModel::PageRole ).toInt();

  return (page == mCurrentPage);
}


PageGroupProxyModel::PageGroupProxyModel( QObject *parent )
  : QAbstractProxyModel( parent ),
    mGroupByPage( false )
{
}

int PageGroupProxyModel::columnCount( const QModelIndex& ) const
{
  // For top-level and second level we have always only one column
  return 1;
}

int PageGroupProxyModel::rowCount( const QModelIndex &parentIndex ) const
{
  if ( mGroupByPage ) {
    if ( parentIndex.isValid() ) {
      if ( parentIndex.parent().isValid() )
        return 0;
      else {
        return mTreeIndexes[ parentIndex.row() ].second.count(); // second-level
      }
    } else {
      return mTreeIndexes.count(); // top-level
    }
  } else {
    if ( !parentIndex.isValid() ) // top-level
      return mIndexes.count();
    else
      return 0;
  }
}

QModelIndex PageGroupProxyModel::index( int row, int column, const QModelIndex &parentIndex ) const
{
  if ( mGroupByPage ) {
    if ( parentIndex.isValid() )
      return createIndex( row, column, qint32( parentIndex.row() + 1 ) );
    else
      return createIndex( row, column, 0 );
  } else {
    return createIndex( row, column, 0 );
  }
}

QModelIndex PageGroupProxyModel::parent( const QModelIndex &idx ) const
{
  if ( mGroupByPage ) {
    if ( idx.internalId() == 0 ) // top-level
      return QModelIndex();
    else
      return index( idx.internalId() - 1, 0 );
  } else {
    // We have only top-level items
    return QModelIndex();
  }
}

QModelIndex PageGroupProxyModel::mapFromSource( const QModelIndex &sourceIndex ) const
{
  if ( mGroupByPage ) {
    if ( sourceIndex.parent().isValid() ) {
      return index( sourceIndex.row(), sourceIndex.column(), sourceIndex.parent() );
    } else {
      return index( sourceIndex.row(), sourceIndex.column() );
    }
  } else {
    for ( int i = 0; i < mIndexes.count(); ++i ) {
      if ( mIndexes[ i ] == sourceIndex )
        return index( i, 0 );
    }

    return QModelIndex();
  }
}

QModelIndex PageGroupProxyModel::mapToSource( const QModelIndex &proxyIndex ) const
{
  if ( mGroupByPage ) {
    if ( proxyIndex.internalId() == 0 ) {

      if ( proxyIndex.row() >= mTreeIndexes.count() )
        return QModelIndex();

      return mTreeIndexes[ proxyIndex.row() ].first;
    } else {
      if ( proxyIndex.internalId() - 1 >= mTreeIndexes.count() ||
           proxyIndex.row() >= mTreeIndexes[ proxyIndex.internalId() - 1 ].second.count() )
        return QModelIndex();

      return mTreeIndexes[ proxyIndex.internalId() - 1 ].second[ proxyIndex.row() ];
    }
  } else {
    if ( proxyIndex.column() > 0 || proxyIndex.row() >= mIndexes.count() )
      return QModelIndex();
    else {
      return mIndexes[ proxyIndex.row() ];
    }
  }
}

void PageGroupProxyModel::setSourceModel( QAbstractItemModel *model )
{
  if ( sourceModel() ) {
    disconnect( sourceModel(), SIGNAL( layoutChanged() ), this, SLOT( rebuildIndexes() ) );
    disconnect( sourceModel(), SIGNAL( modelReset() ), this, SLOT( rebuildIndexes() ) );
    disconnect( sourceModel(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rebuildIndexes() ) );
    disconnect( sourceModel(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( rebuildIndexes() ) );
  }

  QAbstractProxyModel::setSourceModel( model );

  connect( sourceModel(), SIGNAL( layoutChanged() ), this, SLOT( rebuildIndexes() ) );
  connect( sourceModel(), SIGNAL( modelReset() ), this, SLOT( rebuildIndexes() ) );
  connect( sourceModel(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rebuildIndexes() ) );
  connect( sourceModel(), SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( rebuildIndexes() ) );

  rebuildIndexes();
}

void PageGroupProxyModel::rebuildIndexes()
{
  emit layoutAboutToBeChanged();

  if ( mGroupByPage ) {
    mTreeIndexes.clear();

    for ( int row = 0; row < sourceModel()->rowCount(); ++row ) {
      const QModelIndex pageIndex = sourceModel()->index( row, 0 );

      QList<QModelIndex> itemIndexes;
      for ( int subRow = 0; subRow < sourceModel()->rowCount( pageIndex ); ++subRow ) {
        itemIndexes.append( sourceModel()->index( subRow, 0, pageIndex ) );
      }

      mTreeIndexes.append( QPair<QModelIndex, QList<QModelIndex> >( pageIndex, itemIndexes ) );
    }
  } else {
    mIndexes.clear();

    for ( int row = 0; row < sourceModel()->rowCount(); ++row ) {
      const QModelIndex pageIndex = sourceModel()->index( row, 0 );
      for ( int subRow = 0; subRow < sourceModel()->rowCount( pageIndex ); ++subRow ) {
        mIndexes.append( sourceModel()->index( subRow, 0, pageIndex ) );
      }
    }
  }

  emit layoutChanged();
}

void PageGroupProxyModel::groupByPage( bool value )
{
  if ( mGroupByPage == value )
    return;

  mGroupByPage = value;

  rebuildIndexes();
}

#include "annotationproxymodels.moc"
