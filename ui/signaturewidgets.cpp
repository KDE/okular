/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "signaturewidgets.h"
#include "certificateviewer.h"

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

