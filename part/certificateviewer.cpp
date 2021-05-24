/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "certificateviewer.h"

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

#include "signatureguiutils.h"

// DN (DistinguishedName) attributes can be
//     C Country
//     CN Common name
//     DC Domain component
//     E E-mail address
//     EMAIL E-mail address (preferred)
//     EMAILADDRESS E-mail address
//     L Locality
//     O Organization name
//     OU Organizational unit name
//     PC Postal code
//     S State or province
//     SN Family name
//     SP State or province
//     ST State or province (preferred)
//     STREET Street
//     T Title

// CN=James Hacker,
//    L=Basingstoke,
//    O=Widget Inc,
//    C=GB
// CN=L. Eagle, O="Sue, Grabbit and Runn", C=GB
// CN=L. Eagle, O=Sue\, Grabbit and Runn, C=GB

// This is a poor man's attempt at parsing DN, if it fails it is not a problem since it's only for display in a list
static QString splitDNAttributes(const QStringList &text)
{
    const QStringList attributes = {"C", "CN", "DC", "E", "EMAIL", "EMAILADDRESS", "L", "O", "OU", "PC", "S", "SN", "SP", "ST", "STREET", "T"};

    for (const QString &t : text) {
        for (const QString &attribute : attributes) {
            const QRegularExpression re(QStringLiteral("(.*),\\s*(%1=.*)").arg(attribute), QRegularExpression::DotMatchesEverythingOption);
            const QRegularExpressionMatch match = re.match(t);
            if (match.hasMatch()) {
                QStringList results = text;
                const int index = results.indexOf(t);
                results.removeAt(index);
                results.insert(index, match.captured(2));
                results.insert(index, match.captured(1));
                return splitDNAttributes(results);
            }
        }
    }

    // Clean escaped commas
    QStringList result = text;
    for (QString &t : result) {
        t.replace(QLatin1String("\\,"), QLatin1String(","));
    }

    // Clean up quoted attributes
    for (QString &t : result) {
        for (const QString &attribute : attributes) {
            const QRegularExpression re(QStringLiteral("%1=\"(.*)\"").arg(attribute));
            const QRegularExpressionMatch match = re.match(t);
            if (match.hasMatch()) {
                t = attribute + '=' + match.captured(1);
            }
        }
    }

    return result.join(QStringLiteral("\n"));
}

static QString splitDNAttributes(const QString &text)
{
    return splitDNAttributes(QStringList {text});
}

CertificateModel::CertificateModel(const Okular::CertificateInfo &certInfo, QObject *parent)
    : QAbstractTableModel(parent)
    , m_certificateInfo(certInfo)
{
    m_certificateProperties = {Version, SerialNumber, Issuer, IssuedOn, ExpiresOn, Subject, PublicKey, KeyUsage};
}

int CertificateModel::columnCount(const QModelIndex &) const
{
    return 2;
}

int CertificateModel::rowCount(const QModelIndex &) const
{
    return m_certificateProperties.size();
}

static QString propertyVisibleName(CertificateModel::Property p)
{
    switch (p) {
    case CertificateModel::Version:
        return i18n("Version");
    case CertificateModel::SerialNumber:
        return i18n("Serial Number");
    case CertificateModel::Issuer:
        return i18n("Issuer");
    case CertificateModel::IssuedOn:
        return i18n("Issued On");
    case CertificateModel::ExpiresOn:
        return i18n("Expires On");
    case CertificateModel::Subject:
        return i18nc("The person/company that made the signature", "Subject");
    case CertificateModel::PublicKey:
        return i18n("Public Key");
    case CertificateModel::KeyUsage:
        return i18n("Key Usage");
    }
    return QString();
}

static QString propertyVisibleValue(CertificateModel::Property p, const Okular::CertificateInfo &certInfo)
{
    switch (p) {
    case CertificateModel::Version:
        return i18n("V%1", QString::number(certInfo.version()));
    case CertificateModel::SerialNumber:
        return certInfo.serialNumber().toHex(' ');
    case CertificateModel::Issuer:
        return certInfo.issuerInfo(Okular::CertificateInfo::DistinguishedName);
    case CertificateModel::IssuedOn:
        return certInfo.validityStart().toString(Qt::DefaultLocaleLongDate);
    case CertificateModel::ExpiresOn:
        return certInfo.validityEnd().toString(Qt::DefaultLocaleLongDate);
    case CertificateModel::Subject:
        return certInfo.subjectInfo(Okular::CertificateInfo::DistinguishedName);
    case CertificateModel::PublicKey:
        return i18n("%1 (%2 bits)", SignatureGuiUtils::getReadablePublicKeyType(certInfo.publicKeyType()), certInfo.publicKeyStrength());
    case CertificateModel::KeyUsage:
        return SignatureGuiUtils::getReadableKeyUsageCommaSeparated(certInfo.keyUsageExtensions());
    }
    return QString();
}

QVariant CertificateModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_certificateProperties.count())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        switch (index.column()) {
        case 0:
            return propertyVisibleName(m_certificateProperties[row]);
        case 1:
            return propertyVisibleValue(m_certificateProperties[row], m_certificateInfo);
        default:
            return QString();
        }
    case PropertyKeyRole:
        return m_certificateProperties[row];
    case PropertyVisibleValueRole:
        return propertyVisibleValue(m_certificateProperties[row], m_certificateInfo);
    }

    return QVariant();
}

