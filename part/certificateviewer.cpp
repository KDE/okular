/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "certificateviewer.h"

#include "gui/certificatemodel.h"

#include <KColumnResizer>
#include <KLocalizedString>
#include <KMessageBox>

#include <QCryptographicHash>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>

#include "DistinguishedNameParser.h"
#include "gui/signatureguiutils.h"

QString splitDNAttributes(const QString &input)
{
    auto parsed = DN::parseString(input.toStdString());
    QStringList result;
    for (auto &&[key, value] : parsed) {
        result.push_back(QString::fromStdString(key) + QLatin1Char('=') + QString::fromStdString(value));
    }
    return result.join(QStringLiteral("\n"));
}

CertificateViewer::CertificateViewer(const Okular::CertificateInfo &certInfo, QWidget *parent)
    : KPageDialog(parent)
    , m_certificateInfo(certInfo)
{
    setModal(true);
    setMinimumSize(QSize(500, 500));
    setFaceType(Tabbed);
    setWindowTitle(i18n("Certificate Viewer"));
    setStandardButtons(QDialogButtonBox::Close);

    auto exportBtn = new QPushButton(i18n("Export…"));
    connect(exportBtn, &QPushButton::clicked, this, &CertificateViewer::exportCertificate);
    addActionButton(exportBtn);

    // General tab
    auto generalPage = new QFrame(this);
    addPage(generalPage, i18n("General"));

    auto generalPageLayout = new QVBoxLayout(generalPage);
    // force column 1 to have same width
    auto resizer = new KColumnResizer(this);

    if (m_certificateInfo.certificateType() == Okular::CertificateInfo::X509) {
        auto issuerBox = new QGroupBox(i18n("Issued By"), generalPage);
        auto issuerFormLayout = new QFormLayout(issuerBox);
        issuerFormLayout->setLabelAlignment(Qt::AlignLeft);
        issuerFormLayout->addRow(i18n("Common Name(CN)"), new QLabel(m_certificateInfo.issuerInfo(Okular::CertificateInfo::CommonName, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable)));
        issuerFormLayout->addRow(i18n("EMail"), new QLabel(m_certificateInfo.issuerInfo(Okular::CertificateInfo::EmailAddress, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable)));
        issuerFormLayout->addRow(i18n("Organization(O)"), new QLabel(m_certificateInfo.issuerInfo(Okular::CertificateInfo::Organization, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable)));
        generalPageLayout->addWidget(issuerBox);
        resizer->addWidgetsFromLayout(issuerBox->layout(), 0);
    }

    auto subjectBox = new QGroupBox(i18n("Issued To"), generalPage);
    auto subjectFormLayout = new QFormLayout(subjectBox);
    subjectFormLayout->setLabelAlignment(Qt::AlignLeft);
    subjectFormLayout->addRow(i18n("Common Name(CN)"), new QLabel(m_certificateInfo.subjectInfo(Okular::CertificateInfo::CommonName, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable)));
    subjectFormLayout->addRow(i18n("EMail"), new QLabel(m_certificateInfo.subjectInfo(Okular::CertificateInfo::EmailAddress, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable)));

    if (m_certificateInfo.certificateType() == Okular::CertificateInfo::X509) {
        subjectFormLayout->addRow(i18n("Organization(O)"), new QLabel(m_certificateInfo.subjectInfo(Okular::CertificateInfo::Organization, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable)));
    }
    generalPageLayout->addWidget(subjectBox);
    resizer->addWidgetsFromLayout(subjectBox->layout(), 0);

    auto validityBox = new QGroupBox(i18n("Validity"), generalPage);
    auto validityFormLayout = new QFormLayout(validityBox);
    validityFormLayout->setLabelAlignment(Qt::AlignLeft);
    validityFormLayout->addRow(i18n("Issued On"), new QLabel(QLocale().toString(m_certificateInfo.validityStart(), QLocale::LongFormat)));
    validityFormLayout->addRow(i18n("Expires On"), new QLabel(m_certificateInfo.validityEnd().isValid() ? QLocale().toString(m_certificateInfo.validityEnd(), QLocale::LongFormat) : i18nc("certificate that never expires", "Never")));
    generalPageLayout->addWidget(validityBox);
    resizer->addWidgetsFromLayout(validityBox->layout(), 0);

    auto fingerprintBox = new QGroupBox(i18n("Fingerprints"), generalPage);
    auto fingerprintFormLayout = new QFormLayout(fingerprintBox);
    fingerprintFormLayout->setLabelAlignment(Qt::AlignLeft);
    fingerprintFormLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    if (m_certificateInfo.certificateType() == Okular::CertificateInfo::X509) {
        QByteArray certData = m_certificateInfo.certificateData();
        auto sha1Label = new QLabel(QString::fromLatin1(QCryptographicHash::hash(certData, QCryptographicHash::Sha1).toHex(' ')));
        sha1Label->setWordWrap(true);
        auto sha256Label = new QLabel(QString::fromLatin1(QCryptographicHash::hash(certData, QCryptographicHash::Sha256).toHex(' ')));
        sha256Label->setWordWrap(true);
        fingerprintFormLayout->addRow(i18n("SHA-1 Fingerprint"), sha1Label);
        fingerprintFormLayout->addRow(i18n("SHA-256 Fingerprint"), sha256Label);
    } else if (m_certificateInfo.certificateType() == Okular::CertificateInfo::PGP) {
        fingerprintFormLayout->addRow(i18nc("certificate key", "Fingerprint"), new QLabel(m_certificateInfo.nickName()));
    }
    generalPageLayout->addWidget(fingerprintBox);
    resizer->addWidgetsFromLayout(fingerprintBox->layout(), 0);

    generalPageLayout->addStretch();

    // Details tab
    auto detailsFrame = new QFrame(this);
    addPage(detailsFrame, i18n("Details"));
    auto certDataLabel = new QLabel(i18n("Certificate Data:"));
    auto certTree = new QTreeView(this);
    certTree->setIndentation(0);
    m_certificateModel = new CertificateModel(m_certificateInfo, this);
    certTree->setModel(m_certificateModel);
    connect(certTree->selectionModel(), &QItemSelectionModel::currentChanged, this, &CertificateViewer::updateText);
    m_propertyText = new QTextEdit(this);
    m_propertyText->setReadOnly(true);

    auto detailsPageLayout = new QVBoxLayout(detailsFrame);
    detailsPageLayout->addWidget(certDataLabel);
    detailsPageLayout->addWidget(certTree);
    detailsPageLayout->addWidget(m_propertyText);
}

