/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signatureutils.h"
#include <KLocalizedString>

using namespace Okular;

static QString handleEmpty(const QString &string, CertificateInfo::EmptyString empty)
{
    if (string.isEmpty()) {
        switch (empty) {
        case CertificateInfo::EmptyString::Empty:
            return {};
        case CertificateInfo::EmptyString::TranslatedNotAvailable:
            return i18n("Not Available");
        }
        return {};
    }
    return string;
}

class EntityInfo
{
public:
    QString commonName;
    QString distinguishedName;
    QString emailAddress;
    QString organization;
};
class Okular::CertificateInfoPrivate : public QSharedData
{
public:
    bool isNull = true;
    int version = -1;
    QByteArray serialNumber;
    EntityInfo issuerInfo;
    EntityInfo subjectInfo;
    QString nickName;
    QDateTime validityStart;
    QDateTime validityEnd;
    CertificateInfo::KeyUsageExtensions keyUsageExtensions = CertificateInfo::KuNone;
    QByteArray publicKey;
    CertificateInfo::PublicKeyType publicKeyType = CertificateInfo::OtherKey;
    int publicKeyStrength = -1;
    bool isSelfSigned = false;
    QByteArray certificateData;
    CertificateInfo::Backend backend = CertificateInfo::Backend::Unknown;
    std::function<bool(QString)> checkPasswordFunction;
};

CertificateInfo::CertificateInfo()
    : d {new CertificateInfoPrivate()}
{
}

Okular::CertificateInfo::CertificateInfo(const Okular::CertificateInfo &other) = default;
Okular::CertificateInfo::CertificateInfo(Okular::CertificateInfo &&other) noexcept = default;
Okular::CertificateInfo &Okular::CertificateInfo::operator=(const Okular::CertificateInfo &other) = default;
CertificateInfo &Okular::CertificateInfo::operator=(Okular::CertificateInfo &&other) noexcept = default;

CertificateInfo::~CertificateInfo() = default;

Q_DECLARE_OPERATORS_FOR_FLAGS(CertificateInfo::KeyUsageExtensions)

bool CertificateInfo::isNull() const
{
    return d->isNull;
}

void CertificateInfo::setNull(bool isNull)
{
    d->isNull = isNull;
}

int CertificateInfo::version() const
{
    return d->version;
}
void CertificateInfo::setVersion(int version)
{
    d->version = version;
}

QByteArray CertificateInfo::serialNumber() const
{
    return d->serialNumber;
}
void CertificateInfo::setSerialNumber(const QByteArray &serialNumber)
{
    d->serialNumber = serialNumber;
}

QString CertificateInfo::issuerInfo(EntityInfoKey key, EmptyString empty) const
{
    switch (key) {
    case EntityInfoKey::CommonName:
        return handleEmpty(d->issuerInfo.commonName, empty);
    case EntityInfoKey::DistinguishedName:
        return handleEmpty(d->issuerInfo.distinguishedName, empty);
    case EntityInfoKey::EmailAddress:
        return handleEmpty(d->issuerInfo.emailAddress, empty);
    case EntityInfoKey::Organization:
        return handleEmpty(d->issuerInfo.organization, empty);
    }
    return QString();
}

void CertificateInfo::setIssuerInfo(EntityInfoKey key, const QString &value)
{
    switch (key) {
    case EntityInfoKey::CommonName:
        d->issuerInfo.commonName = value;
        return;
    case EntityInfoKey::DistinguishedName:
        d->issuerInfo.distinguishedName = value;
        return;
    case EntityInfoKey::EmailAddress:
        d->issuerInfo.emailAddress = value;
        return;
    case EntityInfoKey::Organization:
        d->issuerInfo.organization = value;
        return;
    }
}

