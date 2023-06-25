/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SIGNATUREUTILS_H
#define OKULAR_SIGNATUREUTILS_H

#include "okularcore_export.h"

#include <QDateTime>
#include <QFlag>
#include <QList>
#include <QSharedPointer>
#include <QString>

namespace Okular
{

/**
 * @short A helper class to store information about x509 certificate
 */
class CertificateInfoPrivate;
class OKULARCORE_EXPORT CertificateInfo
{
public:
    /** The certificate backend is mostly
     important if there is a wish to integrate
     third party viewers, where some third party
     viewers only interacts with some and not other
     backend */
    enum class Backend {
        /** The backend is either unknown
         or known, but not something there is
         currently supported need for*/
        Unknown,
        /** The certificates in question originates
         in gpg and thus can be queried using e.g.
         KDE's certificate manager Kleopatra */
        Gpg
    };
    /**
     * The algorithm of public key.
     */
    enum PublicKeyType { RsaKey, DsaKey, EcKey, OtherKey };

    /**
     * Certificate key usage extensions.
     */
    enum KeyUsageExtension { KuDigitalSignature = 0x80, KuNonRepudiation = 0x40, KuKeyEncipherment = 0x20, KuDataEncipherment = 0x10, KuKeyAgreement = 0x08, KuKeyCertSign = 0x04, KuClrSign = 0x02, KuEncipherOnly = 0x01, KuNone = 0x00 };
    Q_DECLARE_FLAGS(KeyUsageExtensions, KeyUsageExtension)

    /**
     * Predefined keys for elements in an entity's distinguished name.
     */
    enum EntityInfoKey {
        CommonName,
        DistinguishedName,
        EmailAddress,
        Organization,
    };
    /**
     * How should certain empty strings be treated
     * @since 23.08
     */
    enum class EmptyString { /** Empty strings should just be empty*/ Empty, TranslatedNotAvailable /** Empty strings should be a localized version of "Not available" */ };

    /**
     * Destructor
     */
    ~CertificateInfo();

    /**
     * Returns true if the certificate has no contents; otherwise returns false
     * @since 23.08
     */
    bool isNull() const;

    /**
     * Sets the null value of the certificate.
     * @since 23.08
     */
    void setNull(bool null);

    /**
     * The certificate version string.
     * @since 23.08
     */
    int version() const;

    /**
     * Sets the certificate version string.
     * @since 23.08
     */
    void setVersion(int version);

    /**
     * The certificate serial number.
     * @since 23.08
     */
    QByteArray serialNumber() const;

    /**
     * Sets the certificate serial number.
     * @since 23.08
     */
    void setSerialNumber(const QByteArray &serial);

    /**
     * Information about the issuer.
     * @since 23.08
     */
    QString issuerInfo(EntityInfoKey key, EmptyString empty) const;

    /**
     * Sets information about the issuer.
     * @since 23.08
     */
    void setIssuerInfo(EntityInfoKey key, const QString &value);

    /**
     * Information about the subject
     * @since 23.08
     */
    QString subjectInfo(EntityInfoKey key, EmptyString empty) const;

    /**
     * Sets information about the subject
     * @since 23.08
     */
    void setSubjectInfo(EntityInfoKey key, const QString &value);

    /**
     * The certificate internal database nickname
     * @since 23.08
     */
    QString nickName() const;

    /**
     * Sets the certificate internal database nickname
     * @since 23.08
     */
    void setNickName(const QString &nickName);

    /**
     * The date-time when certificate becomes valid.
     * @since 23.08
     */
    QDateTime validityStart() const;

    /**
     * Sets the date-time when certificate becomes valid.
     * @since 23.08
     */
    void setValidityStart(const QDateTime &start);

    /**
     * The date-time when certificate expires.
     * @since 23.08
     */
    QDateTime validityEnd() const;

    /**
     * Sets the date-time when certificate expires.
     * @since 23.08
     */
    void setValidityEnd(const QDateTime &validityEnd);

    /**
     * The uses allowed for the certificate.
     * @since 23.08
     */
    KeyUsageExtensions keyUsageExtensions() const;

    /**
     * Sets the uses allowed for the certificate.
     * @since 23.08
     */
    void setKeyUsageExtensions(KeyUsageExtensions ext);

