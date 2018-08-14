/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "signaturemodel.h"

#include <QVector>
#include <QIcon>
#include <QPointer>
#include <KLocalizedString>

#include "core/annotations.h"
#include "core/document.h"
#include "core/observer.h"
#include "core/page.h"
#include "core/form.h"
#include "core/signatureutils.h"
#include "guiutils.h"

static QVector<Okular::FormFieldSignature*> getSignatureFormFields( Okular::Page* page )
{
    QVector<Okular::FormFieldSignature*> signatureFormFields;
    foreach ( Okular::FormField *f, page->formFields() )
    {
        if ( f->type() == Okular::FormField::FormSignature )
        {
            signatureFormFields.append( static_cast<Okular::FormFieldSignature*>( f ) );
        }
    }
    return signatureFormFields;
}

struct SignatureItem
{
    enum ItemData
    {
        RevisionInfo,
        SignatureValidity,
        SigningTime,
        Reason,
        FieldInfo,
        Other
    };

    SignatureItem();
    SignatureItem( SignatureItem *parent, Okular::FormFieldSignature *form, ItemData data, int page , int _pos );
    //SignatureItem( SignatureItem *parent, int page, int pos );
    ~SignatureItem();

    SignatureItem *parent;
    QVector<SignatureItem*> children;
    Okular::FormFieldSignature *signatureForm;
    QString displayString;
    ItemData itemData;
    int page;
    int pos;
};

SignatureItem::SignatureItem()
    : parent( nullptr ), signatureForm( nullptr ), itemData( Other ), page( -1 ), pos( -1 )
{
}

SignatureItem::SignatureItem(SignatureItem *_parent, Okular::FormFieldSignature *form, ItemData data , int _page, int _pos )
    : parent( _parent ), signatureForm( form ), itemData( data ), page( _page ), pos( _pos )
{
    Q_ASSERT( parent );
    parent->children.append( this );

    Okular::SignatureInfo *info = form->validate();
    switch ( itemData )
    {
        case RevisionInfo:
            displayString = i18n("Rev. %1: Signed By %2", pos, info->signerName() );
            break;
        case SignatureValidity:
            displayString = GuiUtils::getReadableSigState( info->signatureStatus() );
            break;
        case SigningTime:
            displayString = i18n("Signing Time: %1", info->signingTime().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) );
            break;
        case Reason: {
            const QString reason = info->reason();
            displayString = i18n("Reason: %1", !reason.isEmpty() ? reason : i18n("Not Available") );
            break;
        }
        case FieldInfo:
            displayString = i18n("Field: %1 on page %2", form->name(), page+1 );
            break;
        case Other: break;
    }
}

/*SignatureItem::SignatureItem( SignatureItem *_parent, int _page, int _pos )
    : parent( _parent ), signatureForm( nullptr ), page( _page ), pos( _pos )
{
    Q_ASSERT( !parent->parent );
    parent->children.append( this );
    displayString = "pradhan";
}*/

SignatureItem::~SignatureItem()
{
    qDeleteAll( children );
}

class SignatureModelPrivate : public Okular::DocumentObserver
{
public:
    SignatureModelPrivate( SignatureModel *qq );
    ~SignatureModelPrivate() override;

    void notifySetup( const QVector< Okular::Page * > &pages, int setupFlags ) override;

    QModelIndex indexForItem( SignatureItem *item ) const;
    void rebuildTree( const QVector< Okular::Page * > &pages );
    SignatureItem* findItem( int page, int *index ) const;

    SignatureModel *q;
    SignatureItem *root;
    QPointer<Okular::Document> document;
};

SignatureModelPrivate::SignatureModelPrivate( SignatureModel *qq )
    : q( qq ), root( new SignatureItem )
{
}

SignatureModelPrivate::~SignatureModelPrivate()
{
    delete root;
}

static void updateFormFieldSignaturePointer( SignatureItem *item, const QVector<Okular::Page*> &pages )
{
    if ( item->signatureForm )
    {
        foreach ( Okular::FormField *f, pages[item->page]->formFields() )
        {
            if ( item->signatureForm->id() == f->id() )
            {
                item->signatureForm = static_cast<Okular::FormFieldSignature*>( f );
                break;
            }
        }
        if ( !item->signatureForm )
            qWarning() << "Lost signature form field, something went wrong";
    }

    foreach ( SignatureItem *child, item->children )
        updateFormFieldSignaturePointer( child, pages );
}

