/*
    SPDX-FileCopyrightText: 2006 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "embeddedfilesdialog.h"

#include <QAction>
#include <QCursor>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QTemporaryFile>
#include <QTreeWidget>

#include <KConfigGroup>
#include <KFormat>
#include <KLocalizedString>
#include <KRun>
#include <KStandardGuiItem>
#include <QDialogButtonBox>
#include <QIcon>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/document.h"
#include "gui/guiutils.h"

Q_DECLARE_METATYPE(Okular::EmbeddedFile *)

static const int EmbeddedFileRole = Qt::UserRole + 100;

static QString dateToString(const QDateTime &date)
{
    return date.isValid() ? QLocale().toString(date, QLocale::LongFormat) : i18nc("Unknown date", "Unknown");
}

EmbeddedFilesDialog::EmbeddedFilesDialog(QWidget *parent, const Okular::Document *document)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Embedded Files"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mUser1Button = new QPushButton;
    buttonBox->addButton(mUser1Button, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
    KGuiItem::assign(mUser1Button, KStandardGuiItem::save());
    mUser1Button->setEnabled(false);

    mUser2Button = new QPushButton;
    buttonBox->addButton(mUser2Button, QDialogButtonBox::ActionRole);
    KGuiItem::assign(mUser2Button, KGuiItem(i18nc("@action:button", "View"), QStringLiteral("document-open")));
    mUser2Button->setEnabled(false);

    m_tw = new QTreeWidget(this);
    mainLayout->addWidget(m_tw);
    mainLayout->addWidget(buttonBox);

    QStringList header;
    header.append(i18nc("@title:column", "Name"));
    header.append(i18nc("@title:column", "Description"));
    header.append(i18nc("@title:column", "Size"));
    header.append(i18nc("@title:column", "Created"));
    header.append(i18nc("@title:column", "Modified"));
    m_tw->setHeaderLabels(header);
    m_tw->setRootIsDecorated(false);
    m_tw->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tw->setContextMenuPolicy(Qt::CustomContextMenu);

    // embeddedFiles() returns a const QList
    for (Okular::EmbeddedFile *ef : *document->embeddedFiles()) {
        QTreeWidgetItem *twi = new QTreeWidgetItem();
        twi->setText(0, ef->name());
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(ef->name(), QMimeDatabase::MatchExtension);
        if (mime.isValid()) {
            twi->setIcon(0, QIcon::fromTheme(mime.iconName()));
        }
        twi->setText(1, ef->description());
        twi->setText(2, ef->size() <= 0 ? i18nc("Not available size", "N/A") : KFormat().formatByteSize(ef->size()));
        twi->setText(3, dateToString(ef->creationDate()));
        twi->setText(4, dateToString(ef->modificationDate()));
        twi->setData(0, EmbeddedFileRole, QVariant::fromValue(ef));
        m_tw->addTopLevelItem(twi);
    }
    // Having filled the columns, it is nice to resize them to be able to read the contents
    for (int lv = 0; lv < m_tw->columnCount(); ++lv) {
        m_tw->resizeColumnToContents(lv);
    }
    // This is a bit dubious, but I'm not seeing a nice way to say "expand to fit contents"
    m_tw->setMinimumWidth(640);
    m_tw->updateGeometry();

    connect(mUser1Button, &QPushButton::clicked, this, &EmbeddedFilesDialog::saveFileFromButton);
    connect(mUser2Button, &QPushButton::clicked, this, &EmbeddedFilesDialog::viewFileFromButton);
    connect(m_tw, &QWidget::customContextMenuRequested, this, &EmbeddedFilesDialog::attachViewContextMenu);
    connect(m_tw, &QTreeWidget::itemSelectionChanged, this, &EmbeddedFilesDialog::updateSaveButton);
    connect(m_tw, &QTreeWidget::itemDoubleClicked, this, &EmbeddedFilesDialog::viewFileItem);
}

void EmbeddedFilesDialog::updateSaveButton()
{
    bool enable = (m_tw->selectedItems().count() > 0);
    mUser1Button->setEnabled(enable);
    mUser2Button->setEnabled(enable);
}

void EmbeddedFilesDialog::saveFileFromButton()
{
    const QList<QTreeWidgetItem *> selected = m_tw->selectedItems();
    for (const QTreeWidgetItem *twi : selected) {
        Okular::EmbeddedFile *ef = qvariant_cast<Okular::EmbeddedFile *>(twi->data(0, EmbeddedFileRole));
        saveFile(ef);
    }
}

void EmbeddedFilesDialog::viewFileFromButton()
{
    const QList<QTreeWidgetItem *> selected = m_tw->selectedItems();
    for (QTreeWidgetItem *twi : selected) {
        Okular::EmbeddedFile *ef = qvariant_cast<Okular::EmbeddedFile *>(twi->data(0, EmbeddedFileRole));
        viewFile(ef);
    }
}

void EmbeddedFilesDialog::viewFileItem(QTreeWidgetItem *item, int /*column*/)
{
    Okular::EmbeddedFile *ef = qvariant_cast<Okular::EmbeddedFile *>(item->data(0, EmbeddedFileRole));
    viewFile(ef);
}

void EmbeddedFilesDialog::attachViewContextMenu()
{
    QList<QTreeWidgetItem *> selected = m_tw->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    if (selected.size() > 1) {
        return;
    }

    QMenu menu(this);
    QAction *saveAsAct = menu.addAction(QIcon::fromTheme(QStringLiteral("document-save-as")), i18nc("@action:inmenu", "&Save As..."));
    QAction *viewAct = menu.addAction(QIcon::fromTheme(QStringLiteral("document-open")), i18nc("@action:inmenu", "&View..."));

    QAction *act = menu.exec(QCursor::pos());
    if (!act) {
        return;
    }

    Okular::EmbeddedFile *ef = qvariant_cast<Okular::EmbeddedFile *>(selected.at(0)->data(0, EmbeddedFileRole));
    if (act == saveAsAct) {
        saveFile(ef);
    } else if (act == viewAct) {
        viewFile(ef);
    }
}

void EmbeddedFilesDialog::viewFile(Okular::EmbeddedFile *ef)
{
    // get name and extension
    QFileInfo fileInfo(ef->name());

    // save in temporary directory with a unique name resembling the attachment name,
    // using QTemporaryFile's XXXXXX placeholder
    QTemporaryFile *tmpFile =
        new QTemporaryFile(QDir::tempPath() + QLatin1Char('/') + fileInfo.baseName() + QStringLiteral(".XXXXXX") + (fileInfo.completeSuffix().isEmpty() ? QLatin1String("") : QString(QLatin1Char('.') + fileInfo.completeSuffix())));
    GuiUtils::writeEmbeddedFile(ef, this, *tmpFile);

    // set readonly to prevent the viewer application from modifying it
    tmpFile->setPermissions(QFile::ReadOwner);

    // keep temporary file alive while the dialog is open
    m_openedFiles.push_back(QSharedPointer<QTemporaryFile>(tmpFile));

    // view the temporary file with the default application
    new KRun(QUrl::fromLocalFile(tmpFile->fileName()), this);
}

void EmbeddedFilesDialog::saveFile(Okular::EmbeddedFile *ef)
{
    GuiUtils::saveEmbeddedFile(ef, this);
}

#include "moc_embeddedfilesdialog.cpp"
