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
#include <QGroupBox>
#include <QFormLayout>
#include <QTreeView>
#include <QTextDocument>
#include <KIconLoader>
#include <QPainter>
#include <QPaintEvent>
#include <QHeaderView>
#include <QVector>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <KMessageBox>
#include <KMessageWidget>

#include "core/form.h"
#include "core/page.h"
#include "core/document.h"
#include "core/sourcereference.h"
#include "core/form.h"
#include "settings.h"
#include "guiutils.h"


CertificateViewerModel::CertificateViewerModel( Okular::SignatureInfo *sigInfo, QObject * parent )
  : QAbstractTableModel( parent )
{
    m_sigProperties.append( qMakePair( i18n("Subject Name"), sigInfo->signerName() ) );
    m_sigProperties.append( qMakePair( i18n("Subject Distinguished Name"), sigInfo->signerSubjectDN() ) );
    m_sigProperties.append( qMakePair( i18n("Signing Time"), sigInfo->signingTime().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_sigProperties.append( qMakePair( i18n("Hash Algorithm"), GuiUtils::getReadableHashAlgorithm( sigInfo->hashAlgorithm() ) ) );
    m_sigProperties.append( qMakePair( i18n("Signature Status"), GuiUtils::getReadableSigState( sigInfo->signatureStatus() ) ) );
    m_sigProperties.append( qMakePair( i18n("Certificate Status"), GuiUtils::getReadableCertState( sigInfo->certificateStatus() ) ) );
    m_sigProperties.append( qMakePair( i18n("Signature Data"), QString::fromUtf8( sigInfo->signature().toHex(' ') ) ) );
    m_sigProperties.append( qMakePair( i18n("Location"), QString( sigInfo->location() ) ) );
    m_sigProperties.append( qMakePair( i18n("Reason"), QString( sigInfo->reason() ) ) );
    m_sigProperties.append( qMakePair( QStringLiteral("----------"), QString("------Certificate Properties--------") ) );

    Okular::CertificateInfo *certInfo = sigInfo->certificateInfo();
    m_sigProperties.append( qMakePair( i18n("Version"), QString("V" + QString::number(certInfo->version()) ) ) );
    m_sigProperties.append( qMakePair( i18n("Issuer Name"), certInfo->issuerInfo(Okular::CertificateInfo::CommonName) ) );
    m_sigProperties.append( qMakePair( i18n("Issuer Distinguished Name"), certInfo->issuerInfo(Okular::CertificateInfo::DistinguishedName) ) );
    m_sigProperties.append( qMakePair( i18n("Serial Number"), certInfo->serialNumber().toHex(' ') ) );
    m_sigProperties.append( qMakePair( i18n("Validity Start"), certInfo->validityStart().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_sigProperties.append( qMakePair( i18n("Validity End"), certInfo->validityEnd().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_sigProperties.append( qMakePair( i18n("Public Key"), certInfo->publicKey().toHex(' ') ) );
    m_sigProperties.append( qMakePair( i18n("Is Self Signed"), certInfo->isSelfSigned() ? QString("true") : QString("false") ) );
}


int CertificateViewerModel::columnCount( const QModelIndex & ) const
{
    return 2;
}

int CertificateViewerModel::rowCount( const QModelIndex & ) const
{
    return m_sigProperties.size();
}

QVariant CertificateViewerModel::data( const QModelIndex &index, int role ) const
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

QVariant CertificateViewerModel::headerData( int section, Qt::Orientation orientation, int role ) const
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
    : QDialog( parent ), m_sigInfo( sigInfo )
{
    setModal( true );
    setFixedSize( QSize( 450, 500 ));
    setWindowTitle( i18n("Signature Properties") );

    auto sigPropLabel = new QLabel( this );
    sigPropLabel->setText( i18n("Signature Properties:") );

    auto sigPropTree = new QTreeView( this );
    sigPropTree->setIndentation( 0 );
    m_sigPropModel = new CertificateViewerModel( m_sigInfo, this );
    sigPropTree->setModel( m_sigPropModel );
    connect( sigPropTree, &QTreeView::clicked, this, &CertificateViewer::updateText );

    m_sigPropText = new QTextEdit( this );
    m_sigPropText->setReadOnly( true );

    auto btnBox = new QDialogButtonBox( QDialogButtonBox::Close, this );
    btnBox->button( QDialogButtonBox::Close )->setDefault( true );
    connect( btnBox, &QDialogButtonBox::rejected, this, &SignaturePropertiesDialog::reject );

    auto mainLayout = new QVBoxLayout( this );
    mainLayout->addWidget( sigPropLabel );
    mainLayout->addWidget( sigPropTree );
    mainLayout->addWidget( m_sigPropText );
    mainLayout->addWidget( btnBox );
    setLayout( mainLayout );
}

void CertificateViewer::updateText( const QModelIndex &index )
{
    m_sigPropText->setText( m_sigPropModel->data( index, CertificateViewerModel::PropertyValueRole ).toString() );
}

SignaturePropertiesDialog::SignaturePropertiesDialog( Okular::Document *doc, Okular::FormFieldSignature *form, QWidget *parent )
    : QDialog( parent ), m_doc( doc ), m_signatureForm( form )
{
    setModal( true );
    setWindowTitle( i18n("Signature Properties") );

    m_signatureInfo = m_signatureForm->validate();

    auto mainLayout = new QVBoxLayout;
    // signature validation status
    auto sigStatusBox = new QGroupBox( i18n("Validity Status") );
    auto hBoxLayout = new QHBoxLayout;
    auto pixmapLabel = new QLabel;
    pixmapLabel->setPixmap( KIconLoader::global()->loadIcon( QLatin1String("application-certificate"),
                                                             KIconLoader::Desktop, KIconLoader::SizeSmallMedium ) );
    hBoxLayout->addWidget( pixmapLabel );

    auto sigStatusFormLayout = new QFormLayout;
    const Okular::SignatureInfo::SignatureStatus sigStatus = m_signatureInfo->signatureStatus();
    sigStatusFormLayout->addRow( i18n("Signature Validity:"), new QLabel( GuiUtils::getReadableSigState( sigStatus ) ) );
    QString modString;
    if ( sigStatus == Okular::SignatureInfo::SignatureValid )
    {
        if ( m_signatureInfo->signsTotalDocument() )
        {
            modString = i18n("The document has not been modified since it was signed.");
        }
        else
        {
            modString = i18n("The revision of the document that was covered by this signature has not been modified;\n"
                             "however there have been subsequent changes to the document.");
        }

    }
    else if ( sigStatus == Okular::SignatureInfo::SignatureDigestMismatch )
    {
        modString = i18n("The document has been modified in a way not permitted by a previous signer.");
    }
    else
    {
        modString = i18n("The document integrity verification could not be completed.");
    }
    sigStatusFormLayout->addRow( i18n("Document Modifications:"), new QLabel( modString ) );
    hBoxLayout->addLayout( sigStatusFormLayout );
    sigStatusBox->setLayout( hBoxLayout );
    mainLayout->addWidget( sigStatusBox );

    // additional information
    auto extraInfoBox = new QGroupBox( i18n("Additional Information") );

    auto extraInfoFormLayout = new QFormLayout;
    extraInfoFormLayout->addRow( i18n("Signed By:"), new QLabel( m_signatureInfo->signerName() ) );
    extraInfoFormLayout->addRow( i18n("Signing Time:"), new QLabel( m_signatureInfo->signingTime().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    auto getValidString = [=]( const QString &str ) -> QString {
        return !str.isEmpty() ?  str : i18n("Not Available");
    };
    // optional info
    extraInfoFormLayout->addRow( i18n("Reason:"), new QLabel( getValidString( m_signatureInfo->reason() ) ) );
    extraInfoFormLayout->addRow( i18n("Location:"), new QLabel( getValidString( m_signatureInfo->location() ) ) );
    extraInfoBox->setLayout( extraInfoFormLayout );
    mainLayout->addWidget( extraInfoBox );

    // document version
    auto revisionBox = new QGroupBox( i18n("Document Version") );
    auto revisionLayout = new QHBoxLayout;
    QVector<Okular::FormFieldSignature*> signatureFormFields = GuiUtils::getSignatureFormFields( m_doc );
    revisionLayout->addWidget( new QLabel( i18nc("Document Revision <current> of <total>", "Document Revision %1 of %2",
                                                signatureFormFields.indexOf( m_signatureForm ) + 1, signatureFormFields.size() ) ) );
    revisionLayout->addStretch();
    auto revisionBtn = new QPushButton( i18n( "View Signed Version...") );
    revisionBtn->setEnabled( !m_signatureInfo->signsTotalDocument() );
    connect( revisionBtn, &QPushButton::clicked, this, &SignaturePropertiesDialog::viewSignedVersion );
    revisionLayout->addWidget( revisionBtn );
    revisionBox->setLayout( revisionLayout );
    mainLayout->addWidget( revisionBox );

    // button box
    auto btnBox = new QDialogButtonBox( QDialogButtonBox::Close, this );
    auto certPropBtn = new QPushButton( i18n( "Vew Certificate..."), this );
    certPropBtn->setVisible(m_signatureInfo->certificateInfo()->isNull());
    btnBox->button( QDialogButtonBox::Close )->setDefault( true );
    btnBox->addButton( certPropBtn, QDialogButtonBox::ActionRole );
    connect( btnBox, &QDialogButtonBox::rejected, this, &SignaturePropertiesDialog::reject );
    connect( certPropBtn, &QPushButton::clicked, this, &SignaturePropertiesDialog::viewCertificateProperties );
    mainLayout->addWidget( btnBox );

    setLayout( mainLayout );
    resize( mainLayout->sizeHint() );
}

void SignaturePropertiesDialog::viewCertificateProperties()
{
    CertificateViewer sigPropDlg( m_signatureInfo, this );
    sigPropDlg.exec();
}

void SignaturePropertiesDialog::viewSignedVersion()
{
    QByteArray data;
    m_doc->requestSignedRevisionData( m_signatureInfo, &data );
    const QString tmpDir = QStandardPaths::writableLocation( QStandardPaths::TempLocation );
    QTemporaryFile tf( tmpDir + "/revision_XXXXXX.pdf" );
    if ( !tf.open() )
    {
        KMessageBox::error( this, i18n("Could not open revision for preview" ) );
        return;
    }
    tf.write(data);
    RevisionViewer view( tf.fileName(), this);
    view.exec();
    tf.close();
}

RevisionViewer::RevisionViewer( const QString &filename, QWidget *parent )
    : FilePrinterPreview( filename, parent )
{
    setWindowTitle( i18n("Revision Preview") );
}

RevisionViewer::~RevisionViewer()
{
}

#include "moc_signaturewidgets.cpp"