void SignatureModelPrivate::notifySetup( const QVector<Okular::Page*> &pages, int setupFlags )
{
    if ( !( setupFlags & Okular::DocumentObserver::DocumentChanged ) )
    {
        if ( setupFlags & Okular::DocumentObserver::UrlChanged )
        {
            updateFormFieldSignaturePointer( root, pages );
        }
        return;
    }

    q->beginResetModel();
    qDeleteAll( root->children );
    root->children.clear();
    rebuildTree( pages );
    q->endResetModel();
}

QModelIndex SignatureModelPrivate::indexForItem( SignatureItem *item ) const
{
    if ( item->parent )
    {
        int index = item->parent->children.indexOf( item );
        if ( index >= 0 && index < item->parent->children.count() )
           return q->createIndex( index, 0, item );
    }
    return QModelIndex();
}

void SignatureModelPrivate::rebuildTree( const QVector< Okular::Page*> &pages )
{
    if ( pages.isEmpty() )
        return;

    emit q->layoutAboutToBeChanged();
    for ( int i = 0; i < pages.count(); i++ )
    {
        QVector<Okular::FormFieldSignature*> signatureFormFields = getSignatureFormFields( pages.at( i ) );
        if ( signatureFormFields.isEmpty() )
            continue;

        int pos = 1;
        foreach ( auto sf, signatureFormFields )
        {
            SignatureItem *sigItem = new SignatureItem( root, sf, SignatureItem::RevisionInfo, i, pos );
            new SignatureItem( sigItem, sf, SignatureItem::SignatureValidity, i, pos );
            new SignatureItem( sigItem, sf, SignatureItem::SigningTime, i, pos );
            new SignatureItem( sigItem, sf, SignatureItem::Reason, i, pos );
            new SignatureItem( sigItem, sf, SignatureItem::FieldInfo, i, pos );
            pos++;
        }
    }
    emit q->layoutChanged();
}

SignatureItem *SignatureModelPrivate::findItem( int page, int *index ) const
{
    for ( int i = 0; i < root->children.count(); i++ )
    {
        SignatureItem *tmp = root->children.at( i );
        if ( tmp->page == page )
        {
            if ( index )
                *index = i;
            return tmp;
        }
    }
    if ( index )
        *index = -1;
    return nullptr;
}


SignatureModel::SignatureModel( Okular::Document *doc, QObject *parent )
    : QAbstractItemModel( parent ), d_ptr( new SignatureModelPrivate( this ) )
{
    Q_D( SignatureModel );
    d->document = doc;
    d->document->addObserver( d );
}

SignatureModel::~SignatureModel()
{
    Q_D( SignatureModel );
    if ( d->document )
        d->document->removeObserver( d );
}

int SignatureModel::columnCount( const QModelIndex & ) const
{
    return 1;
}

QVariant SignatureModel::data( const QModelIndex &index, int role ) const
{
    if ( !index.isValid() )
        return QVariant();

    SignatureItem *item = static_cast<SignatureItem*>( index.internalPointer() );
    if ( item->signatureForm )
    {
        switch ( role )
        {
            case Qt::DisplayRole:
            case Qt::ToolTipRole:
                return item->displayString;
            case Qt::DecorationRole:
                if ( item->itemData == SignatureItem::RevisionInfo )
                    return QIcon::fromTheme( QStringLiteral("dialog-yes") );
                return QIcon();
            case FormRole:
                if ( item->itemData == SignatureItem::RevisionInfo )
                    return item->signatureForm->id();
                return -1;
            case PageRole:
                return item->page;
        }
    }

    return QVariant();
}

bool SignatureModel::hasChildren( const QModelIndex &parent ) const
{
    if ( !parent.isValid() )
        return true;

    SignatureItem *item = static_cast<SignatureItem*>( parent.internalPointer() );
    return !item->children.isEmpty();
}

QModelIndex SignatureModel::index( int row, int column, const QModelIndex &parent ) const
{
    Q_D( const SignatureModel );

    if ( row < 0 || column != 0 )
        return QModelIndex();

    SignatureItem *item = parent.isValid() ? static_cast<SignatureItem*>( parent.internalPointer() ) : d->root;
    if ( row < item->children.count() )
        return createIndex( row, column, item->children.at( row ) );

    return QModelIndex();
}

QModelIndex SignatureModel::parent( const QModelIndex &index ) const
{
    Q_D( const SignatureModel );

    if ( !index.isValid() )
        return QModelIndex();

    SignatureItem *item = static_cast<SignatureItem*>( index.internalPointer() );
    return d->indexForItem( item->parent );
}

int SignatureModel::rowCount( const QModelIndex &parent ) const
{
    Q_D( const SignatureModel );

    SignatureItem *item = parent.isValid() ? static_cast<SignatureItem*>( parent.internalPointer() ) : d->root;
    return item->children.count();
}

#include "moc_signaturemodel.cpp"
