/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pdfsignatureutils.h"

#include <KLocalizedString>
#include <QDebug>
#include <QInputDialog>

static Okular::CertificateInfo::KeyUsageExtensions fromPoppler(Poppler::CertificateInfo::KeyUsageExtensions popplerKu)
{
    using namespace Okular;
    CertificateInfo::KeyUsageExtensions ku = CertificateInfo::KuNone;
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuDigitalSignature)) {
        ku |= CertificateInfo::KuDigitalSignature;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuNonRepudiation)) {
        ku |= CertificateInfo::KuNonRepudiation;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuKeyEncipherment)) {
        ku |= CertificateInfo::KuKeyEncipherment;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuDataEncipherment)) {
        ku |= CertificateInfo::KuDataEncipherment;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuKeyAgreement)) {
        ku |= CertificateInfo::KuKeyAgreement;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuKeyCertSign)) {
        ku |= CertificateInfo::KuKeyCertSign;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuClrSign)) {
        ku |= CertificateInfo::KuClrSign;
    }
    if (popplerKu.testFlag(Poppler::CertificateInfo::KuEncipherOnly)) {
        ku |= CertificateInfo::KuEncipherOnly;
    }
    return ku;
}

static Okular::CertificateInfo::PublicKeyType fromPoppler(Poppler::CertificateInfo::PublicKeyType type)
{
    switch (type) {
    case Poppler::CertificateInfo::RsaKey:
        return Okular::CertificateInfo::RsaKey;
    case Poppler::CertificateInfo::DsaKey:
        return Okular::CertificateInfo::DsaKey;
    case Poppler::CertificateInfo::EcKey:
        return Okular::CertificateInfo::EcKey;
    case Poppler::CertificateInfo::OtherKey:
        return Okular::CertificateInfo::OtherKey;
    }
    return Okular::CertificateInfo::OtherKey;
}

#if POPPLER_VERSION_MACRO > QT_VERSION_CHECK(23, 8, 0)
static Okular::CertificateInfo::KeyLocation fromPoppler(Poppler::CertificateInfo::KeyLocation location)
{
    switch (location) {
    case Poppler::CertificateInfo::KeyLocation::Computer:
        return Okular::CertificateInfo::KeyLocation::Computer;
    case Poppler::CertificateInfo::KeyLocation::Unknown:
        return Okular::CertificateInfo::KeyLocation::Unknown;
    case Poppler::CertificateInfo::KeyLocation::HardwareToken:
        return Okular::CertificateInfo::KeyLocation::HardwareToken;
    case Poppler::CertificateInfo::KeyLocation::Other:
        return Okular::CertificateInfo::KeyLocation::Other;
    }
    return Okular::CertificateInfo::KeyLocation::Unknown;
}
#endif

Okular::CertificateInfo fromPoppler(const Poppler::CertificateInfo &pInfo)
{
    Okular::CertificateInfo oInfo;
    if (pInfo.isNull()) {
        return oInfo;
    }
    oInfo.setNull(false);
    oInfo.setVersion(pInfo.version());
    oInfo.setSerialNumber(pInfo.serialNumber());
    for (auto key :
         {Poppler::CertificateInfo::EntityInfoKey::CommonName, Poppler::CertificateInfo::EntityInfoKey::DistinguishedName, Poppler::CertificateInfo::EntityInfoKey::EmailAddress, Poppler::CertificateInfo::EntityInfoKey::Organization}) {
        oInfo.setIssuerInfo(static_cast<Okular::CertificateInfo::EntityInfoKey>(key), pInfo.issuerInfo(key));
        oInfo.setSubjectInfo(static_cast<Okular::CertificateInfo::EntityInfoKey>(key), pInfo.subjectInfo(key));
    }
    oInfo.setNickName(pInfo.nickName());
    oInfo.setValidityStart(pInfo.validityStart());
    oInfo.setValidityEnd(pInfo.validityEnd());
    oInfo.setKeyUsageExtensions(fromPoppler(pInfo.keyUsageExtensions()));
    oInfo.setPublicKey(pInfo.publicKey());
    oInfo.setPublicKeyType(fromPoppler(pInfo.publicKeyType()));
    oInfo.setPublicKeyStrength(pInfo.publicKeyStrength());
    oInfo.setSelfSigned(pInfo.isSelfSigned());
    oInfo.setCertificateData(pInfo.certificateData());
#if POPPLER_VERSION_MACRO > QT_VERSION_CHECK(23, 8, 0)
    oInfo.setKeyLocation(fromPoppler(pInfo.keyLocation()));
#endif
    oInfo.setCheckPasswordFunction([pInfo](const QString &password) {
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 06, 0)
        auto backend = Poppler::activeCryptoSignBackend();
        if (!backend) {
            return false;
        }
        if (Poppler::hasCryptoSignBackendFeature(backend.value(), Poppler::CryptoSignBackendFeature::BackendAsksPassphrase)) {
            // we shouldn't ask anyone about passwords. The backend will do that themselves, so just assume everything is okay.
            return true;
        }
#endif
        return pInfo.checkPassword(password);
    });
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(23, 06, 0)
    if (Poppler::activeCryptoSignBackend() == Poppler::CryptoSignBackend::GPG) {
        oInfo.setBackend(Okular::CertificateInfo::Backend::Gpg);
    }
#endif
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(24, 12, 0)
    oInfo.setQualified(pInfo.isQualified());
#endif
    return oInfo;
}

