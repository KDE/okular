/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "signaturepanel.h"

#include <QPainter>
#include <QPaintEvent>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTreeView>

#include <KLocalizedString>

#include "core/document.h"
#include "core/page.h"
#include "core/form.h"
#include "guiutils.h"
#include "signaturemodel.h"
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
    connect(m_view, &QTreeView::activated, this, &SignaturePanel::activated);

    vLayout->addWidget( m_view );
}

void SignaturePanel::activated( const QModelIndex &index )
{
    int formId = m_model->data( index, SignatureModel::FormRole ).toInt();
    if ( formId == -1 )
        return;

    auto formFields = GuiUtils::getSignatureFormFields( m_document );
    Okular::FormFieldSignature *sf;
    foreach( auto f, formFields )
    {
        if ( f->id() == formId )
        {
            sf = f;
            break;
        }
    }
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

SignaturePanel::~SignaturePanel()
{
    delete m_model;
}

#include "moc_signaturepanel.cpp"
