/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "signaturepropertiesdialog.h"

#include <KLocalizedString>
#include <KColumnResizer>

#include <QVector>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QDialogButtonBox>

#include "core/form.h"
#include "core/document.h"

#include "signatureguiutils.h"
#include "revisionviewer.h"
#include "certificateviewer.h"

static QString getValidDisplayString( const QString &str )
{
    return !str.isEmpty() ?  str : i18n("Not Available");
}

SignaturePropertiesDialog::SignaturePropertiesDialog( Okular::Document *doc, Okular::FormFieldSignature *form, QWidget *parent )
    : QDialog( parent ), m_doc( doc ), m_signatureForm( form )
{
    setModal( true );
    setWindowTitle( i18n("Signature Properties") );

    m_signatureInfo = m_signatureForm->validate();
    const Okular::SignatureInfo::SignatureStatus signatureStatus = m_signatureInfo->signatureStatus();
    const QString readableSignatureStatus = SignatureGuiUtils::getReadableSignatureStatus( signatureStatus );
    const QString signerName = getValidDisplayString( m_signatureInfo->signerName() );
    const QString signingTime = getValidDisplayString( m_signatureInfo->signingTime().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) );
    const QString signingLocation = getValidDisplayString( m_signatureInfo->location() );
    const QString signingReason = getValidDisplayString( m_signatureInfo->reason() );

    // signature validation status
    QString modificationSummary;
    if ( signatureStatus == Okular::SignatureInfo::SignatureValid )
    {
        if ( m_signatureInfo->signsTotalDocument() )
        {
            modificationSummary = i18n("The document has not been modified since it was signed.");
        }
        else
        {
            modificationSummary = i18n("The revision of the document that was covered by this signature has not been modified;\n"
                                       "however there have been subsequent changes to the document.");
        }
    }
    else if ( signatureStatus == Okular::SignatureInfo::SignatureDigestMismatch )
    {
        modificationSummary = i18n("The document has been modified in a way not permitted by a previous signer.");
    }
    else
    {
        modificationSummary = i18n("The document integrity verification could not be completed.");
    }

    auto signatureStatusBox = new QGroupBox( i18n("Validity Status") );
    auto signatureStatusFormLayout = new QFormLayout( signatureStatusBox );
    signatureStatusFormLayout->setLabelAlignment( Qt::AlignLeft );
    signatureStatusFormLayout->addRow( i18n("Signature Validity:"), new QLabel( readableSignatureStatus ) );
    signatureStatusFormLayout->addRow( i18n("Document Modifications:"), new QLabel( modificationSummary ) );

    // additional information
    auto extraInfoBox = new QGroupBox( i18n("Additional Information") );
    auto extraInfoFormLayout = new QFormLayout( extraInfoBox );
    extraInfoFormLayout->setLabelAlignment( Qt::AlignLeft );
    extraInfoFormLayout->addRow( i18n("Signed By:"), new QLabel( signerName ) );
    extraInfoFormLayout->addRow( i18n("Signing Time:"), new QLabel( signingTime  ) );
    extraInfoFormLayout->addRow( i18n("Reason:"), new QLabel( signingReason ) );
    extraInfoFormLayout->addRow( i18n("Location:"), new QLabel( signingLocation ) );

    // keep width of column 1 same
    auto resizer = new KColumnResizer( this );
    resizer->addWidgetsFromLayout( signatureStatusFormLayout->layout(), 0 );
    resizer->addWidgetsFromLayout( extraInfoFormLayout->layout(), 0 );

    // document revision info
    auto revisionBox = new QGroupBox( i18n("Document Version") );
    auto revisionLayout = new QHBoxLayout( revisionBox );
    QVector<Okular::FormFieldSignature*> signatureFormFields = SignatureGuiUtils::getSignatureFormFields( m_doc, true, -1 );
    revisionLayout->addWidget( new QLabel( i18nc("Document Revision <current> of <total>", "Document Revision %1 of %2",
                                                signatureFormFields.indexOf( m_signatureForm ) + 1, signatureFormFields.size() ) ) );
    revisionLayout->addStretch();
    auto revisionBtn = new QPushButton( i18n("View Signed Version...") );
    revisionBtn->setEnabled( !m_signatureInfo->signsTotalDocument() );
    connect( revisionBtn, &QPushButton::clicked, this, &SignaturePropertiesDialog::viewSignedVersion );
    revisionLayout->addWidget( revisionBtn );

    // button box
    auto btnBox = new QDialogButtonBox( QDialogButtonBox::Close, this );
    auto certPropBtn = new QPushButton( i18n("Vew Certificate...") );
    certPropBtn->setEnabled( !m_signatureInfo->certificateInfo()->isNull() );
    btnBox->addButton( certPropBtn, QDialogButtonBox::ActionRole );
    connect( btnBox, &QDialogButtonBox::rejected, this, &SignaturePropertiesDialog::reject );
    connect( certPropBtn, &QPushButton::clicked, this, &SignaturePropertiesDialog::viewCertificateProperties );

    auto mainLayout = new QVBoxLayout( this );
    mainLayout->addWidget( signatureStatusBox );
    mainLayout->addWidget( extraInfoBox );
    mainLayout->addWidget( revisionBox );
    mainLayout->addWidget( btnBox );

    resize( mainLayout->sizeHint() );
}

void SignaturePropertiesDialog::viewCertificateProperties()
{
    CertificateViewer certViewer( m_signatureInfo->certificateInfo(), this );
    certViewer.exec();
}

void SignaturePropertiesDialog::viewSignedVersion()
{
    QByteArray data;
    m_doc->requestSignedRevisionData( m_signatureInfo, &data );
    RevisionViewer revViewer( data, this );
    revViewer.viewRevision();
}

#include "moc_signaturepropertiesdialog.cpp"