QString CertificateInfo::subjectInfo(EntityInfoKey key, EmptyString empty) const
{
    switch (key) {
    case EntityInfoKey::CommonName:
        return handleEmpty(d->subjectInfo.commonName, empty);
    case EntityInfoKey::DistinguishedName:
        return handleEmpty(d->subjectInfo.distinguishedName, empty);
    case EntityInfoKey::EmailAddress:
        return handleEmpty(d->subjectInfo.emailAddress, empty);
    case EntityInfoKey::Organization:
        return handleEmpty(d->subjectInfo.organization, empty);
    }
    return QString();
}

void CertificateInfo::setSubjectInfo(EntityInfoKey key, const QString &value)
{
    switch (key) {
    case EntityInfoKey::CommonName:
        d->subjectInfo.commonName = value;
        return;
    case EntityInfoKey::DistinguishedName:
        d->subjectInfo.distinguishedName = value;
        return;
    case EntityInfoKey::EmailAddress:
        d->subjectInfo.emailAddress = value;
        return;
    case EntityInfoKey::Organization:
        d->subjectInfo.organization = value;
        return;
    }
}

QString CertificateInfo::nickName() const
{
    return d->nickName;
}

void CertificateInfo::setNickName(const QString &nickName)
{
    d->nickName = nickName;
}

QDateTime CertificateInfo::validityStart() const
{
    return d->validityStart;
}

void CertificateInfo::setValidityStart(const QDateTime &start)
{
    d->validityStart = start;
}

QDateTime CertificateInfo::validityEnd() const
{
    return d->validityEnd;
}

void Okular::CertificateInfo::setValidityEnd(const QDateTime &validityEnd)
{
    d->validityEnd = validityEnd;
}

CertificateInfo::KeyUsageExtensions CertificateInfo::keyUsageExtensions() const
{
    return d->keyUsageExtensions;
}

void Okular::CertificateInfo::setKeyUsageExtensions(Okular::CertificateInfo::KeyUsageExtensions ext)
{
    d->keyUsageExtensions = ext;
}

QByteArray CertificateInfo::publicKey() const
{
    return d->publicKey;
}

void Okular::CertificateInfo::setPublicKey(const QByteArray &publicKey)
{
    d->publicKey = publicKey;
}

CertificateInfo::PublicKeyType CertificateInfo::publicKeyType() const
{
    return d->publicKeyType;
}

void CertificateInfo::setPublicKeyType(PublicKeyType type)
{
    d->publicKeyType = type;
}

int CertificateInfo::publicKeyStrength() const
{
    return d->publicKeyStrength;
}

void CertificateInfo::setPublicKeyStrength(int strength)
{
    d->publicKeyStrength = strength;
}

bool CertificateInfo::isSelfSigned() const
{
    return d->isSelfSigned;
}

void CertificateInfo::setSelfSigned(bool selfSigned)
{
    d->isSelfSigned = selfSigned;
}

QByteArray CertificateInfo::certificateData() const
{
    return d->certificateData;
}

void CertificateInfo::setCertificateData(const QByteArray &certificateData)
{
    d->certificateData = certificateData;
}

CertificateInfo::Backend CertificateInfo::backend() const
{
    return d->backend;
}

void CertificateInfo::setBackend(Backend backend)
{
    d->backend = backend;
}

bool CertificateInfo::checkPassword(const QString &password) const
{
    if (d->checkPasswordFunction) {
        return d->checkPasswordFunction(password);
    }
    return false;
}

void CertificateInfo::setCheckPasswordFunction(const std::function<bool(const QString &)> &passwordFunction)
{
    d->checkPasswordFunction = passwordFunction;
}

class Okular::SignatureInfoPrivate : public QSharedData
{
public:
    SignatureInfo::SignatureStatus signatureStatus = SignatureInfo::SignatureStatusUnknown;
    SignatureInfo::CertificateStatus certificateStatus = SignatureInfo::CertificateStatusUnknown;
    SignatureInfo::HashAlgorithm hashAlgorithm = SignatureInfo::HashAlgorithmUnknown;
    QString signerName;
    QString signerSubjectDN;
    QString location;
    QString reason;
    QDateTime signingTime;
    QByteArray signature;
    QList<qint64> signedRangeBounds;
    bool signsTotalDocument = false;
    CertificateInfo certificateInfo;
};

