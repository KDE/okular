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
    m_sigProperties.append( qMakePair( i18n("Subject Name"), sigInfo->subjectName() ) );
    m_sigProperties.append( qMakePair( i18n("Subject Distinguished Name"), sigInfo->subjectDN() ) );
    m_sigProperties.append( qMakePair( i18n("Signing Time"), sigInfo->signingTime().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_sigProperties.append( qMakePair( i18n("Hash Algorithm"), GuiUtils::getReadableHashAlgorithm( sigInfo->hashAlgorithm() ) ) );
    m_sigProperties.append( qMakePair( i18n("Signature Status"), GuiUtils::getReadableSigState( sigInfo->signatureStatus() ) ) );
    m_sigProperties.append( qMakePair( i18n("Certificate Status"), GuiUtils::getReadableCertState( sigInfo->certificateStatus() ) ) );
    m_sigProperties.append( qMakePair( i18n("Signature Data"), QString::fromUtf8( sigInfo->signature().toHex(' ') ) ) );
    m_sigProperties.append( qMakePair( i18n("Location"), QString( sigInfo->location() ) ) );
    m_sigProperties.append( qMakePair( i18n("Reason"), QString( sigInfo->reason() ) ) );
    m_sigProperties.append( qMakePair( QStringLiteral("----------"), QString("------Certificate Properties--------") ) );

    Okular::CertificateInfo *certInfo = sigInfo->certificateInfo();
    m_sigProperties.append( qMakePair( i18n("Version"), certInfo->version() ) );
    m_sigProperties.append( qMakePair( i18n("Issuer Name"), certInfo->issuerName() ) );
    m_sigProperties.append( qMakePair( i18n("Issuer Distinguished Name"), certInfo->issuerDN() ) );
    m_sigProperties.append( qMakePair( i18n("Serial Number"), certInfo->serialNumber() ) );
    m_sigProperties.append( qMakePair( i18n("Validity Start"), certInfo->validityStart().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_sigProperties.append( qMakePair( i18n("Validity End"), certInfo->validityEnd().toString( QStringLiteral("MMM dd yyyy hh:mm:ss") ) ) );
    m_sigProperties.append( qMakePair( i18n("Public Key"), certInfo->publicKey() ) );
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
    extraInfoFormLayout->addRow( i18n("Signed By:"), new QLabel( m_signatureInfo->subjectName() ) );
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

TreeView1::TreeView1(Okular::Document *document, QWidget *parent)
    : QTreeView( parent ), m_document( document )
{
}

void TreeView1::paintEvent( QPaintEvent *event )
{
  bool hasSignatures = false;
  for ( uint i = 0; i < m_document->pages(); i++ )
  {
      foreach (Okular::FormField *f, m_document->page( i )->formFields() )
      {
          if ( f->type() == Okular::FormField::FormSignature )
          {
              hasSignatures = true;
              break;
          }
      }
  }

  if ( !hasSignatures )
  {
      QPainter p( viewport() );
      p.setRenderHint( QPainter::Antialiasing, true );
      p.setClipRect( event->rect() );

      QTextDocument document;
      document.setHtml( i18n( "<div align=center>"
                            "This document does not contain any digital signature."
                            "</div>" ) );
      document.setTextWidth( width() - 50 );

      const uint w = document.size().width() + 20;
      const uint h = document.size().height() + 20;
      p.setBrush( palette().background() );
      p.translate( 0.5, 0.5 );
      p.drawRoundRect( 15, 15, w, h, (8*200)/w, (8*200)/h );

      p.translate( 20, 20 );
      document.drawContents( &p );

  }
  else
  {
      QTreeView::paintEvent( event );
  }
}

SignaturePanel::SignaturePanel( QWidget *parent, Okular::Document *document )
    : QWidget( parent ), m_document( document )
{
    auto vLayout = new QVBoxLayout( this );
    vLayout->setMargin( 0 );
    vLayout->setSpacing( 6 );

    m_view = new TreeView1( m_document, this );
    m_view->setAlternatingRowColors( true );
    m_view->setSelectionMode( QAbstractItemView::ExtendedSelection );
    m_view->header()->hide();

    m_model = new SignatureModel( m_document, this );

    m_view->setModel( m_model );
    connect(m_view, &TreeView1::activated, this, &SignaturePanel::activated);

    vLayout->addWidget( m_view );
}

void SignaturePanel::activated( const QModelIndex &index )
{
    int formId = m_model->data( index, SignatureModel::FormRole ).toInt();
    auto formFields = GuiUtils::getSignatureFormFields( m_document );
    Okular::FormFieldSignature *sf;
    foreach( auto f, formFields )
    {
        if ( f->id() == formId )
        {
            sf = f;
            break;
        }
    }
    if ( !sf )
      return;

    Okular::NormalizedRect nr = sf->rect();
    Okular::DocumentViewport vp;
    vp.pageNumber = m_model->data( index, SignatureModel::PageRole ).toInt();
    vp.rePos.enabled = true;
    vp.rePos.pos = Okular::DocumentViewport::Center;
    vp.rePos.normalizedX = ( nr.right + nr.left ) / 2.0;
    vp.rePos.normalizedY = ( nr.bottom + nr.top ) / 2.0;
    m_document->setViewport( vp, nullptr, true );
}

SignaturePanel::~SignaturePanel()
{
    m_document->removeObserver( this );
}

#include "moc_signaturewidgets.cpp"

