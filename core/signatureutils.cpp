/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "signatureutils.h".h"

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


SignatureInfo::SignatureInfo()
{
}

SignatureInfo::~SignatureInfo()
{
}

SignatureInfo::SignatureStatus SignatureInfo::signatureStatus() const
{
    return SignatureStatusUnknown;
}

SignatureInfo::CertificateStatus SignatureInfo::certificateStatus() const
{
    return CertificateStatusUnknown;

}

SignatureInfo::HashAlgorithm SignatureInfo::hashAlgorithm() const
{
    return HashAlgorithmUnknown;
}

QString SignatureInfo::subjectName() const
{
    return QString();
}

QString SignatureInfo::subjectDN() const
{
    return QString();
}

QDateTime SignatureInfo::signingTime() const
{
    return QDateTime();
}

QByteArray SignatureInfo::signature() const
{
    return QByteArray();
}

QList<qint64> SignatureInfo::signedRangeBounds() const
{
    return QList<qint64>();
}

bool SignatureInfo::signsTotalDocument() const
{
    return false;
}

CertificateInfo SignatureInfo::certificateInfo() const
{
    return CertificateInfo();
}
