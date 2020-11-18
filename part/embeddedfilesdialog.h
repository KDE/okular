/***************************************************************************
 *   Copyright (C) 2006 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