    /**
     * The public key value.
     * @since 23.08
     */
    QByteArray publicKey() const;
    /**
     * Sets the public key value.
     * @since 23.08
     */
    void setPublicKey(const QByteArray &publicKey);

    /**
     * The public key type.
     * @since 23.08
     */
    PublicKeyType publicKeyType() const;

    /**
     * Sets the public key type.
     * @since 23.08
     */
    void setPublicKeyType(PublicKeyType type);

    /**
     * The strength of public key in bits.
     * @since 23.08
     */
    int publicKeyStrength() const;

    /**
     * Sets the strength of strength key in bits.
     * @since 23.08
     */
    void setPublicKeyStrength(int strength);

    /**
     * Returns true if certificate is self-signed otherwise returns false.
     * @since 23.08
     */
    bool isSelfSigned() const;

    /**
     * Sets if certificate is self-signed
     * @since 23.08
     */
    void setSelfSigned(bool selfSigned);

    /**
     * The DER encoded certificate.
     * @since 23.08
     */
    QByteArray certificateData() const;

    /**
     * Sets the DER encoded certificate.
     * @since 23.08
     */
    void setCertificateData(const QByteArray &certificateData);

    /**
     * The backend where the certificate originates.
     * see @ref Backend for details
     * @since 23.08
     */
    Backend backend() const;

    /**
     * Sets the backend for this certificate.
     * see @ref Backend for details
     * @since 23.08
     */
    void setBackend(Backend backend);

    /**
     * Checks if the given password is the correct one for this certificate
     *
     * @since 23.08
     */
    bool checkPassword(const QString &password) const;

    /**
     * Sets a function to check if the current password is correct.
     *
     * The default reject all passwords
     *
     * @since 23.08
     */
    void setCheckPasswordFunction(const std::function<bool(const QString &)> &passwordFunction);

    CertificateInfo();
    CertificateInfo(const CertificateInfo &other);
    CertificateInfo(CertificateInfo &&other) noexcept;
    CertificateInfo &operator=(const CertificateInfo &other);
    CertificateInfo &operator=(CertificateInfo &&other) noexcept;

private:
    QSharedDataPointer<CertificateInfoPrivate> d;
};

/**
 * @short A helper class to store information about digital signature
 */
class SignatureInfoPrivate;
class OKULARCORE_EXPORT SignatureInfo
{
public:
    /**
     * The verification result of the signature.
     */
    enum SignatureStatus {
        SignatureStatusUnknown,  ///< The signature status is unknown for some reason.
        SignatureValid,          ///< The signature is cryptographically valid.
        SignatureInvalid,        ///< The signature is cryptographically invalid.
        SignatureDigestMismatch, ///< The document content was changed after the signature was applied.
        SignatureDecodingError,  ///< The signature CMS/PKCS7 structure is malformed.
        SignatureGenericError,   ///< The signature could not be verified.
        SignatureNotFound,       ///< The requested signature is not present in the document.
        SignatureNotVerified     ///< The signature is not yet verified.
    };

    /**
     * The verification result of the certificate.
     */
    enum CertificateStatus {
        CertificateStatusUnknown,   ///< The certificate status is unknown for some reason.
        CertificateTrusted,         ///< The certificate is considered trusted.
        CertificateUntrustedIssuer, ///< The issuer of this certificate has been marked as untrusted by the user.
        CertificateUnknownIssuer,   ///< The certificate trust chain has not finished in a trusted root certificate.
        CertificateRevoked,         ///< The certificate was revoked by the issuing certificate authority.
        CertificateExpired,         ///< The signing time is outside the validity bounds of this certificate.
        CertificateGenericError,    ///< The certificate could not be verified.
        CertificateNotVerified      ///< The certificate is not yet verified.
    };

    /**
     * The hash algorithm of the signature
     */
    enum HashAlgorithm { HashAlgorithmUnknown, HashAlgorithmMd2, HashAlgorithmMd5, HashAlgorithmSha1, HashAlgorithmSha256, HashAlgorithmSha384, HashAlgorithmSha512, HashAlgorithmSha224 };

    /**
     * Destructor.
     */
    ~SignatureInfo();

    /**
     * The signature status of the signature.
     * @since 23.08
     */
    SignatureStatus signatureStatus() const;

