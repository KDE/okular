/***************************************************************************
 *   Copyright (C) 2019 by Bubli                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "certificatetools.h"
#include <iostream>
#include <klocalizedstring.h>
#include "settings.h"

#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>

CertificateTools::CertificateTools( QWidget * parent )
    : WidgetConfigurationToolsBase( parent )
{
}

QStringList CertificateTools::tools() const
{
    QStringList res;

    const int count = m_list->count();
    for ( int i = 0; i < count; ++i )
    {
        QListWidgetItem * listEntry = m_list->item(i);

        res << listEntry->text();
    }

    return res;
}

void CertificateTools::setTools(const QStringList& /*items*/)
{
    m_list->clear();

    QStringList certs = Okular::Settings::certificates();
    foreach( const QString cert, certs )
    {
        QListWidgetItem * listEntry = new QListWidgetItem( cert, m_list );
    }

    updateButtons();
}

void CertificateTools::slotAdd()
{
    QString certCN = QInputDialog::getText( this, i18n("Enter Certificate CN"), i18n("CertificateCN"), QLineEdit::Normal, QString() );

    if (certCN.isEmpty())
        return;

    // Create list entry
    QListWidgetItem * listEntry = new QListWidgetItem( certCN, m_list );

    // Select and scroll
    m_list->setCurrentItem( listEntry );
    m_list->scrollToItem( listEntry );
    updateButtons();
    emit changed();
}

void CertificateTools::slotEdit()
{
    QListWidgetItem *listEntry = m_list->currentItem();

    QString certCN = QInputDialog::getText( this, i18n("Change Certificate CN"), i18n("CertificateCN"), QLineEdit::Normal, listEntry->text() );
    listEntry->setText(certCN);

    // Select and scrolldd
    m_list->setCurrentItem( listEntry );
    m_list->scrollToItem( listEntry );
    updateButtons();
    emit changed();
}
