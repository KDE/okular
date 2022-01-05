/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pdfsignatureutils.h"

#include <KLocalizedString>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>

PopplerCertificateInfo::PopplerCertificateInfo(const Poppler::CertificateInfo &info)
    : m_info(info)
{
}

PopplerCertificateInfo::~PopplerCertificateInfo()
{
}

bool PopplerCertificateInfo::isNull() const
{
    return m_info.isNull();
}

int PopplerCertificateInfo::version() const
{
    return m_info.version();
}

QByteArray PopplerCertificateInfo::serialNumber() const
{
    return m_info.serialNumber();
}

QString PopplerCertificateInfo::issuerInfo(PopplerCertificateInfo::EntityInfoKey key) const
{
    QString str = m_info.issuerInfo(static_cast<Poppler::CertificateInfo::EntityInfoKey>(key));
    return !str.isEmpty() ? str : i18n("Not Available");
}

QString PopplerCertificateInfo::subjectInfo(PopplerCertificateInfo::EntityInfoKey key) const
{
    QString str = m_info.subjectInfo(static_cast<Poppler::CertificateInfo::EntityInfoKey>(key));
    return !str.isEmpty() ? str : i18n("Not Available");
}

QString PopplerCertificateInfo::nickName() const
{
    return m_info.nickName();
}

QDateTime PopplerCertificateInfo::validityStart() const
{
    return m_info.validityStart();
}

QDateTime PopplerCertificateInfo::validityEnd() const
{
    return m_info.validityEnd();
}

PopplerCertificateInfo::KeyUsageExtensions PopplerCertificateInfo::keyUsageExtensions() const
{
    Poppler::CertificateInfo::KeyUsageExtensions popplerKu = m_info.keyUsageExtensions();
    KeyUsageExtensions ku = KuNone;
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuDigitalSignature)) {
        ku |= KuDigitalSignature;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuNonRepudiation)) {
        ku |= KuNonRepudiation;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuKeyEncipherment)) {
        ku |= KuKeyEncipherment;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuDataEncipherment)) {
        ku |= KuDataEncipherment;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuKeyAgreement)) {
        ku |= KuKeyAgreement;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuKeyCertSign)) {
        ku |= KuKeyCertSign;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuClrSign)) {
        ku |= KuClrSign;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuEncipherOnly)) {
        ku |= KuEncipherOnly;
    }
    return ku;
}

QByteArray PopplerCertificateInfo::publicKey() const
{
    return m_info.publicKey();
}

PopplerCertificateInfo::PublicKeyType PopplerCertificateInfo::publicKeyType() const
{
    switch (m_info.publicKeyType()) {
    case Poppler::CertificateInfo::RsaKey:
        return RsaKey;
    case Poppler::CertificateInfo::DsaKey:
        return DsaKey;
    case Poppler::CertificateInfo::EcKey:
        return EcKey;
    case Poppler::CertificateInfo::OtherKey:
        return OtherKey;
    }

    return OtherKey;
}

int PopplerCertificateInfo::publicKeyStrength() const
{
    return m_info.publicKeyStrength();
}

bool PopplerCertificateInfo::isSelfSigned() const
{
    return m_info.isSelfSigned();
}

QByteArray PopplerCertificateInfo::certificateData() const
{
    return m_info.certificateData();
}

bool PopplerCertificateInfo::checkPassword(const QString &password) const
{
    return m_info.checkPassword(password);
}

PopplerSignatureInfo::PopplerSignatureInfo(const Poppler::SignatureValidationInfo &info)
    : m_info(info)
{
    m_certfiticateInfo = new PopplerCertificateInfo(m_info.certificateInfo());
}

PopplerSignatureInfo::~PopplerSignatureInfo()
{
    delete m_certfiticateInfo;
}

PopplerSignatureInfo::SignatureStatus PopplerSignatureInfo::signatureStatus() const
{
    switch (m_info.signatureStatus()) {
    case Poppler::SignatureValidationInfo::SignatureValid:
        return SignatureValid;
    case Poppler::SignatureValidationInfo::SignatureInvalid:
        return SignatureInvalid;
    case Poppler::SignatureValidationInfo::SignatureDigestMismatch:
        return SignatureDigestMismatch;
    case Poppler::SignatureValidationInfo::SignatureDecodingError:
        return SignatureDecodingError;
    case Poppler::SignatureValidationInfo::SignatureGenericError:
        return SignatureGenericError;
    case Poppler::SignatureValidationInfo::SignatureNotFound:
        return SignatureNotFound;
    case Poppler::SignatureValidationInfo::SignatureNotVerified:
        return SignatureNotVerified;
    default:
        return SignatureStatusUnknown;
    }
}

