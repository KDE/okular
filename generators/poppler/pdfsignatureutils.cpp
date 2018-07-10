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
    : m_info( new Poppler::CertificateInfo( nullptr ) )
{
    *m_info = info;
}

PopplerCertificateInfo::~PopplerCertificateInfo()
{
    delete m_info;
}

QByteArray PopplerCertificateInfo::version() const
{
    return m_info->version();
}

QString PopplerCertificateInfo::issuerName() const
{
    return m_info->issuerName();
}

QString PopplerCertificateInfo::issuerDN() const
{
    return m_info->issuerDN();
}

QByteArray PopplerCertificateInfo::serialNumber() const
{
    return m_info->serialNumber();
}

QDateTime PopplerCertificateInfo::validityStart() const
{
    return m_info->validityStart();
}

QDateTime PopplerCertificateInfo::validityEnd() const
{
    return m_info->validityEnd();
}

PopplerCertificateInfo::KeyUsages PopplerCertificateInfo::keyUsages() const
{
    Poppler::CertificateInfo::KeyUsages popplerKu = m_info->keyUsages();
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

QByteArray PopplerCertificateInfo::publicKey() const
{
    return m_info->publicKey();
}

PopplerCertificateInfo::PublicKeyType PopplerCertificateInfo::publicKeyType() const
{
    switch ( m_info->publicKeyType() )
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

int PopplerCertificateInfo::publicKeyStrength() const
{
    return m_info->publicKeyStrength();
}

bool PopplerCertificateInfo::isSelfSigned() const
{
    return m_info->isSelfSigned();
}

QByteArray PopplerCertificateInfo::certificateData() const
{
    return m_info->certificateData();
}


PopplerSignatureInfo::PopplerSignatureInfo( const Poppler::SignatureValidationInfo &info )
    : m_info( new Poppler::SignatureValidationInfo( nullptr ) )
{
    *m_info = info;
}

PopplerSignatureInfo::~PopplerSignatureInfo()
{
    delete m_info;
}

PopplerSignatureInfo::SignatureStatus PopplerSignatureInfo::signatureStatus() const
{
    switch ( m_info->signatureStatus() )
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
    switch ( m_info->certificateStatus() )
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
    switch ( m_info->hashAlgorithm() )
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
    return m_info->signerName();
}

QString PopplerSignatureInfo::subjectDN() const
{
    return m_info->signerSubjectDN();
}

QDateTime PopplerSignatureInfo::signingTime() const
{
    return QDateTime::fromTime_t( m_info->signingTime() );
}

QByteArray PopplerSignatureInfo::signature() const
{
    return m_info->signature();
}

QList<qint64> PopplerSignatureInfo::signedRangeBounds() const
{
    return m_info->signedRangeBounds();
}

bool PopplerSignatureInfo::signsTotalDocument() const
{
    return m_info->signsTotalDocument();
}

QString PopplerSignatureInfo::location() const
{
    return m_info->location();
}

QString PopplerSignatureInfo::reason() const
{
    return m_info->reason();
}

Okular::CertificateInfo *PopplerSignatureInfo::certificateInfo() const
{
    return ( new PopplerCertificateInfo( m_info->certificateInfo() ) );
}
