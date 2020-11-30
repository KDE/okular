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
#include "pdfsignatureutils.h"

#include <KLocalizedString>
#include <KUrlRequester>

#include <QEvent>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>

CertificateTools::CertificateTools(QWidget *parent)
    : QWidget(parent)
{
    if (Poppler::hasNSSSupport()) {
        m_certsw.setupUi(this);
        m_certsw.loadSignaturesButton->hide();

        KUrlRequester *pDlg = new KUrlRequester();
        pDlg->setObjectName(QStringLiteral("kcfg_DBCertificatePath"));
        pDlg->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
        pDlg->setEnabled(false);
        m_certsw.formLayout->setWidget(1, QFormLayout::FieldRole, pDlg);

        connect(m_certsw.customRadioButton, &QRadioButton::toggled, pDlg, &KUrlRequester::setEnabled);

        if (!CertificateSettings::useDefaultDB()) {
            m_certsw.customRadioButton->setChecked(true);
            m_certsw.defaultLabel->setVisible(false);
        }

        m_tree = new QTreeWidget(this);
        m_tree->setHeaderLabels({i18nc("Name of the person to whom the cerficate was issued", "Issued to"), i18n("E-mail"), i18nc("Certificate expiration date", "Expiration date")});
        m_tree->setRootIsDecorated(false);

        m_certsw.certificatesPlaceholder->addWidget(m_tree);

        connect(CertificateSettings::self(), &CertificateSettings::useDefaultDBChanged, this, &CertificateTools::warnRestartNeeded);
        connect(CertificateSettings::self(), &CertificateSettings::dBCertificatePathChanged, this, [this] {
            if (!CertificateSettings::useDefaultDB()) {
                warnRestartNeeded();
            }
        });
        connect(m_certsw.loadSignaturesButton, &QPushButton::clicked, this, [this] {
            m_certificatesAsked = false;
            update();
        });
    } else {
        m_tree = nullptr;
        QHBoxLayout *lay = new QHBoxLayout(this);
        QLabel *l = new QLabel(i18n("You are using a Poppler library built without NSS support.\nAdding Digital Signatures isn't available for that reason"));
        l->setWordWrap(true);
        lay->addWidget(l);
    }
}

bool CertificateTools::event(QEvent *e)
{
    if (m_tree && e->type() == QEvent::Paint && !m_certificatesAsked) {
        m_certificatesAsked = true;

        PopplerCertificateStore st;
        bool userCancelled;
        const QList<Okular::CertificateInfo *> certs = st.signingCertificates(&userCancelled);

        m_certsw.loadSignaturesButton->setVisible(userCancelled);

        for (auto cert : certs) {
            new QTreeWidgetItem(m_tree, {cert->subjectInfo(Okular::CertificateInfo::EntityInfoKey::CommonName), cert->subjectInfo(Okular::CertificateInfo::EntityInfoKey::EmailAddress), cert->validityEnd().toString("yyyy-MM-dd")});
        }
        qDeleteAll(certs);

        m_certsw.defaultLabel->setText(Poppler::getNSSDir());

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
