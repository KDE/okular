/***************************************************************************
 *   Copyright (C) 2019 by Bubli                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "certificatetools.h"
#include "certsettings.h"
#include <iostream>
#include <klocalizedstring.h>

#include <poppler-form.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

CertificateTools::CertificateTools(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hBoxLayout = new QHBoxLayout(this);
    m_list = new QListWidget(this);
    m_list->setIconSize(QSize(32, 32));
    hBoxLayout->addWidget(m_list);

    QVBoxLayout *vBoxLayout = new QVBoxLayout();
    m_btnAdd = new QPushButton(i18n("&Add..."), this);
    m_btnAdd->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    vBoxLayout->addWidget(m_btnAdd);
    m_btnEdit = new QPushButton(i18n("&Edit..."), this);
    m_btnEdit->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    m_btnEdit->setEnabled(false);
    vBoxLayout->addWidget(m_btnEdit);
    m_btnRemove = new QPushButton(i18n("&Remove"), this);
    m_btnRemove->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    m_btnRemove->setEnabled(false);
    vBoxLayout->addWidget(m_btnRemove);
    m_btnMoveUp = new QPushButton(i18n("Move &Up"), this);
    m_btnMoveUp->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
    m_btnMoveUp->setEnabled(false);
    vBoxLayout->addWidget(m_btnMoveUp);
    m_btnMoveDown = new QPushButton(i18n("Move &Down"), this);
    m_btnMoveDown->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
    m_btnMoveDown->setEnabled(false);
    vBoxLayout->addWidget(m_btnMoveDown);
    vBoxLayout->addStretch();
    hBoxLayout->addLayout(vBoxLayout);

    connect(m_list, &QListWidget::itemDoubleClicked, this, &CertificateTools::slotEdit);
    connect(m_list, &QListWidget::currentRowChanged, this, &CertificateTools::updateButtons);
    connect(m_btnAdd, &QPushButton::clicked, this, &CertificateTools::slotAdd);
    connect(m_btnEdit, &QPushButton::clicked, this, &CertificateTools::slotEdit);
    connect(m_btnRemove, &QPushButton::clicked, this, &CertificateTools::slotRemove);
    connect(m_btnMoveUp, &QPushButton::clicked, this, &CertificateTools::slotMoveUp);
    connect(m_btnMoveDown, &QPushButton::clicked, this, &CertificateTools::slotMoveDown);

    setCertificates(QStringList());
}

CertificateTools::~CertificateTools()
{
}

QStringList CertificateTools::certificates() const
{
    QStringList res;

    const int count = m_list->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem *listEntry = m_list->item(i);
        res << listEntry->data(Qt::UserRole).toString();
    }

    return res;
}

void CertificateTools::setCertificates(const QStringList & /*items*/)
{
    m_list->clear();

    /*  TODO: custom list of certs, perhaps from files? also permit ordering...
        QStringList certs = CertificateSettings::certificates();
        foreach( const QString cert, certs )
        {
            QListWidgetItem * listEntry = new QListWidgetItem( cert, m_list );
            (void)listEntry;
        }
    */
    Poppler::setNSSDir(CertificateSettings::certificatePath());
    const QVector<Poppler::CertificateInfo> nssCerts = Poppler::getAvailableSigningCertificates();
    foreach (auto cert, nssCerts) {
        QListWidgetItem *listEntry = new QListWidgetItem(
            cert.subjectInfo(Poppler::CertificateInfo::EntityInfoKey::CommonName) + "\t\t" + cert.subjectInfo(Poppler::CertificateInfo::EntityInfoKey::EmailAddress) + "\t\t(" + cert.validityEnd().toString("yyyy-MM-dd") + ")", m_list);

        QJsonObject json;
        json["NickName"] = cert.nickName();
        json["CommonName"] = cert.subjectInfo(Poppler::CertificateInfo::EntityInfoKey::CommonName);
        json["EMail"] = cert.subjectInfo(Poppler::CertificateInfo::EntityInfoKey::EmailAddress);
        json["ValidUntil"] = cert.validityEnd().toString();
        listEntry->setData(Qt::UserRole, QJsonDocument(json).toJson());
    }

    updateButtons();
}

void CertificateTools::slotAdd()
{
    QString certCN = QInputDialog::getText(this, i18n("Enter Certificate CN"), i18n("CertificateCN"), QLineEdit::Normal, QString());

    if (certCN.isEmpty())
        return;

    // Create list entry
    QListWidgetItem *listEntry = new QListWidgetItem(certCN, m_list);

    // Select and scroll
    m_list->setCurrentItem(listEntry);
    m_list->scrollToItem(listEntry);
    updateButtons();
    emit changed();
}

void CertificateTools::slotEdit()
{
    QListWidgetItem *listEntry = m_list->currentItem();

    bool ok;
    QString certCN = QInputDialog::getText(this, i18n("Change Certificate CN"), i18n("CertificateCN"), QLineEdit::Normal, listEntry->text(), &ok);

    if (ok) {
        listEntry->setText(certCN);

        // Select and scrolldd
        m_list->setCurrentItem(listEntry);
        m_list->scrollToItem(listEntry);
        updateButtons();
        emit changed();
    }
}

void CertificateTools::updateButtons()
{
    const int row = m_list->currentRow();
    const int last = m_list->count() - 1;

    m_btnEdit->setEnabled(row != -1);
    m_btnRemove->setEnabled(row != -1);
    m_btnMoveUp->setEnabled(row > 0);
    m_btnMoveDown->setEnabled(row != -1 && row != last);
}

void CertificateTools::slotRemove()
{
    const int row = m_list->currentRow();
    delete m_list->takeItem(row);
    updateButtons();
    emit changed();
}

void CertificateTools::slotMoveUp()
{
    const int row = m_list->currentRow();
    m_list->insertItem(row, m_list->takeItem(row - 1));
    m_list->scrollToItem(m_list->currentItem());
    updateButtons();
    emit changed();
}

void CertificateTools::slotMoveDown()
{
    const int row = m_list->currentRow();
    m_list->insertItem(row, m_list->takeItem(row + 1));
    m_list->scrollToItem(m_list->currentItem());
    updateButtons();
    emit changed();
}
