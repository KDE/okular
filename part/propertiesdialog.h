/*
    SPDX-FileCopyrightText: 2004 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _PROPERTIESDIALOG_H_
#define _PROPERTIESDIALOG_H_

#include <QAbstractItemModel>
#include <QList>

#include <KPageDialog>

#include "core/fontinfo.h"

class QLabel;
class QProgressBar;
class FontsListModel;
class PageSizesModel;

namespace Okular
{
class Document;
}

class PropertiesDialog : public KPageDialog
{
    Q_OBJECT

public:
    PropertiesDialog(QWidget *parent, Okular::Document *doc);
    ~PropertiesDialog() override;

private Q_SLOTS:
    void pageChanged(KPageWidgetItem *, KPageWidgetItem *);
    void slotFontReadingProgress(int page);
    void slotFontReadingEnded();
    void reallyStartFontReading();
    void showFontsMenu(const QPoint pos);

private:
    Okular::Document *m_document;
    KPageWidgetItem *m_fontPage;
    FontsListModel *m_fontModel;
    PageSizesModel *m_pageSizesModel;
    QLabel *m_fontInfo;
    QProgressBar *m_fontProgressBar;
    bool m_fontScanStarted;
};

class FontsListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit FontsListModel(QObject *parent = nullptr);
    ~FontsListModel() override;

    // reimplementations from QAbstractTableModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

public Q_SLOTS:
    void addFont(const Okular::FontInfo &fi);

private:
    QList<Okular::FontInfo> m_fonts;
};

class PageSizesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit PageSizesModel(QObject *parent = nullptr, Okular::Document *doc = nullptr);
    ~PageSizesModel() override;

    // reimplementations from QAbstractTableModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    Okular::Document *m_document;
};

#endif

/* kate: replace-tabs on; indent-width 4; */
