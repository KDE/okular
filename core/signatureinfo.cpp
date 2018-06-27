/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "signatureinfo.h"

using namespace Okular;

class Okular::CertificateInfoPrivate
{
    public:
        CertificateInfoPrivate()
            : pkType( CertificateInfo::OtherKey ), keyUsages( CertificateInfo::KuNone ),
              certificateDer( QByteArray() ), serialNumber( QByteArray() ), publicKey( QByteArray() ),
              version( QByteArray() ), validityStart( QDateTime() ), validityEnd( QDateTime() ),
              issuerName( QString() ), issuerDN( QString() ), pkStrength( -1 )
        {
        }

        CertificateInfo::PublicKeyType pkType;
        CertificateInfo::KeyUsages keyUsages;

        QByteArray certificateDer;
        QByteArray serialNumber;
        QByteArray publicKey;
        QByteArray version;
        QDateTime validityStart;
        QDateTime validityEnd;
        QString issuerName;
        QString issuerDN;
        int pkStrength;
};

CertificateInfo::CertificateInfo( CertificateInfoPrivate *priv )
    : d_ptr( priv )
{
}

CertificateInfo::CertificateInfo( const CertificateInfo &other )
    : d_ptr( other.d_ptr )
{
}

CertificateInfo &CertificateInfo::operator=( const CertificateInfo &other )
{
    if ( this != &other )
        d_ptr = other.d_ptr;

    return *this;
}

CertificateInfo::~CertificateInfo()
{
}

Q_DECLARE_OPERATORS_FOR_FLAGS( CertificateInfo::KeyUsages )

bool CertificateInfo::isValid() const
{
    return d_ptr.data() != nullptr;
}

QByteArray CertificateInfo::version() const
{
    Q_D( const CertificateInfo );
    return d->version;
}

QString CertificateInfo::issuerName() const
{
    Q_D( const CertificateInfo );
    return d->issuerName;
}

QString CertificateInfo::issuerDN() const
{
    Q_D( const CertificateInfo );
    return d->issuerDN;
}

QByteArray CertificateInfo::serialNumber() const
{
    Q_D( const CertificateInfo );
    return d->serialNumber;
}

QDateTime CertificateInfo::validityStart() const
{
    Q_D( const CertificateInfo );
    return d->validityStart;
}

QDateTime CertificateInfo::validityEnd() const
{
    Q_D( const CertificateInfo );
    return d->validityEnd;
}

CertificateInfo::KeyUsages CertificateInfo::keyUsages() const
{
    Q_D( const CertificateInfo );
    return d->keyUsages;
}

QByteArray CertificateInfo::publicKey() const
{
    Q_D( const CertificateInfo );
    return d->publicKey;
}


CertificateInfo::PublicKeyType CertificateInfo::publicKeyType() const
{
    Q_D( const CertificateInfo );
    return d->pkType;
}

int CertificateInfo::publicKeyStrength() const
{
    Q_D( const CertificateInfo );
    return d->pkStrength;
}

QByteArray CertificateInfo::certificateData() const
{
    Q_D( const CertificateInfo );
    return d->certificateDer;
}

void CertificateInfo::initPrivate()
{
    if ( d_ptr.isNull() )
        d_ptr = QSharedPointer<CertificateInfoPrivate>( new CertificateInfoPrivate );
}

void CertificateInfo::setVersion( const QByteArray &version )
{
    Q_D( CertificateInfo );
    d->version = version;
}

void CertificateInfo::setIssuerName( const QString &issuerName )
{
    Q_D( CertificateInfo );
    d->issuerName = issuerName;
}

void CertificateInfo::setIssuerDN( const QString &issuerDN )
{
    Q_D( CertificateInfo );
    d->issuerDN = issuerDN;
}

void CertificateInfo::setSerialNumber( const QByteArray &serialNumber )
{
    Q_D( CertificateInfo );
    d->serialNumber = serialNumber;
}

void CertificateInfo::setValidityStart( const QDateTime &validityStart )
{
    Q_D( CertificateInfo );
    d->validityStart = validityStart;
}

void CertificateInfo::setValidityEnd( const QDateTime & validityEnd )
{
    Q_D( CertificateInfo );
    d->validityEnd = validityEnd;
}

void CertificateInfo::setKeyUsages( KeyUsages ku )
{
    Q_D( CertificateInfo );
    d->keyUsages = ku;
}

void CertificateInfo::setPublicKey( const QByteArray &publicKey )
{
    Q_D( CertificateInfo );
    d->publicKey = publicKey;
}

void CertificateInfo::setPublicKeyType( PublicKeyType pkType )
{
    Q_D( CertificateInfo );
    d->pkType = pkType;
}

void CertificateInfo::setPublicKeyStrength( int pkStrength )
{
    Q_D( CertificateInfo );
    d->pkStrength = pkStrength;
}

