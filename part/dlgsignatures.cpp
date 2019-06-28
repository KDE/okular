/***************************************************************************
 *   Copyright (C) 2019 by Bubli                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgsignatures.h"

#include "certificatetools.h"
#include "ui_dlgsignaturesbase.h"

#include <KConfigDialogManager>

DlgSignatures::DlgSignatures(QWidget *parent)
    : QWidget(parent)
{
    m_dlg = new Ui_DlgSignaturesBase();
    m_dlg->setupUi(this);

    CertificateTools * kcfg_CertTools = new CertificateTools( m_dlg->certificatesGroup );
    m_dlg->certificatesPlaceholder->addWidget( kcfg_CertTools );
    kcfg_CertTools->setObjectName( QStringLiteral("kcfg_Certificates") );

    KConfigDialogManager::changedMap()->insert( QStringLiteral("CertificateTools"), SIGNAL(changed()) );
}

DlgSignatures::~DlgSignatures()
{
    delete m_dlg;
}
#include "moc_dlgsignatures.cpp"
