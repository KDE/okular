/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "signaturepanel.h"

#include <QApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTreeView>
#include <QMenu>
#include <KLocalizedString>

#include "core/document.h"
#include "core/page.h"
#include "core/form.h"
#include "guiutils.h"
#include "signaturemodel.h"
#include "signaturewidgets.h"
#include <QDebug>

SignaturePanel::SignaturePanel( QWidget *parent, Okular::Document *document )
    : QWidget( parent ), m_document( document )
{
    auto vLayout = new QVBoxLayout( this );
    vLayout->setMargin( 0 );
    vLayout->setSpacing( 6 );

    m_view = new QTreeView( this );
    m_view->setAlternatingRowColors( true );
    m_view->setSelectionMode( QAbstractItemView::ExtendedSelection );
    m_view->header()->hide();

    m_model = new SignatureModel( m_document, this );

    m_view->setModel( m_model );
    connect( m_view, &QTreeView::activated, this, &SignaturePanel::activated );
    connect( m_view, &QTreeView::pressed, this, &SignaturePanel::showContextMenu );

    vLayout->addWidget( m_view );
}

static Okular::FormFieldSignature *getSignatureFromId( int formId, Okular::Document *doc )
{
    auto formFields = GuiUtils::getSignatureFormFields( doc );
    foreach( auto f, formFields )
    {
        if ( f->id() == formId )
        {
            return f;
        }
    }
    return nullptr;
}

void SignaturePanel::activated( const QModelIndex &index )
{
    int formId = m_model->data( index, SignatureModel::FormRole ).toInt();
    if ( formId == -1 )
        return;

    Okular::FormFieldSignature *sf = getSignatureFromId( formId, m_document );
    if ( !sf )
      return;

    Okular::NormalizedRect nr = sf->rect();
    Okular::DocumentViewport vp;
    vp.pageNumber = m_model->data( index, SignatureModel::PageRole ).toInt();
    vp.rePos.enabled = true;
    vp.rePos.pos = Okular::DocumentViewport::Center;
    vp.rePos.normalizedX = ( nr.right + nr.left ) / 2.0;
    vp.rePos.normalizedY = ( nr.bottom + nr.top ) / 2.0;
    m_document->setViewport( vp, nullptr, true );
}

void SignaturePanel::showContextMenu( const QModelIndex &index )
{
    Qt::MouseButtons buttons = QApplication::mouseButtons();
    if ( buttons == Qt::RightButton )
    {
        int formId = m_model->data( index, SignatureModel::FormRole ).toInt();
        if ( formId == -1 )
            return;

        Okular::FormFieldSignature *sf = getSignatureFromId( formId, m_document );
        if ( !sf )
          return;

        QMenu *menu = new QMenu( this );
        QAction *sigProp = new QAction( i18n("Validate Signature"), this );
        menu->addAction( sigProp );
        connect( sigProp, &QAction::triggered, this, [=]{
            SignaturePropertiesDialog sigSummaryDlg( m_document, sf, this );
            sigSummaryDlg.exec();
        } );
        QAction *sigRev = new QAction( i18n("View Revision"), this );
        menu->addAction( sigRev );
        connect( sigRev, &QAction::triggered, this, [=]{
            QByteArray data;
            m_document->requestSignedRevisionData( sf->validate(), &data );
            const QString tmpDir = QStandardPaths::writableLocation( QStandardPaths::TempLocation );
            QTemporaryFile tf( tmpDir + "/revision_XXXXXX.pdf" );
            tf.open();
            tf.write(data);
            RevisionViewer view( tf.fileName(), this);
            view.exec();
            tf.close();
        } );
        menu->exec( QCursor::pos() );
        delete menu;
    }
}

SignaturePanel::~SignaturePanel()
{
    delete m_model;
}

#include "moc_signaturepanel.cpp"
