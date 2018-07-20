/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "certificateviewer.h"

#include <KLocalizedString>

#include <QDebug>
#include <QLabel>
#include <QTextEdit>
#include <QTreeView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QTextDocument>
#include <QHeaderView>
#include <QVector>
#include <KMessageBox>
#include <KMessageWidget>
#include <QCryptographicHash>
#include <QFileDialog>

#include "core/form.h"
#include "guiutils.h"


CertificateModel::CertificateModel( Okular::SignatureInfo *sigInfo, QObject * parent )
  : QAbstractTableModel( parent )
{
    certInfo = sigInfo->certificateInfo();
    m_certificateProperties.append( qMakePair( i18n("Version"), i18n("V%1", QString::number( certInfo->version() )) ) );
    m_certificateProperties.append( qMakePair( i18n("Serial Number"), certInfo->serialNumber().toHex(' ') ) );
    m_certificateProperties.append( qMakePair( i18n("Hash Algorithm"), GuiUtils::getReadableHashAlgorithm( sigInfo->hashAlgorithm() ) ) );
    m_certificateProperties.append( qMakePair( i18n("Issuer"), certInfo->issuerInfo(Okular::CertificateInfo::DistinguishedName) ) );
    m_certificateProperties.append( qMakePair( i18n("Issued On"), certInfo->validityStart().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_certificateProperties.append( qMakePair( i18n("Expires On"), certInfo->validityEnd().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_certificateProperties.append( qMakePair( i18n("Subject"), certInfo->subjectInfo(Okular::CertificateInfo::DistinguishedName) ) );
    m_certificateProperties.append( qMakePair( i18n("Public Key"), i18n("%1 (%2 bits)", GuiUtils::getReadablePublicKeyType( certInfo->publicKeyType() ),
                                                                        certInfo->publicKeyStrength()) ) );
    m_certificateProperties.append( qMakePair( i18n("Key Usage"), GuiUtils::getReadableKeyUsage( certInfo->keyUsageExtensions() ) ) );
}


int CertificateModel::columnCount( const QModelIndex & ) const
{
    return 2;
}

int CertificateModel::rowCount( const QModelIndex & ) const
{
    return m_certificateProperties.size();
}

QVariant CertificateModel::data( const QModelIndex &index, int role ) const
{
    int row = index.row();
    if ( !index.isValid() || row < 0 || row >= m_certificateProperties.count() )
        return QVariant();

    switch ( role )
    {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        switch ( index.column() )
        {
            case 0:
                return m_certificateProperties[row].first;
            case 1:
                    return m_certificateProperties[row].second;
            default:
                return QString();
        }
        case PropertyKeyRole:
            return m_certificateProperties[row].first;
        case PropertyValueRole:
            return m_certificateProperties[row].second;
        case PublicKeyRole:
            return QString( certInfo->publicKey().toHex(' ') );
    }

    return QVariant();
}

QVariant CertificateModel::headerData( int section, Qt::Orientation orientation, int role ) const
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

CertificateViewer::CertificateViewer( Okular::SignatureInfo *sigInfo, QWidget *parent )
    : KPageDialog( parent ), m_sigInfo( sigInfo )
{
    setModal( true );
    setMinimumSize( QSize( 500, 550 ));
    setFaceType( Tabbed );
    setWindowTitle( i18n("Certificate Viewer") );
    setStandardButtons( QDialogButtonBox::Close );

    auto certInfo = sigInfo->certificateInfo();
    auto exportBtn = new QPushButton( i18n("Export...") );
    connect( exportBtn, &QPushButton::clicked, this, [=]{
        QString filename = QDir::homePath() + i18n("/Certificate.cer");
        const QString caption = i18n( "Where do you want to save %1?", filename );
        const QString path = QFileDialog::getSaveFileName( this, caption, filename, i18n("Certificate File (*.cer)") );

        QFile targetFile( path );
        targetFile.open( QIODevice::WriteOnly );
        qWarning() << targetFile.write( certInfo->certificateData() );
        targetFile.close();
    } );
    addActionButton( exportBtn );

    // General tab
    auto generalPage = new QFrame( this );
    addPage( generalPage, i18n("General") );

    auto issuerBox = new QGroupBox( i18n("Issued By"), generalPage );
    auto issuerFormLayout = new QFormLayout( issuerBox );
    issuerFormLayout->setLabelAlignment( Qt::AlignLeft );
    issuerFormLayout->setHorizontalSpacing( 50 );
    issuerFormLayout->addRow( i18n("Common Name(CN)"), new QLabel( certInfo->issuerInfo( Okular::CertificateInfo::CommonName ) ) );
    issuerFormLayout->addRow( i18n("EMail"), new QLabel( certInfo->issuerInfo( Okular::CertificateInfo::EmailAddress ) ) );
    issuerFormLayout->addRow( i18n("Organization(O)"), new QLabel( certInfo->issuerInfo( Okular::CertificateInfo::Organization ) ) );

    auto subjectBox = new QGroupBox( i18n("Issued To"), generalPage );
    auto subjectFormLayout = new QFormLayout( subjectBox );
    subjectFormLayout->setLabelAlignment( Qt::AlignLeft );
    subjectFormLayout->setHorizontalSpacing( 50 );
    subjectFormLayout->addRow( i18n("Common Name(CN)"), new QLabel( certInfo->subjectInfo( Okular::CertificateInfo::CommonName ) ) );
    subjectFormLayout->addRow( i18n("EMail"), new QLabel( certInfo->subjectInfo( Okular::CertificateInfo::EmailAddress ) ) );
    subjectFormLayout->addRow( i18n("Organization(O)"), new QLabel( certInfo->subjectInfo( Okular::CertificateInfo::Organization ) ) );

    auto validityBox = new QGroupBox( i18n("Validity"), generalPage );
    auto validityFormLayout = new QFormLayout( validityBox );
    validityFormLayout->setLabelAlignment( Qt::AlignLeft );
    validityFormLayout->setHorizontalSpacing( 100 );
    validityFormLayout->addRow( i18n("Issued On"), new QLabel( certInfo->validityStart().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    validityFormLayout->addRow( i18n("Expires On"), new QLabel( certInfo->validityEnd().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );

    auto fingerprintBox = new QGroupBox( i18n("Fingerprints"), generalPage );
    auto fingerprintFormLayout = new QFormLayout( fingerprintBox );
    fingerprintFormLayout->setLabelAlignment( Qt::AlignLeft );
    fingerprintFormLayout->setHorizontalSpacing( 50 );
    QByteArray certData = certInfo->certificateData();
    auto sha1Label = new QLabel( QString( QCryptographicHash::hash( certData, QCryptographicHash::Sha1 ).toHex(' ') ) );
    sha1Label->setWordWrap( true );
    auto sha256Label = new QLabel( QString( QCryptographicHash::hash( certData, QCryptographicHash::Sha256 ).toHex(' ') ) );
    sha256Label->setWordWrap( true );
    fingerprintFormLayout->addRow( i18n("SHA-1 Fingerprint"), sha1Label );
    fingerprintFormLayout->addRow( i18n("SHA-256 Fingerprint"), sha256Label );

    auto generalPageLayout = new QVBoxLayout( generalPage );
    generalPageLayout->addWidget( issuerBox );
    generalPageLayout->addWidget( subjectBox );
    generalPageLayout->addWidget( validityBox );
    generalPageLayout->addWidget( fingerprintBox );

    // Details tab
    auto detailsFrame = new QFrame( this );
    addPage( detailsFrame, i18n("Details") );
    auto certDataLabel = new QLabel( i18n("Certificate Data:") );
    auto certTree = new QTreeView( this );
    certTree->setIndentation( 0 );
    m_certModel = new CertificateModel( m_sigInfo, this );
    certTree->setModel( m_certModel );
    connect( certTree, &QTreeView::activated, this, &CertificateViewer::updateText );
    m_propertyText = new QTextEdit( this );
    m_propertyText->setReadOnly( true );

    auto detailsPageLayout = new QVBoxLayout( detailsFrame );
    detailsPageLayout->addWidget( certDataLabel );
    detailsPageLayout->addWidget( certTree );
    detailsPageLayout->addWidget( m_propertyText );

    //mainLayout->addWidget( btnBox );
    //setLayout( mainLayout );
}

void CertificateViewer::updateText( const QModelIndex &index )
{
    QString key = m_certModel->data( index, CertificateModel::PropertyKeyRole ).toString();
    if ( key == QLatin1String("Public Key") )
        m_propertyText->setText( m_certModel->data( index, CertificateModel::PublicKeyRole ).toString() );
    else
    {
        QString textToView;
        QString propertyValue = m_certModel->data( index, CertificateModel::PropertyValueRole ).toString();
        foreach ( auto c, propertyValue )
        {
            if ( c == ',' )
                textToView += '\n';
            else
                textToView += c;
        }
        m_propertyText->setText( textToView );
    }
}