PopplerSignatureInfo::CertificateStatus PopplerSignatureInfo::certificateStatus() const
{
    switch (m_info.certificateStatus()) {
    case Poppler::SignatureValidationInfo::CertificateTrusted:
        return CertificateTrusted;
    case Poppler::SignatureValidationInfo::CertificateUntrustedIssuer:
        return CertificateUntrustedIssuer;
    case Poppler::SignatureValidationInfo::CertificateUnknownIssuer:
        return CertificateUnknownIssuer;
    case Poppler::SignatureValidationInfo::CertificateRevoked:
        return CertificateRevoked;
    case Poppler::SignatureValidationInfo::CertificateExpired:
        return CertificateExpired;
    case Poppler::SignatureValidationInfo::CertificateGenericError:
        return CertificateGenericError;
    case Poppler::SignatureValidationInfo::CertificateNotVerified:
        return CertificateNotVerified;
    default:
        return CertificateStatusUnknown;
    }
}

PopplerSignatureInfo::HashAlgorithm PopplerSignatureInfo::hashAlgorithm() const
{
    switch (m_info.hashAlgorithm()) {
    case Poppler::SignatureValidationInfo::HashAlgorithmMd2:
        return HashAlgorithmMd2;
    case Poppler::SignatureValidationInfo::HashAlgorithmMd5:
        return HashAlgorithmMd5;
    case Poppler::SignatureValidationInfo::HashAlgorithmSha1:
        return HashAlgorithmSha1;
    case Poppler::SignatureValidationInfo::HashAlgorithmSha256:
        return HashAlgorithmSha256;
    case Poppler::SignatureValidationInfo::HashAlgorithmSha384:
        return HashAlgorithmSha384;
    case Poppler::SignatureValidationInfo::HashAlgorithmSha512:
        return HashAlgorithmSha512;
    case Poppler::SignatureValidationInfo::HashAlgorithmSha224:
        return HashAlgorithmSha224;
    default:
        return HashAlgorithmUnknown;
    }
}

QString PopplerSignatureInfo::signerName() const
{
    return m_info.signerName();
}

QString PopplerSignatureInfo::signerSubjectDN() const
{
    return m_info.signerSubjectDN();
}

QString PopplerSignatureInfo::location() const
{
    return m_info.location();
}

QString PopplerSignatureInfo::reason() const
{
    return m_info.reason();
}

QDateTime PopplerSignatureInfo::signingTime() const
{
    return QDateTime::fromSecsSinceEpoch(m_info.signingTime());
}

QByteArray PopplerSignatureInfo::signature() const
{
    return m_info.signature();
}

QList<qint64> PopplerSignatureInfo::signedRangeBounds() const
{
    return m_info.signedRangeBounds();
}

bool PopplerSignatureInfo::signsTotalDocument() const
{
    return m_info.signsTotalDocument();
}

const Okular::CertificateInfo &PopplerSignatureInfo::certificateInfo() const
{
    return *m_certfiticateInfo;
}

PopplerCertificateStore::~PopplerCertificateStore() = default;

QList<Okular::CertificateInfo *> PopplerCertificateStore::signingCertificates(bool *userCancelled) const
{
    *userCancelled = false;
    auto PDFGeneratorNSSPasswordCallback = [&userCancelled](const char *element) -> char * {
        bool ok;
        const QString pwd = QInputDialog::getText(nullptr, i18n("Enter Password"), i18n("Enter password to open %1:", QString::fromUtf8(element)), QLineEdit::Password, QString(), &ok);
        *userCancelled = !ok;
        return ok ? strdup(pwd.toUtf8().constData()) : nullptr;
    };
    Poppler::setNSSPasswordCallback(PDFGeneratorNSSPasswordCallback);

    const QVector<Poppler::CertificateInfo> certs = Poppler::getAvailableSigningCertificates();
    QList<Okular::CertificateInfo *> vReturnCerts;
    for (const auto &cert : certs) {
        vReturnCerts.append(new PopplerCertificateInfo(cert));
    }

    Poppler::setNSSPasswordCallback(nullptr);

    return vReturnCerts;
}