void CertificateInfo::setCertificateData( const QByteArray &data )
{
    Q_D( CertificateInfo );
    d->certificateDer = data;
}


class Okular::SignatureInfoPrivate
{
    public:
        SignatureInfoPrivate()
            : signatureStatus( SignatureInfo::SignatureStatusUnknown ),
              certificateStatus( SignatureInfo::CertificateStatusUnknown ),
              hashAlgorithm( SignatureInfo::HashAlgorithmUnknown ),
              rangeBounds( QList<qint64>() ), signingTime( QDateTime() ), signature( QByteArray() ),
              subjectName( QString() ), subjectDN( QString() ), signsTotalDoc( false )
        {
        }

        SignatureInfo::SignatureStatus signatureStatus;
        SignatureInfo::CertificateStatus certificateStatus;
        SignatureInfo::HashAlgorithm hashAlgorithm;

        CertificateInfo certInfo;
        QList<qint64> rangeBounds;
        QDateTime signingTime;
        QByteArray signature;
        QString subjectName;
        QString subjectDN;
        bool signsTotalDoc;
};

SignatureInfo::SignatureInfo( SignatureInfoPrivate *priv )
    : d_ptr( priv )
{
}

SignatureInfo::SignatureInfo( const SignatureInfo &other )
    : d_ptr( other.d_ptr )
{
}

SignatureInfo &SignatureInfo::operator=( const SignatureInfo &other )
{
    if ( this != &other )
        d_ptr = other.d_ptr;

    return *this;
}

SignatureInfo::~SignatureInfo()
{
}

bool SignatureInfo::isValid() const
{
    return d_ptr.data() != nullptr;
}

SignatureInfo::SignatureStatus SignatureInfo::signatureStatus() const
{
    Q_D( const SignatureInfo );
    return d->signatureStatus;
}

SignatureInfo::CertificateStatus SignatureInfo::certificateStatus() const
{
    Q_D( const SignatureInfo);
    return d->certificateStatus;

}

SignatureInfo::HashAlgorithm SignatureInfo::hashAlgorithm() const
{
    Q_D( const SignatureInfo );
    return d->hashAlgorithm;
}

QString SignatureInfo::subjectName() const
{
    Q_D( const SignatureInfo );
    return d->subjectName;
}

QString SignatureInfo::subjectDN() const
{
    Q_D( const SignatureInfo );
    return d->subjectDN;
}

QDateTime SignatureInfo::signingTime() const
{
    Q_D( const SignatureInfo );
    return d->signingTime;
}

QByteArray SignatureInfo::signature() const
{
    Q_D( const SignatureInfo );
    return d->signature;
}

QList<qint64> SignatureInfo::signedRangeBounds() const
{
    Q_D( const SignatureInfo );
    return d->rangeBounds;
}

bool SignatureInfo::signsTotalDocument() const
{
    Q_D( const SignatureInfo );
    return d->signsTotalDoc;
}

CertificateInfo SignatureInfo::certificateInfo() const
{
    Q_D( const SignatureInfo );
    return d->certInfo;
}

void SignatureInfo::initPrivate()
{
    if ( d_ptr.isNull() )
        d_ptr = QSharedPointer<SignatureInfoPrivate>( new SignatureInfoPrivate );
}

void SignatureInfo::setSignatureStatus( SignatureStatus sigStatus )
{
    Q_D( SignatureInfo );
    d->signatureStatus = sigStatus;
}

void SignatureInfo::setCertificateStatus( CertificateStatus certStatus )
{
    Q_D( SignatureInfo );
    d->certificateStatus = certStatus;
}

void SignatureInfo::setSubjectName( const QString &subjectName )
{
    Q_D( SignatureInfo );
    d->subjectName = subjectName;
}

void SignatureInfo::setSubjectDN( const QString &subjectDN )
{
    Q_D( SignatureInfo );
    d->subjectDN = subjectDN;
}

void SignatureInfo::setHashAlgorithm( HashAlgorithm hashAlgorithm )
{
    Q_D( SignatureInfo );
    d->hashAlgorithm = hashAlgorithm;
}

void SignatureInfo::setSigningTime( const QDateTime & signingTime)
{
    Q_D( SignatureInfo );
    d->signingTime = signingTime;
}

void SignatureInfo::setSignature( const QByteArray &data )
{
    Q_D( SignatureInfo );
    d->signature = data;
}

void SignatureInfo::setSignedRangeBounds( const QList<qint64> &rangeBounds )
{
    Q_D( SignatureInfo );
    d->rangeBounds = rangeBounds;
}

void SignatureInfo::setSignsTotalDocument( bool signsTotalDoc )
{
    Q_D( SignatureInfo );
    d->signsTotalDoc = signsTotalDoc;
}

void SignatureInfo::setCertificateInfo( CertificateInfo certInfo )
{
    Q_D( SignatureInfo );
    d->certInfo = certInfo;
}