void CertificateViewer::updateText(const QModelIndex &index)
{
    QString text;
    const CertificateModel::Property key = m_certificateModel->data(index, CertificateModel::PropertyKeyRole).value<CertificateModel::Property>();
    switch (key) {
    case CertificateModel::SerialNumber:
    case CertificateModel::Version:
    case CertificateModel::IssuedOn:
    case CertificateModel::ExpiresOn:
        text = m_certificateModel->data(index, CertificateModel::PropertyVisibleValueRole).toString();
        break;
    case CertificateModel::Issuer:
    case CertificateModel::Subject:
        text = splitDNAttributes(m_certificateModel->data(index, CertificateModel::PropertyVisibleValueRole).toString());
        break;
    case CertificateModel::PublicKey:
        text = QString::fromLatin1(m_certificateInfo.publicKey().toHex(' '));
        break;
    case CertificateModel::KeyUsage:
        text = SignatureGuiUtils::getReadableKeyUsageNewLineSeparated(m_certificateInfo.keyUsageExtensions());
        break;
    case CertificateModel::IssuerName:
    case CertificateModel::IssuerEmail:
    case CertificateModel::IssuerOrganization:
    case CertificateModel::SubjectName:
    case CertificateModel::SubjectEmail:
    case CertificateModel::SubjectOrganization:
    case CertificateModel::Sha1:
    case CertificateModel::Sha256:
        Q_ASSERT(false);
        qWarning() << "Unused";
    }
    m_propertyText->setText(text);
}

void CertificateViewer::exportCertificate()
{
    const QString caption = i18n("Where do you want to save this certificate?");
    QString fileTypes;
    QString fileName;
    if (m_certificateInfo.certificateType() == Okular::CertificateInfo::PGP) {
        fileTypes = i18n("Certificate File (*.asc)");
        fileName = QStringLiteral("%1_public.asc").arg(m_certificateInfo.nickName());
    } else {
        fileTypes = i18n("Certificate File (*.cer)");
        fileName = QStringLiteral("Certificate.cer");
    }
    const QString path = QFileDialog::getSaveFileName(this, caption, fileName, fileTypes);
    if (!path.isEmpty()) {
        if (!m_certificateModel->exportCertificateTo(path)) {
            KMessageBox::error(this, i18n("Could not export the certificate"));
        }
    }
}
