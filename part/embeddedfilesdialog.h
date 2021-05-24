/*
    SPDX-FileCopyrightText: 2006 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _EMBEDDEDFILESDIALOG_H_
#define _EMBEDDEDFILESDIALOG_H_

#include <QDialog>

class QTreeWidget;
class QPushButton;
class QTemporaryFile;
class QTreeWidgetItem;

namespace Okular
{
class Document;
class EmbeddedFile;
}

class EmbeddedFilesDialog : public QDialog
{
    Q_OBJECT
public:
    EmbeddedFilesDialog(QWidget *parent, const Okular::Document *document);

private Q_SLOTS:
    void saveFileFromButton();
    void attachViewContextMenu();
    void updateSaveButton();
    void viewFileFromButton();
    void viewFileItem(QTreeWidgetItem *item, int column);

private:
    void saveFile(Okular::EmbeddedFile *);
    void viewFile(Okular::EmbeddedFile *);

    QTreeWidget *m_tw;

    QPushButton *mUser1Button;
    QPushButton *mUser2Button;
    QList<QSharedPointer<QTemporaryFile>> m_openedFiles;
};

#endif