QVariant CertificateModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::TextAlignmentRole)
        return QVariant(Qt::AlignLeft);

    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case 0:
        return i18n("Property");
    case 1:
        return i18n("Value");
    default:
        return QVariant();
    }
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

    auto exportBtn = new QPushButton(i18n("Export..."));
    connect(exportBtn, &QPushButton::clicked, this, &CertificateViewer::exportCertificate);
    addActionButton(exportBtn);

    // General tab
    auto generalPage = new QFrame(this);
    addPage(generalPage, i18n("General"));

    auto issuerBox = new QGroupBox(i18n("Issued By"), generalPage);
    auto issuerFormLayout = new QFormLayout(issuerBox);
    issuerFormLayout->setLabelAlignment(Qt::AlignLeft);
    issuerFormLayout->addRow(i18n("Common Name(CN)"), new QLabel(m_certificateInfo.issuerInfo(Okular::CertificateInfo::CommonName)));
    issuerFormLayout->addRow(i18n("EMail"), new QLabel(m_certificateInfo.issuerInfo(Okular::CertificateInfo::EmailAddress)));
    issuerFormLayout->addRow(i18n("Organization(O)"), new QLabel(m_certificateInfo.issuerInfo(Okular::CertificateInfo::Organization)));

    auto subjectBox = new QGroupBox(i18n("Issued To"), generalPage);
    auto subjectFormLayout = new QFormLayout(subjectBox);
    subjectFormLayout->setLabelAlignment(Qt::AlignLeft);
    subjectFormLayout->addRow(i18n("Common Name(CN)"), new QLabel(m_certificateInfo.subjectInfo(Okular::CertificateInfo::CommonName)));
    subjectFormLayout->addRow(i18n("EMail"), new QLabel(m_certificateInfo.subjectInfo(Okular::CertificateInfo::EmailAddress)));
    subjectFormLayout->addRow(i18n("Organization(O)"), new QLabel(m_certificateInfo.subjectInfo(Okular::CertificateInfo::Organization)));

    auto validityBox = new QGroupBox(i18n("Validity"), generalPage);
    auto validityFormLayout = new QFormLayout(validityBox);
    validityFormLayout->setLabelAlignment(Qt::AlignLeft);
    validityFormLayout->addRow(i18n("Issued On"), new QLabel(m_certificateInfo.validityStart().toString(Qt::DefaultLocaleLongDate)));
    validityFormLayout->addRow(i18n("Expires On"), new QLabel(m_certificateInfo.validityEnd().toString(Qt::DefaultLocaleLongDate)));

    auto fingerprintBox = new QGroupBox(i18n("Fingerprints"), generalPage);
    auto fingerprintFormLayout = new QFormLayout(fingerprintBox);
    fingerprintFormLayout->setLabelAlignment(Qt::AlignLeft);
    QByteArray certData = m_certificateInfo.certificateData();
    auto sha1Label = new QLabel(QString(QCryptographicHash::hash(certData, QCryptographicHash::Sha1).toHex(' ')));
    sha1Label->setWordWrap(true);
    auto sha256Label = new QLabel(QString(QCryptographicHash::hash(certData, QCryptographicHash::Sha256).toHex(' ')));
    sha256Label->setWordWrap(true);
    fingerprintFormLayout->addRow(i18n("SHA-1 Fingerprint"), sha1Label);
    fingerprintFormLayout->addRow(i18n("SHA-256 Fingerprint"), sha256Label);

    auto generalPageLayout = new QVBoxLayout(generalPage);
    generalPageLayout->addWidget(issuerBox);
    generalPageLayout->addWidget(subjectBox);
    generalPageLayout->addWidget(validityBox);
    generalPageLayout->addWidget(fingerprintBox);
    generalPageLayout->addStretch();

    // force column 1 to have same width
    auto resizer = new KColumnResizer(this);
    resizer->addWidgetsFromLayout(issuerBox->layout(), 0);
    resizer->addWidgetsFromLayout(subjectBox->layout(), 0);
    resizer->addWidgetsFromLayout(validityBox->layout(), 0);
    resizer->addWidgetsFromLayout(fingerprintBox->layout(), 0);

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
        text = m_certificateInfo.publicKey().toHex(' ');
        break;
    case CertificateModel::KeyUsage:
        text = SignatureGuiUtils::getReadableKeyUsageNewLineSeparated(m_certificateInfo.keyUsageExtensions());
        break;
    }
    m_propertyText->setText(text);
}

void CertificateViewer::exportCertificate()
{
    const QString caption = i18n("Where do you want to save this certificate?");
    const QString path = QFileDialog::getSaveFileName(this, caption, QStringLiteral("Certificate.cer"), i18n("Certificate File (*.cer)"));
    if (!path.isEmpty()) {
        QFile targetFile(path);
        targetFile.open(QIODevice::WriteOnly);
        if (targetFile.write(m_certificateInfo.certificateData()) == -1) {
            KMessageBox::error(this, i18n("Could not export the certificate"));
        }
        targetFile.close();
    }
}
