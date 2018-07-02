/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SIGNATUREINFO_H
#define OKULAR_SIGNATUREINFO_H

#include "okularcore_export.h"

#include <QFlag>
#include <QList>
#include <QString>
#include <QDateTime>
#include <QSharedPointer>

namespace Okular {

class CertificateInfoPrivate;
class SignatureInfoPrivate;

/**
 * @short A helper class to store information x509 digital certificate
 */
class OKULARCORE_EXPORT CertificateInfo
{
    public:

        /**
         * The algorithm of public key.
         */
        enum PublicKeyType
        {
            RsaKey,
            DsaKey,
            EcKey,
            OtherKey
        };

        /**
         * Certificate key usage.
         */
        enum KeyUsage
        {
            KuNone = 0,
            KuDigitalSignature = 1,
            KuNonRepudiation = 2,
            KuKeyEncipherment = 4,
            KuDataEncipherment = 8,
            KuKeyAgreement = 16,
            KuKeyCertSign = 32,
            KuClrSign = 64,
            KuEncipherOnly = 128
        };
        Q_DECLARE_FLAGS( KeyUsages, KeyUsage )

        /**
         * Destructor
         */
        virtual ~CertificateInfo();

        /**
         * The certificate version string in hex encoding.
         */
        virtual QByteArray version() const;

        /**
         * The common name of certificate issuer.
         */
        virtual QString issuerName() const;

        /**
         * The distinguished name of certificate issuer.
         */
        virtual QString issuerDN() const;

        /**
        The hex encoded certificate serial number.
         */
        virtual QByteArray serialNumber() const;

        /**
        The date-time when certificate becomes valid.
          */
        virtual QDateTime validityStart() const;

        /**
         * The date-time when certificate expires.
         */
        virtual QDateTime validityEnd() const;

        /**
         * The key usages of certificate.
         */
        virtual KeyUsages keyUsages() const;

        /**
         * The public key value.
         */
        virtual QByteArray publicKey() const;

        /**
         * The public key type.
         */
        virtual PublicKeyType publicKeyType() const;

        /**
         * The strength of public key in bits or -1 in case
         * key type is 'OtherKey'.
         */
        virtual int publicKeyStrength() const;

        /**
         * Returns true if certificate is self signed; otherwise returns false.
         */
        virtual bool isSelfSigned() const;

        /**
         * The DER encoded certificate.
         */
        virtual QByteArray certificateData() const;

    protected:
        CertificateInfo();

    private:
        Q_DISABLE_COPY( CertificateInfo )
};

/**
 * @short A helper class to store information about digital signature
 */
class OKULARCORE_EXPORT SignatureInfo
{
    public:

        /**
         * The verfication result of the signature.
         */
        enum SignatureStatus
        {
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
        enum CertificateStatus
        {
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
        enum HashAlgorithm
        {
            HashAlgorithmUnknown,
            HashAlgorithmMd2,
            HashAlgorithmMd5,
            HashAlgorithmSha1,
            HashAlgorithmSha256,
            HashAlgorithmSha384,
            HashAlgorithmSha512,
            HashAlgorithmSha224
        };

        /**
         * Destructor.
         */
        virtual ~SignatureInfo();

        /**
         * The signature status of the signature.
         */
        virtual SignatureStatus signatureStatus() const;

        /**
         * The certificate status of the signature.
         */
        virtual CertificateStatus certificateStatus() const;

        /**
         * The signer subject common name associated with the signature.
         */
        virtual QString subjectName() const;

        /**
         * The signer subject distinguished name associated with the signature.
         */
        virtual QString subjectDN() const;

        /**
         * The the hash algorithm used for the signature.
         */
        virtual HashAlgorithm hashAlgorithm() const;

        /**
         * The signing time associated with the signature.
         */
        virtual QDateTime signingTime() const;

        /**
         * Get the signature binary data.
         */
        virtual QByteArray signature() const;

        /**
         * Get the bounds of the ranges of the document which are signed.
         */
        virtual QList<qint64> signedRangeBounds() const;

        /**
         * Checks whether the signature authenticates the total document
         * except for the signature itself.
         */
        virtual bool signsTotalDocument() const;

        /**
         * Get location.
         */
        virtual QString location() const;

        /**
         * Get signing reason.
         */
        virtual QString reason() const;

        /**
         * Get certificate details.
         */
        virtual CertificateInfo *certificateInfo() const;

    protected:
        SignatureInfo();

    private:
        Q_DISABLE_COPY( SignatureInfo )
};

}

#endif