    /**
     * Sets the signature status of the signature.
     * @since 23.08
     */
    void setSignatureStatus(SignatureStatus status);

    /**
     * The certificate status of the signature.
     * @since 23.08
     */
    CertificateStatus certificateStatus() const;

    /**
     * Sets the certificate status of the signature.
     * @since 23.08
     */
    void setCertificateStatus(CertificateStatus status);

    /**
     * The signer subject common name associated with the signature.
     * @since 23.08
     */
    QString signerName() const;

    /**
     * Sets the signer subject common name associated with the signature.
     * @since 23.08
     */
    void setSignerName(const QString &signerName);

    /**
     * The signer subject distinguished name associated with the signature.
     * @since 23.08
     */
    QString signerSubjectDN() const;

    /**
     * Sets the signer subject distinguished name associated with the signature.
     * @since 23.08
     */
    void setSignerSubjectDN(const QString &signerSubjectDN);

    /**
     * Get signing location.
     * @since 23.08
     */
    QString location() const;

    /**
     * Sets the signing location.
     * @since 23.08
     */
    void setLocation(const QString &location);

    /**
     * Get signing reason.
     * @since 23.08
     */
    QString reason() const;

    /**
     * Sets the signing reason.
     * @since 23.08
     */
    void setReason(const QString &reason);

    /**
     * The hash algorithm used for the signature.
     * @since 23.08
     */
    HashAlgorithm hashAlgorithm() const;

    /**
     * Sets the hash algorithm used for the signature.
     * @since 23.08
     */
    void setHashAlgorithm(HashAlgorithm algorithm);

    /**
     * The signing time associated with the signature.
     * @since 23.08
     */
    QDateTime signingTime() const;

    /**
     * Sets the signing time associated with the signature.
     * @since 23.08
     */
    void setSigningTime(const QDateTime &time);

    /**
     * Get the signature binary data.
     * @since 23.08
     */
    QByteArray signature() const;

    /**
     * Sets the signature binary data.
     * @since 23.08
     */
    void setSignature(const QByteArray &signature);

    /**
     * Get the bounds of the ranges of the document which are signed.
     * @since 23.08
     */
    QList<qint64> signedRangeBounds() const;

    /**
     * Sets the bounds of the ranges of the document which are signed.
     * @since 23.08
     */
    void setSignedRangeBounds(const QList<qint64> &range);

    /**
     * Checks whether the signature authenticates the total document
     * except for the signature itself.
     * @since 23.08
     */
    bool signsTotalDocument() const;

    /**
     * Checks whether the signature authenticates the total document
     * except for the signature itself.
     * @since 23.08
     */
    void setSignsTotalDocument(bool total);

    /**
     * Get certificate details.
     * @since 23.08
     */
    CertificateInfo certificateInfo() const;

    /**
     * Sets certificate details.
     * @since 23.08
     */
    void setCertificateInfo(const CertificateInfo &info);

    SignatureInfo();
    SignatureInfo(const SignatureInfo &other);
    SignatureInfo(SignatureInfo &&other) noexcept;
    SignatureInfo &operator=(const SignatureInfo &other);
    SignatureInfo &operator=(SignatureInfo &&other) noexcept;

private:
    QSharedDataPointer<SignatureInfoPrivate> d;
};

/**
 * @short A helper class to store information about x509 certificate
 */
class OKULARCORE_EXPORT CertificateStore
{
public:
    /**
     * Destructor
     */
    virtual ~CertificateStore();

    /**
     * Returns list of valid, usable signing certificates.
     *
     * This can ask the user for a password, userCancelled will be true if the user decided not to enter it.
     * @since 23.08
     */
    virtual QList<CertificateInfo> signingCertificates(bool *userCancelled) const;

    /**
     * Returns list of valid, usable signing certificates for current date and time.
     *
     * This can ask the user for a password, userCancelled will be true if the user decided not to enter it.
     *
     * nonDateValidCerts is true if the user has signing certificates but their validity start date is in the future or past their validity end date.
     * @since 23.08
     */
    QList<CertificateInfo> signingCertificatesForNow(bool *userCancelled, bool *nonDateValidCerts) const;

protected:
    CertificateStore();

private:
    Q_DISABLE_COPY(CertificateStore)
};

}

#endif
