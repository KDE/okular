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


PopplerSignatureInfo::PopplerSignatureInfo(const Poppler::SignatureValidationInfo &info)
    : Okular::SignatureInfo()
{
    initPrivate();

    setSignatureStatus( convertToOkularSigStatus( info.signatureStatus() ) );
    setCertificateStatus( convertToOkularCertStatus( info.certificateStatus() ) );
    setSubjectName( info.signerName() );
    setSubjectDN( info.signerSubjectDN() );
    setHashAlgorithm( convertToOkularHashAlg( info.hashAlgorithm() ) );
    setSigningTime( QDateTime::fromTime_t( info.signingTime() ) );
    setSignature( info.signature() );
    setSignedRangeBounds( info.signedRangeBounds() );
    setSignsTotalDocument( info.signsTotalDocument() );
    setCertificateInfo( PopplerCertificateInfo( info.certificateInfo() ) );
}

PopplerSignatureInfo::~PopplerSignatureInfo()
{
}

PopplerSignatureInfo::SignatureStatus PopplerSignatureInfo::convertToOkularSigStatus( Poppler::SignatureValidationInfo::SignatureStatus popplerSigStatus)
{
    switch( popplerSigStatus )
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

PopplerSignatureInfo::CertificateStatus PopplerSignatureInfo::convertToOkularCertStatus( Poppler::SignatureValidationInfo::CertificateStatus popplerCertStatus )
{
    switch( popplerCertStatus )
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

PopplerSignatureInfo::HashAlgorithm PopplerSignatureInfo::convertToOkularHashAlg( Poppler::SignatureValidationInfo::HashAlgorithm popplerHashAlg )
{
    switch( popplerHashAlg )
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
        case Poppler::SignatureValidationInfo::HashAlgorithmUnknown:
            return HashAlgorithmUnknown;
    }
}
