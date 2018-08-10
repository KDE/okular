/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "revisionviewer.h"

#include <KMessageBox>
#include <KLocalizedString>

#include <QLayout>
#include <QMimeType>
#include <QPushButton>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDialogButtonBox>

#include "fileprinterpreview.h"

static void clearLayout( QLayout* layout )
{
    while ( QLayoutItem* item = layout->takeAt(0) )
    {
        delete item->widget();
        delete item;
    }
}

class RevisionPreview : public Okular::FilePrinterPreview
{
    Q_OBJECT

    public:
        explicit RevisionPreview( const QString &revisionFile, QWidget *parent = nullptr );

    private Q_SLOTS:
        void doSave();

    private:
        QString m_filename;
};

RevisionPreview::RevisionPreview( const QString &revisionFile, QWidget *parent )
    : FilePrinterPreview( revisionFile, parent ), m_filename( revisionFile )
{
    setWindowTitle( i18n("Revision Preview") );

    auto mainLayout = static_cast<QVBoxLayout*>( layout() );
    clearLayout( mainLayout );
    auto btnBox = new QDialogButtonBox( QDialogButtonBox::Close, this );
    auto saveBtn = new QPushButton( i18n( "Save As"), this );
    btnBox->button( QDialogButtonBox::Close )->setDefault( true );
    btnBox->addButton( saveBtn, QDialogButtonBox::ActionRole );
    connect( btnBox, &QDialogButtonBox::rejected, this, &RevisionPreview::reject );
    connect( saveBtn, &QPushButton::clicked, this, &RevisionPreview::doSave );
    mainLayout->addWidget( btnBox );
}

void RevisionPreview::doSave()
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile( m_filename );
    const QString caption = i18n( "Where do you want to save this revision?" );
    const QString path = QFileDialog::getSaveFileName( this, caption, QStringLiteral("Revision"), mime.filterString() );
    if ( !path.isEmpty() && !QFile::copy( m_filename, path ) )
    {
        KMessageBox::error( this, i18n("Could not save file %1.", path) );
        return;
    }
}

RevisionViewer::RevisionViewer( const QByteArray &revisionData, QWidget *parent )
    : QObject( parent ), m_parent( parent ), m_revisionData( revisionData )
{
}

void RevisionViewer::viewRevision()
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForData( m_revisionData );
    const QString tempDir = QStandardPaths::writableLocation( QStandardPaths::TempLocation );
    QTemporaryFile tf( tempDir + QStringLiteral("/okular_revision_XXXXXX.%1").arg( mime.suffixes().first() ));
    if ( !tf.open() )
    {
        KMessageBox::error( m_parent, i18n("Could not view revision.") );
        return;
    }
    tf.write( m_revisionData );
    RevisionPreview previewdlg( tf.fileName(), m_parent );
    previewdlg.exec();
}

#include "revisionviewer.moc"