SignatureInfo::SignatureInfo()
    : d {new SignatureInfoPrivate()}
{
}

SignatureInfo::SignatureInfo(SignatureInfo &&other) noexcept = default;
SignatureInfo::SignatureInfo(const SignatureInfo &other) = default;
SignatureInfo &SignatureInfo::operator=(SignatureInfo &&other) noexcept = default;
Okular::SignatureInfo &Okular::SignatureInfo::operator=(const Okular::SignatureInfo &other) = default;
SignatureInfo::~SignatureInfo() = default;

SignatureInfo::SignatureStatus SignatureInfo::signatureStatus() const
{
    return d->signatureStatus;
}

void SignatureInfo::setSignatureStatus(SignatureInfo::SignatureStatus status)
{
    d->signatureStatus = status;
}

SignatureInfo::CertificateStatus SignatureInfo::certificateStatus() const
{
    return d->certificateStatus;
}

void SignatureInfo::setCertificateStatus(SignatureInfo::CertificateStatus status)
{
    d->certificateStatus = status;
}

SignatureInfo::HashAlgorithm SignatureInfo::hashAlgorithm() const
{
    return d->hashAlgorithm;
}

void Okular::SignatureInfo::setHashAlgorithm(Okular::SignatureInfo::HashAlgorithm algorithm)
{
    d->hashAlgorithm = algorithm;
}

QString SignatureInfo::signerName() const
{
    return d->signerName;
}

void SignatureInfo::setSignerName(const QString &signerName)
{
    d->signerName = signerName;
}

QString SignatureInfo::signerSubjectDN() const
{
    return d->signerSubjectDN;
}

void Okular::SignatureInfo::setSignerSubjectDN(const QString &signerSubjectDN)
{
    d->signerSubjectDN = signerSubjectDN;
}

QString SignatureInfo::location() const
{
    return d->location;
}

void SignatureInfo::setLocation(const QString &location)
{
    d->location = location;
}

QString SignatureInfo::reason() const
{
    return d->reason;
}

void Okular::SignatureInfo::setReason(const QString &reason)
{
    d->reason = reason;
}

QDateTime SignatureInfo::signingTime() const
{
    return d->signingTime;
}

void Okular::SignatureInfo::setSigningTime(const QDateTime &time)
{
    d->signingTime = time;
}

QByteArray SignatureInfo::signature() const
{
    return d->signature;
}

void SignatureInfo::setSignature(const QByteArray &signature)
{
    d->signature = signature;
}

QList<qint64> SignatureInfo::signedRangeBounds() const
{
    return d->signedRangeBounds;
}

void SignatureInfo::setSignedRangeBounds(const QList<qint64> &range)
{
    d->signedRangeBounds = range;
}

bool SignatureInfo::signsTotalDocument() const
{
    return d->signsTotalDocument;
}

void SignatureInfo::setSignsTotalDocument(bool total)
{
    d->signsTotalDocument = total;
}

CertificateInfo SignatureInfo::certificateInfo() const
{
    return d->certificateInfo;
}

void SignatureInfo::setCertificateInfo(const Okular::CertificateInfo &info)
{
    d->certificateInfo = info;
}

CertificateStore::CertificateStore()
{
}

CertificateStore::~CertificateStore()
{
}

QList<CertificateInfo> CertificateStore::signingCertificates(bool *userCancelled) const
{
    *userCancelled = false;
    return QList<CertificateInfo>();
}

QList<CertificateInfo> CertificateStore::signingCertificatesForNow(bool *userCancelled, bool *nonDateValidCerts) const
{
    const QDateTime now = QDateTime::currentDateTime();
    QList<Okular::CertificateInfo> certs = signingCertificates(userCancelled);
    auto it = certs.begin();
    *nonDateValidCerts = false;
    while (it != certs.end()) {
        if (it->validityStart() > now || now > it->validityEnd()) {
            it = certs.erase(it);
            *nonDateValidCerts = true;
        } else {
            ++it;
        }
    }
    return certs;
}
