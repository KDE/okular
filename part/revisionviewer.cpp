/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "revisionviewer.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLayout>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryFile>

#include "fileprinterpreview.h"

class RevisionPreview : public Okular::FilePrinterPreview
{
    Q_OBJECT

public:
    explicit RevisionPreview(const QString &revisionFile, QWidget *parent = nullptr);

private Q_SLOTS:
    void doSave();

private:
    QString m_filename;
};

RevisionPreview::RevisionPreview(const QString &revisionFile, QWidget *parent)
    : FilePrinterPreview(revisionFile, parent)
    , m_filename(revisionFile)
{
    setWindowTitle(i18n("Revision Preview"));

    QDialogButtonBox *btnBox = findChild<QDialogButtonBox *>();
    auto saveBtn = new QPushButton(i18n("Save As"), this);
    btnBox->addButton(saveBtn, QDialogButtonBox::ActionRole);
    connect(saveBtn, &QPushButton::clicked, this, &RevisionPreview::doSave);
}

void RevisionPreview::doSave()
{
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(m_filename);
    const QString caption = i18n("Where do you want to save this revision?");
    const QString path = QFileDialog::getSaveFileName(this, caption, QStringLiteral("Revision"), mime.filterString());
    if (!path.isEmpty() && !QFile::copy(m_filename, path)) {
        KMessageBox::error(this, i18n("Could not save file %1.", path));
        return;
    }
}

RevisionViewer::RevisionViewer(const QByteArray &revisionData, QWidget *parent)
    : QObject(parent)
    , m_parent(parent)
    , m_revisionData(revisionData)
{
}

void RevisionViewer::viewRevision()
{
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForData(m_revisionData);
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QTemporaryFile tf(tempDir + QStringLiteral("/okular_revision_XXXXXX.%1").arg(mime.suffixes().constFirst()));
    if (!tf.open()) {
        KMessageBox::error(m_parent, i18n("Could not view revision."));
        return;
    }
    tf.write(m_revisionData);
    RevisionPreview previewdlg(tf.fileName(), m_parent);
    previewdlg.exec();
}

#include "revisionviewer.moc"
