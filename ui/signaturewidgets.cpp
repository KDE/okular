/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "signaturewidgets.h"
#include <KLocalizedString>

#include <QDebug>
#include <QLabel>
#include <QTextEdit>
#include <QTreeView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDialogButtonBox>

static QString getReadableSigState( Okular::SignatureInfo::SignatureStatus sigStatus )
{
    switch ( sigStatus )
    {
        case Okular::SignatureInfo::SignatureValid:
            return i18n("Signature is Valid.");
        case Okular::SignatureInfo::SignatureInvalid:
            return i18n("Signature is Invalid.");
        case Okular::SignatureInfo::SignatureDigestMismatch:
            return i18n("Digest Mismatch.");
        case Okular::SignatureInfo::SignatureDecodingError:
            return i18n("Document isn't signed or corrupted data.");
        case Okular::SignatureInfo::SignatureNotVerified:
            return i18n("Signature has not yet been verified.");
        default:
            return i18n("Unknown Validation Failure.");
    }
}

static QString getReadableCertState( Okular::SignatureInfo::CertificateStatus certStatus )
{
    switch ( certStatus )
    {
        case Okular::SignatureInfo::CertificateTrusted:
            return i18n("Certificate is Trusted.");
        case Okular::SignatureInfo::CertificateUntrustedIssuer:
            return i18n("Certificate issuer isn't Trusted.");
        case Okular::SignatureInfo::CertificateUnknownIssuer:
            return i18n("Certificate issuer is unknown.");
        case Okular::SignatureInfo::CertificateRevoked:
            return i18n("Certificate has been Revoked.");
        case Okular::SignatureInfo::CertificateExpired:
            return i18n("Certificate has Expired.");
        case Okular::SignatureInfo::CertificateNotVerified:
            return i18n("Certificate has not yet been verified.");
        default:
            return i18n("Unknown issue with Certificate or corrupted data.");
    }
}

static QString getReadableHashAlgorithm( Okular::SignatureInfo::HashAlgorithm hashAlg )
{
    switch ( hashAlg )
    {
        case Okular::SignatureInfo::HashAlgorithmMd2:
            return i18n("MD2");
        case Okular::SignatureInfo::HashAlgorithmMd5:
            return i18n("MD5");
        case Okular::SignatureInfo::HashAlgorithmSha1:
            return i18n("SHA1");
        case Okular::SignatureInfo::HashAlgorithmSha256:
            return i18n("SHA256");
        case Okular::SignatureInfo::HashAlgorithmSha384:
            return i18n("SHA384");
        case Okular::SignatureInfo::HashAlgorithmSha512:
            return i18n("SHA512");
        case Okular::SignatureInfo::HashAlgorithmSha224:
            return i18n("SHA224");
        default:
            return i18n("Unknown");
    }
}

