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

#include <klocalizedstring.h>

#include <poppler-form.h>

#include <QEvent>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>

CertificateTools::CertificateTools(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hBoxLayout = new QHBoxLayout(this);
    m_tree = new QTreeWidget(this);
    hBoxLayout->addWidget(m_tree);
    m_tree->setHeaderLabels({i18nc("Name of the person to whom the cerficate was issued", "Issued to"), i18n("E-mail"), i18nc("Certificate expiration date", "Expiration date")});
    m_tree->setRootIsDecorated(false);

    connect(CertificateSettings::self(), &CertificateSettings::useDefaultDBChanged, this, &CertificateTools::warnRestartNeeded);
    connect(CertificateSettings::self(), &CertificateSettings::dBCertificatePathChanged, this, [this] {
        if (!CertificateSettings::useDefaultDB()) {
            warnRestartNeeded();
        }
    });
}

bool CertificateTools::event(QEvent *e)
{
    if (e->type() == QEvent::Paint && !m_certificatesAsked) {
        m_certificatesAsked = true;

        const QVector<Poppler::CertificateInfo> nssCerts = Poppler::getAvailableSigningCertificates();
        foreach (auto cert, nssCerts) {
            new QTreeWidgetItem(m_tree, {cert.subjectInfo(Poppler::CertificateInfo::EntityInfoKey::CommonName), cert.subjectInfo(Poppler::CertificateInfo::EntityInfoKey::EmailAddress), cert.validityEnd().toString("yyyy-MM-dd")});
        }

        m_tree->resizeColumnToContents(1);
        m_tree->resizeColumnToContents(0);
    }
    return QWidget::event(e);
}

void CertificateTools::warnRestartNeeded()
{
    if (!m_warnedAboutRestart) {
        m_warnedAboutRestart = true;
        QMessageBox::information(this, i18n("Restart needed"), i18n("You need to restart Okular after changing the NSS directory settings"));
    }
}
