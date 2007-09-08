/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "annotationmodel.h"

#include <qlinkedlist.h>
#include <qlist.h>

#include <kicon.h>

#include "core/annotations.h"
#include "core/document.h"
#include "core/observer.h"
#include "core/page.h"

struct AnnotationItem
{
    AnnotationItem();
    AnnotationItem( AnnotationItem *parent, Okular::Annotation *ann );
    AnnotationItem( AnnotationItem *parent, int page );
    ~AnnotationItem();

    AnnotationItem *parent;
    QList< AnnotationItem* > children;

    Okular::Annotation *annotation;
    int page;
};


class AnnotationModelPrivate : public Okular::DocumentObserver
{
public:
    AnnotationModelPrivate( AnnotationModel *qq );
    virtual ~AnnotationModelPrivate();

    virtual uint observerId() const;
    virtual void notifySetup( const QVector< Okular::Page * > &pages, bool documentChanged );

    QModelIndex indexForItem( AnnotationItem *item ) const;
    void rebuildTree( const QVector< Okular::Page * > &pages );

    AnnotationModel *q;
    AnnotationItem *root;
    Okular::Document *document;
};


AnnotationItem::AnnotationItem()
    : parent( 0 ), annotation( 0 ), page( -1 )
{
}

AnnotationItem::AnnotationItem( AnnotationItem *_parent, Okular::Annotation *ann )
    : parent( _parent ), annotation( ann ), page( _parent->page )
{
    Q_ASSERT( !parent->annotation );
    parent->children.append( this );
}

AnnotationItem::AnnotationItem( AnnotationItem *_parent, int _page )
    : parent( _parent ), annotation( 0 ), page( _page )
{
    Q_ASSERT( !parent->parent );
    parent->children.append( this );
}

AnnotationItem::~AnnotationItem()
{
    qDeleteAll( children );
}


AnnotationModelPrivate::AnnotationModelPrivate( AnnotationModel *qq )
    : q( qq ), root( new AnnotationItem )
{
}

AnnotationModelPrivate::~AnnotationModelPrivate()
{
    delete root;
}

uint AnnotationModelPrivate::observerId() const
{
    return ANNOTATIONMODEL_ID;
}

void AnnotationModelPrivate::notifySetup( const QVector< Okular::Page * > &pages, bool documentChanged )
{
    if ( !documentChanged )
        return;

    qDeleteAll( root->children );
    root->children.clear();
    q->reset();

    rebuildTree( pages );
}

QModelIndex AnnotationModelPrivate::indexForItem( AnnotationItem *item ) const
{
    if ( item->parent )
    {
        int id = item->parent->children.indexOf( item );
        if ( id >= 0 && id < item->parent->children.count() )
           return q->createIndex( id, 0, item );
    }
    return QModelIndex();
}

void AnnotationModelPrivate::rebuildTree( const QVector< Okular::Page * > &pages )
{
    emit q->layoutAboutToBeChanged();
    for ( int i = 0; i < pages.count(); ++i )
    {
        QLinkedList< Okular::Annotation* > annots = pages.at( i )->annotations();
        if ( annots.isEmpty() )
            continue;

        AnnotationItem *annItem = new AnnotationItem( root, i );
        QLinkedList< Okular::Annotation* >::ConstIterator it = annots.begin(), itEnd = annots.end();
        for ( ; it != itEnd; ++it )
        {
            new AnnotationItem( annItem, *it );
        }
    }
    emit q->layoutChanged();
}


AnnotationModel::AnnotationModel( Okular::Document *document, QObject *parent )
    : QAbstractItemModel( parent ), d( new AnnotationModelPrivate( this ) )
{
    d->document = document;

    d->document->addObserver( d );
}

AnnotationModel::~AnnotationModel()
{
    delete d;
}

int AnnotationModel::columnCount( const QModelIndex &parent ) const
{
    Q_UNUSED( parent )
    return 1;
}

QVariant AnnotationModel::data( const QModelIndex &index, int role ) const
{
    if ( !index.isValid() )
        return QVariant();

    AnnotationItem *item = static_cast< AnnotationItem* >( index.internalPointer() );
    if ( !item->annotation )
    {
        if ( role == Qt::DisplayRole )
           return item->page;
        return QVariant();
    }
    switch ( role )
    {
        case Qt::DisplayRole:
            return item->annotation->author();
            break;
        case Qt::DecorationRole:
            return KIcon( "okular" );
            break;
        case AuthorRole:
            return item->annotation->author();
            break;
        case PageRole:
            return item->page;
            break;
    }
    return QVariant();
}

bool AnnotationModel::hasChildren( const QModelIndex &parent ) const
{
    if ( !parent.isValid() )
        return true;

    AnnotationItem *item = static_cast< AnnotationItem* >( parent.internalPointer() );
    return !item->children.isEmpty();
}

QVariant AnnotationModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation != Qt::Horizontal )
        return QVariant();

    if ( section == 0 && role == Qt::DisplayRole )
        return "Annotations";

    return QVariant();
}

QModelIndex AnnotationModel::index( int row, int column, const QModelIndex &parent ) const
{
    if ( row < 0 || column != 0 )
        return QModelIndex();

    AnnotationItem *item = parent.isValid() ? static_cast< AnnotationItem* >( parent.internalPointer() ) : d->root;
    if ( row < item->children.count() )
        return createIndex( row, column, item->children.at( row ) );

    return QModelIndex();
}

QModelIndex AnnotationModel::parent( const QModelIndex &index ) const
{
    if ( !index.isValid() )
        return QModelIndex();

    AnnotationItem *item = static_cast< AnnotationItem* >( index.internalPointer() );
    return d->indexForItem( item->parent );
}

int AnnotationModel::rowCount( const QModelIndex &parent ) const
{
    AnnotationItem *item = parent.isValid() ? static_cast< AnnotationItem* >( parent.internalPointer() ) : d->root;
    return item->children.count();
}

bool AnnotationModel::isAnnotation( const QModelIndex &index ) const
{
    return annotationForIndex( index );
}

Okular::Annotation* AnnotationModel::annotationForIndex( const QModelIndex &index ) const
{
    if ( !index.isValid() )
        return 0;

    AnnotationItem *item = static_cast< AnnotationItem* >( index.internalPointer() );
    return item->annotation;
}

#include "annotationmodel.moc"