SignaturePropertiesModel::SignaturePropertiesModel( Okular::SignatureInfo *sigInfo, QObject * parent )
  : QAbstractTableModel( parent )
{
    m_sigProperties.append( qMakePair( i18n("Subject Name"), sigInfo->subjectName() ) );
    m_sigProperties.append( qMakePair( i18n("Subject Distinguished Name"), sigInfo->subjectDN() ) );
    m_sigProperties.append( qMakePair( i18n("Signing Time"), sigInfo->signingTime().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_sigProperties.append( qMakePair( i18n("Hash Algorithm"), getReadableHashAlgorithm( sigInfo->hashAlgorithm() ) ) );
    m_sigProperties.append( qMakePair( i18n("Signature Status"), getReadableSigState( sigInfo->signatureStatus() ) ) );
    m_sigProperties.append( qMakePair( i18n("Certificate Status"), getReadableCertState( sigInfo->certificateStatus() ) ) );
    m_sigProperties.append( qMakePair( i18n("Signature Data"), QString::fromUtf8( sigInfo->signature().toHex(' ') ) ) );
    m_sigProperties.append( qMakePair( QStringLiteral("----------"), QString("------Certificate Properties--------") ) );

    Okular::CertificateInfo certInfo = sigInfo->certificateInfo();
    m_sigProperties.append( qMakePair( i18n("Version"), certInfo.version() ) );
    m_sigProperties.append( qMakePair( i18n("Issuer Name"), certInfo.issuerName() ) );
    m_sigProperties.append( qMakePair( i18n("Issuer Distinguished Name"), certInfo.issuerDN() ) );
    m_sigProperties.append( qMakePair( i18n("Serial Number"), certInfo.serialNumber() ) );
    m_sigProperties.append( qMakePair( i18n("Validity Start"), certInfo.validityStart().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_sigProperties.append( qMakePair( i18n("Validity End"), certInfo.validityEnd().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_sigProperties.append( qMakePair( i18n("Public Key"), certInfo.publicKey() ) );
}


int SignaturePropertiesModel::columnCount( const QModelIndex &parent ) const
{
    return parent.isValid() ? 0 : 2;
}

int SignaturePropertiesModel::rowCount( const QModelIndex &parent ) const
{
    return parent.isValid() ? 0 : m_sigProperties.size();
}

QVariant SignaturePropertiesModel::data( const QModelIndex &index, int role ) const
{
    int row = index.row();
    if ( !index.isValid() || row < 0 || row >= m_sigProperties.count() )
        return QVariant();

    switch ( role )
    {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        switch ( index.column() )
        {
            case 0:
                return m_sigProperties[row].first;
            case 1:
                return m_sigProperties[row].second;
            default:
                return QString();
        }
        case PropertyValueRole:
            return m_sigProperties[row].second;
    }

    return QVariant();
}

QVariant SignaturePropertiesModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( role == Qt::TextAlignmentRole )
        return QVariant( Qt::AlignLeft );

    if ( orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch ( section )
    {
        case 0:
            return i18n("Property");
        case 1:
            return i18n("Value");
        default:
            return QVariant();
    }
}


SignaturePropertiesDialog::SignaturePropertiesDialog( Okular::SignatureInfo *sigInfo, QWidget *parent )
    : QDialog( parent ), m_sigInfo( sigInfo )
{
    setModal( true );
    setFixedSize( QSize( 450, 500 ));
    setWindowTitle( i18n("Signature Properties") );

    auto sigPropLabel = new QLabel( this );
    sigPropLabel->setText( i18n("Signature Properties:") );

    auto sigPropTree = new QTreeView( this );
    sigPropTree->setIndentation( 0 );
    m_sigPropModel = new SignaturePropertiesModel( m_sigInfo, this );
    sigPropTree->setModel( m_sigPropModel );
    connect( sigPropTree, &QTreeView::clicked, this, &SignaturePropertiesDialog::updateText );

    m_sigPropText = new QTextEdit( this );
    m_sigPropText->setReadOnly( true );

    auto btnBox = new QDialogButtonBox( QDialogButtonBox::Close, this );
    btnBox->button( QDialogButtonBox::Close )->setDefault( true );
    connect( btnBox, &QDialogButtonBox::rejected, this, &SignatureSummaryDialog::reject );

    auto mainLayout = new QVBoxLayout( this );
    mainLayout->addWidget( sigPropLabel );
    mainLayout->addWidget( sigPropTree );
    mainLayout->addWidget( m_sigPropText );
    mainLayout->addWidget( btnBox );
    setLayout( mainLayout );
}

void SignaturePropertiesDialog::updateText( const QModelIndex &index )
{
    m_sigPropText->setText( m_sigPropModel->data( index, SignaturePropertiesModel::PropertyValueRole ).toString() );
}
SignatureSummaryDialog::SignatureSummaryDialog( Okular::SignatureInfo *sigInfo, QWidget *parent )
    : QDialog( parent ), m_sigInfo( sigInfo )
{
    setModal( true );
    setFixedSize( QSize(250, 100) );
    setWindowTitle( i18n("Signature Validation Status") );

    auto sigStatusLabel = new QLabel( this );
    sigStatusLabel->setText( getReadableSigState( m_sigInfo->signatureStatus() ) );

    auto btnBox = new QDialogButtonBox( QDialogButtonBox::Close, this );
    auto sigPropBtn = new QPushButton( i18n( "Signature Properties"), this );
    btnBox->button( QDialogButtonBox::Close )->setDefault( true );
    btnBox->addButton( sigPropBtn, QDialogButtonBox::ActionRole );
    connect( btnBox, &QDialogButtonBox::rejected, this, &SignatureSummaryDialog::reject );
    connect( sigPropBtn, &QPushButton::clicked, this, &SignatureSummaryDialog::showSignatureProperties );

    auto mainLayout = new QVBoxLayout( this );
    mainLayout->addWidget( sigStatusLabel );
    mainLayout->addWidget( btnBox );
    setLayout( mainLayout );
}

void SignatureSummaryDialog::showSignatureProperties()
{
    reject();
    SignaturePropertiesDialog sigPropDlg( m_sigInfo, this );
    sigPropDlg.exec();
}

#include "moc_signaturewidgets.cpp"