Okular::SignatureInfo::CertificateStatus fromPoppler(Poppler::SignatureValidationInfo::CertificateStatus status)
{
    switch (status) {
    case Poppler::SignatureValidationInfo::CertificateTrusted:
        return Okular::SignatureInfo::CertificateTrusted;
    case Poppler::SignatureValidationInfo::CertificateUntrustedIssuer:
        return Okular::SignatureInfo::CertificateUntrustedIssuer;
    case Poppler::SignatureValidationInfo::CertificateUnknownIssuer:
        return Okular::SignatureInfo::CertificateUnknownIssuer;
    case Poppler::SignatureValidationInfo::CertificateRevoked:
        return Okular::SignatureInfo::CertificateRevoked;
    case Poppler::SignatureValidationInfo::CertificateExpired:
        return Okular::SignatureInfo::CertificateExpired;
    case Poppler::SignatureValidationInfo::CertificateGenericError:
        return Okular::SignatureInfo::CertificateGenericError;
    case Poppler::SignatureValidationInfo::CertificateNotVerified:
        return Okular::SignatureInfo::CertificateNotVerified;
#if POPPLER_VERSION_MACRO > QT_VERSION_CHECK(24, 04, 0)
    case Poppler::SignatureValidationInfo::CertificateVerificationInProgress:
        return Okular::SignatureInfo::CertificateVerificationInProgress;
#endif
    default:
        return Okular::SignatureInfo::CertificateStatusUnknown;
    }
}

Okular::SignatureInfo::SignatureStatus fromPoppler(Poppler::SignatureValidationInfo::SignatureStatus status)
{
    switch (status) {
    case Poppler::SignatureValidationInfo::SignatureValid:
        return Okular::SignatureInfo::SignatureValid;
    case Poppler::SignatureValidationInfo::SignatureInvalid:
        return Okular::SignatureInfo::SignatureInvalid;
    case Poppler::SignatureValidationInfo::SignatureDigestMismatch:
        return Okular::SignatureInfo::SignatureDigestMismatch;
    case Poppler::SignatureValidationInfo::SignatureDecodingError:
        return Okular::SignatureInfo::SignatureDecodingError;
    case Poppler::SignatureValidationInfo::SignatureGenericError:
        return Okular::SignatureInfo::SignatureGenericError;
    case Poppler::SignatureValidationInfo::SignatureNotFound:
        return Okular::SignatureInfo::SignatureNotFound;
    case Poppler::SignatureValidationInfo::SignatureNotVerified:
        return Okular::SignatureInfo::SignatureNotVerified;
    default:
        return Okular::SignatureInfo::SignatureStatusUnknown;
    }
}

Okular::SignatureInfo::HashAlgorithm fromPoppler(Poppler::SignatureValidationInfo::HashAlgorithm hash)
{
    switch (hash) {
    case Poppler::SignatureValidationInfo::HashAlgorithmMd2:
        return Okular::SignatureInfo::HashAlgorithmMd2;
    case Poppler::SignatureValidationInfo::HashAlgorithmMd5:
        return Okular::SignatureInfo::HashAlgorithmMd5;
    case Poppler::SignatureValidationInfo::HashAlgorithmSha1:
        return Okular::SignatureInfo::HashAlgorithmSha1;
    case Poppler::SignatureValidationInfo::HashAlgorithmSha256:
        return Okular::SignatureInfo::HashAlgorithmSha256;
    case Poppler::SignatureValidationInfo::HashAlgorithmSha384:
        return Okular::SignatureInfo::HashAlgorithmSha384;
    case Poppler::SignatureValidationInfo::HashAlgorithmSha512:
        return Okular::SignatureInfo::HashAlgorithmSha512;
    case Poppler::SignatureValidationInfo::HashAlgorithmSha224:
        return Okular::SignatureInfo::HashAlgorithmSha224;
    default:
        return Okular::SignatureInfo::HashAlgorithmUnknown;
    }
}

Okular::SignatureInfo fromPoppler(const Poppler::SignatureValidationInfo &pInfo)
{
    Okular::SignatureInfo oInfo;
    oInfo.setCertificateInfo(fromPoppler(pInfo.certificateInfo()));
    oInfo.setSignatureStatus(fromPoppler(pInfo.signatureStatus()));
    oInfo.setCertificateStatus(fromPoppler(pInfo.certificateStatus()));
    oInfo.setHashAlgorithm(fromPoppler(pInfo.hashAlgorithm()));
    oInfo.setSignerName(pInfo.signerName());
    oInfo.setSignerSubjectDN(pInfo.signerSubjectDN());
    oInfo.setLocation(pInfo.location());
    oInfo.setReason(pInfo.reason());
    oInfo.setSigningTime(QDateTime::fromSecsSinceEpoch(pInfo.signingTime()));
    oInfo.setSignature(pInfo.signature());
    oInfo.setSignedRangeBounds(pInfo.signedRangeBounds());
    oInfo.setSignsTotalDocument(pInfo.signsTotalDocument());
    return oInfo;
}

PopplerCertificateStore::~PopplerCertificateStore() = default;

QList<Okular::CertificateInfo> PopplerCertificateStore::signingCertificates(bool *userCancelled) const
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
    QList<Okular::CertificateInfo> vReturnCerts;
    for (const auto &cert : certs) {
        vReturnCerts.append(fromPoppler(cert));
    }

    Poppler::setNSSPasswordCallback(nullptr);

    return vReturnCerts;
}
