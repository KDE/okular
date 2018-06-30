/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "pdfsignatureutils.h"

PopplerCertificateInfo::PopplerCertificateInfo( const Poppler::CertificateInfo &info )
    : Okular::CertificateInfo()
{
    initPrivate();

    setVersion( info.version() );
    setIssuerName( info.issuerName() );
    setIssuerDN( info.issuerDN() );
    setSerialNumber( info.serialNumber() );
    setValidityStart( info.validityStart() );
    setValidityEnd( info.validityEnd() );
    setKeyUsages( convertToOkularKeyUsages( info.keyUsages() ) );
    setPublicKey( info.publicKey() );
    setPublicKeyStrength( info.publicKeyStrength() );
    setPublicKeyType( convertToOkularPublicKeyType( info.publicKeyType() ) );
    setCertificateData( info.certificateData() );
}

PopplerCertificateInfo::~PopplerCertificateInfo()
{
}

PopplerCertificateInfo::KeyUsages PopplerCertificateInfo::convertToOkularKeyUsages( Poppler::CertificateInfo::KeyUsages popplerKu )
{
    KeyUsages ku = KuNone;
    if ( popplerKu.testFlag( Poppler::CertificateInfo::KuDigitalSignature ) )
        ku |= KuDigitalSignature;
    if ( popplerKu.testFlag( Poppler::CertificateInfo::KuNonRepudiation ) )
        ku |= KuNonRepudiation;
    if ( popplerKu.testFlag( Poppler::CertificateInfo::KuKeyEncipherment ) )
        ku |= KuKeyEncipherment;
    if ( popplerKu.testFlag( Poppler::CertificateInfo::KuDataEncipherment ) )
        ku |= KuDataEncipherment;
    if ( popplerKu.testFlag( Poppler::CertificateInfo::KuKeyAgreement ) )
        ku |= KuKeyAgreement;
    if ( popplerKu.testFlag( Poppler::CertificateInfo::KuKeyCertSign ) )
        ku |= KuKeyCertSign;
    if ( popplerKu.testFlag( Poppler::CertificateInfo::KuClrSign ) )
        ku |= KuClrSign;
    if ( popplerKu.testFlag( Poppler::CertificateInfo::KuEncipherOnly ) )
        ku |= KuEncipherOnly;
    return ku;
}

PopplerCertificateInfo::PublicKeyType PopplerCertificateInfo::convertToOkularPublicKeyType( Poppler::CertificateInfo::PublicKeyType popplerPkType )
{
    switch ( popplerPkType )
    {
        case Poppler::CertificateInfo::RsaKey:
            return RsaKey;
        case Poppler::CertificateInfo::DsaKey:
            return DsaKey;
        case Poppler::CertificateInfo::EcKey:
            return EcKey;
        case Poppler::CertificateInfo::OtherKey:
            return OtherKey;
    }
}


class PopplerSignatureInfoPrivate
{
    public:
        QList<qint64> rangeBounds;
        QDateTime signingTime;
        QByteArray signature;
        QString subjectCN;
        QString subjectDN;
        int signatureStatus;
        int certficateStatus;
        int hashAlgorithm;
        bool signsTotalDoc;
};

PopplerSignatureInfo::PopplerSignatureInfo( const Poppler::SignatureValidationInfo &info )
    : d_ptr( new PopplerSignatureInfoPrivate() )
{
    Q_D( PopplerSignatureInfo );
    d->signatureStatus = info.signatureStatus();
    d->certficateStatus = info.certificateStatus();
    d->subjectCN = info.signerName();
    d->subjectDN = info.signerSubjectDN();
    d->hashAlgorithm = info.hashAlgorithm();
    d->signingTime = QDateTime::fromTime_t( info.signingTime() );
    d->signature = info.signature();
    d->rangeBounds = info.signedRangeBounds();
    d->signsTotalDoc = info.signsTotalDocument();
}

PopplerSignatureInfo::~PopplerSignatureInfo()
{
}

PopplerSignatureInfo::SignatureStatus PopplerSignatureInfo::signatureStatus() const
{
    Q_D( const PopplerSignatureInfo );
    switch ( d->signatureStatus )
    {
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
    Q_D( const PopplerSignatureInfo );
    switch ( d->certficateStatus )
    {
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
    Q_D( const PopplerSignatureInfo );
    switch ( d->hashAlgorithm )
    {
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

QString PopplerSignatureInfo::subjectName() const
{
    Q_D( const PopplerSignatureInfo );
    return d->subjectCN;
}
QString PopplerSignatureInfo::subjectDN() const
{
    Q_D( const PopplerSignatureInfo );
    return d->subjectDN;
}

QDateTime PopplerSignatureInfo::signingTime() const
{
    Q_D( const PopplerSignatureInfo );
    return d->signingTime;
}

QByteArray PopplerSignatureInfo::signature() const
{
    Q_D( const PopplerSignatureInfo );
    return d->signature;
}

QList<qint64> PopplerSignatureInfo::signedRangeBounds() const
{
    Q_D( const PopplerSignatureInfo );
    return d->rangeBounds;
}

bool PopplerSignatureInfo::signsTotalDocument() const
{
    Q_D( const PopplerSignatureInfo );
    return d->signsTotalDoc;
}

Okular::CertificateInfo PopplerSignatureInfo::certificateInfo() const
{
    return Okular::CertificateInfo();
}
