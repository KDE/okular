/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "signaturepanel.h"

#include "pageview.h"
#include "signatureguiutils.h"
#include "signaturemodel.h"
#include "revisionviewer.h"
#include "signaturepropertiesdialog.h"

#include <KLocalizedString>

#include <QMenu>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QApplication>

#include "core/form.h"
#include "core/document.h"

class SignaturePanelPrivate
{
    public:
        Okular::FormFieldSignature *getSignatureFromId( int formId );

        QHash<int, Okular::FormFieldSignature *> m_signatureForms;
        Okular::Document *m_document;
        Okular::FormFieldSignature *m_currentForm;
        QTreeView *m_view;
        SignatureModel *m_model;
        PageView *m_pageView;
};

SignaturePanel::SignaturePanel( QWidget *parent, Okular::Document *document )
    : QWidget( parent ), d_ptr( new SignaturePanelPrivate )
{
    Q_D( SignaturePanel );
    d->m_view = new QTreeView( this );
    d->m_view->setAlternatingRowColors( true );
    d->m_view->setSelectionMode( QAbstractItemView::ExtendedSelection );
    d->m_view->header()->hide();

    d->m_document = document;
    d->m_model = new SignatureModel( d->m_document, this );

    d->m_view->setModel( d->m_model );
    connect( d->m_view->selectionModel(), &QItemSelectionModel::currentChanged, this, &SignaturePanel::activated );
    connect( d->m_view, &QTreeView::pressed, this, &SignaturePanel::slotShowContextMenu );

    auto vLayout = new QVBoxLayout( this );
    vLayout->setMargin( 0 );
    vLayout->setSpacing( 6 );
    vLayout->addWidget( d->m_view );
}

Okular::FormFieldSignature *SignaturePanelPrivate::getSignatureFromId( int formId )
{
    if ( formId == -1 )
        return nullptr;
    return m_signatureForms[formId];
}

void SignaturePanel::activated( const QModelIndex &index )
{
    Q_D( SignaturePanel );
    int formId = d->m_model->data( index, SignatureModel::FormIdRole ).toInt();
    d->m_currentForm = d->getSignatureFromId( formId );
    if ( !d->m_currentForm )
        return;
    Okular::NormalizedRect nr = d->m_currentForm->rect();
    Okular::DocumentViewport vp;
    vp.pageNumber = d->m_model->data( index, SignatureModel::PageRole ).toInt();
    vp.rePos.enabled = true;
    vp.rePos.pos = Okular::DocumentViewport::Center;
    vp.rePos.normalizedX = ( nr.right + nr.left ) / 2.0;
    vp.rePos.normalizedY = ( nr.bottom + nr.top ) / 2.0;
    d->m_document->setViewport( vp, nullptr );
    d->m_pageView->highlightSignatureFormWidget( formId );
}

void SignaturePanel::slotShowContextMenu()
{
    Q_D( SignaturePanel );
    if ( !d->m_currentForm )
        return;

    Qt::MouseButtons buttons = QApplication::mouseButtons();
    if ( buttons == Qt::RightButton )
    {
        QMenu *menu = new QMenu( this );
        QAction *sigRev = new QAction( i18n("View Revision"), menu );
        QAction *sigProp = new QAction( i18n("View Properties"), menu );
        connect( sigRev, &QAction::triggered, this, &SignaturePanel::slotViewRevision );
        connect( sigProp, &QAction::triggered, this, &SignaturePanel::slotViewProperties );
        menu->addAction( sigRev );
        menu->addAction( sigProp );
        menu->exec( QCursor::pos() );
        delete menu;
    }
}

void SignaturePanel::slotViewRevision()
{
    Q_D( SignaturePanel );
    QByteArray data;
    d->m_document->requestSignedRevisionData( d->m_currentForm->validate(), &data );
    RevisionViewer viewer( data, this );
    viewer.viewRevision();
}

void SignaturePanel::slotViewProperties()
{
    Q_D( SignaturePanel );
    SignaturePropertiesDialog propDlg( d->m_document, d->m_currentForm, this );
    propDlg.exec();
}

void SignaturePanel::notifySetup( const QVector<Okular::Page *> &/*pages*/, int setupFlags )
{
    if ( !( setupFlags & Okular::DocumentObserver::UrlChanged ) )
        return;

    Q_D( SignaturePanel );
    QVector<Okular::FormFieldSignature *> signatureForms = SignatureGuiUtils::getSignatureFormFields( d->m_document, true, -1 );
    foreach ( auto sf, signatureForms )
    {
        d->m_signatureForms[sf->id()] = sf;
    }
}

SignaturePanel::~SignaturePanel()
{
    Q_D( SignaturePanel );
    d->m_document->removeObserver( this );
    delete d->m_model;
}

void SignaturePanel::setPageView( PageView *pv )
{
    Q_D( SignaturePanel );
    d->m_pageView = pv;
}

#include "moc_signaturepanel.cpp"