void SignatureDialog::accept()
{
    m_reason = m_reason_field->text();
    m_location = m_location_field->text();
    m_someText = m_someText_field->text();
    QDialog::accept();
}

void SignatureDialog::pickImage()
{
    QFileInfo oldInfo(m_imagePath);
    QString filePath = QFileDialog::getOpenFileName(this, i18n("Pick SVG ..."), oldInfo.absolutePath(), QStringLiteral("Scalable Vector Graphics (*.svg)"));
    QFileInfo newInfo(filePath);
    if (newInfo.exists()) {
        m_imagePath = newInfo.absoluteFilePath();
    }
    m_backgroundPreview->setPixmap(QIcon(m_imagePath).pixmap(QSize(72, 72)));
}

SignatureDialog::SignatureDialog(QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *stack = new QVBoxLayout();

    QHBoxLayout *line;
    QLabel *label;

    label = new QLabel(i18n("You may provide the following optional information for your digital signature:"));
    stack->addWidget(label);

    QGridLayout *dataLayout = new QGridLayout();

    label = new QLabel(i18n("Reason:"));
    label->setAlignment(Qt::AlignRight);
    m_reason_field = new QLineEdit(m_reason);
    dataLayout->addWidget(label, 0, 0);
    dataLayout->addWidget(m_reason_field, 0, 1);

    label = new QLabel(i18n("Location:"));
    label->setAlignment(Qt::AlignRight);
    m_location_field = new QLineEdit(m_location);
    dataLayout->addWidget(label, 1, 0);
    dataLayout->addWidget(m_location_field, 1, 1);

    label = new QLabel(i18n("Other information:"));
    label->setAlignment(Qt::AlignRight);
    m_someText_field = new QLineEdit(m_someText);
    dataLayout->addWidget(label, 2, 0);
    dataLayout->addWidget(m_someText_field, 2, 1);

    QString defaultImgFilename = QStringLiteral("howtoholdapen1.svg");
    QFile *defaultImg = new QFile(defaultImgFilename); // look in curdir first
    if (!defaultImg->exists()) {
        QString foundPath = QStandardPaths::locate(QStandardPaths::DataLocation, defaultImgFilename);
        if (!foundPath.isEmpty())
            defaultImg = new QFile(foundPath);
    }

    if (defaultImg->exists()) {
        QFileInfo defaultImgInfo(defaultImg->fileName());
        m_imagePath = defaultImgInfo.absoluteFilePath();
    }

    label = new QLabel();
    label->setFrameShape(QFrame::Box);
    label->setFrameShadow(QFrame::Raised);
    label->setLineWidth(2);
    label->setAlignment(Qt::AlignCenter);
    if (!m_imagePath.isEmpty())
        label->setPixmap(QIcon(m_imagePath).pixmap(QSize(72, 72)));
    else
        label->setText(i18n("(none)"));
    QPushButton *imgDlg = new QPushButton(i18n("Pick background SVG"));
    dataLayout->addWidget(label, 3, 0);
    dataLayout->addWidget(imgDlg, 3, 1);
    m_backgroundPreview = label;

    stack->addLayout(dataLayout);

    line = new QHBoxLayout();
    QPushButton *sign = new QPushButton(i18n("Sign"));
    QPushButton *cancel = new QPushButton(i18n("Cancel"));
    QIcon cancel_ico = QIcon::fromTheme(QStringLiteral("dialog-cancel"));
    QIcon sign_ico = QIcon::fromTheme(QStringLiteral("document-edit-sign"));
    sign->setIcon(sign_ico);
    cancel->setIcon(cancel_ico);
    line->addWidget(sign);
    line->addWidget(cancel);
    stack->addLayout(line);

    setLayout(stack);
    setModal(true);
    connect(sign, &QPushButton::clicked, this, &SignatureDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(imgDlg, &QPushButton::clicked, this, &SignatureDialog::pickImage);
}

QString SignatureDialog::reason()
{
    return m_reason;
}

QString SignatureDialog::location()
{
    return m_location;
}

QString SignatureDialog::someText()
{
    return m_someText;
}

QString SignatureDialog::imagePath()
{
    return m_imagePath;
}
