/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _PROPERTIESDIALOG_H_
#define _PROPERTIESDIALOG_H_

#include <qabstractitemmodel.h>
#include <qlist.h>

#include <kpagedialog.h>

#include "core/fontinfo.h"

class QLabel;
class QProgressBar;
class FontsListModel;
class KSqueezedTextLabel;

namespace Okular {
class Document;
}

class PropertiesDialog : public KPageDialog
{
    Q_OBJECT

  public:
        PropertiesDialog( QWidget *parent, Okular::Document *doc );
        virtual ~PropertiesDialog();

    private Q_SLOTS:
        void pageChanged( KPageWidgetItem *, KPageWidgetItem * );
        void slotFontReadingProgress( int page );
        void slotFontReadingEnded();
        void reallyStartFontReading();
        void showFontsMenu(const QPoint &pos);

    private:
        Okular::Document * m_document;
        KPageWidgetItem * m_fontPage;
        FontsListModel * m_fontModel;
        QLabel * m_fontInfo;
        QProgressBar * m_fontProgressBar;
        bool m_fontScanStarted;
};

class FontsListModel
  : public QAbstractTableModel
{
  Q_OBJECT

  public:
    FontsListModel( QObject * parent = Q_NULLPTR );
    virtual ~FontsListModel();

    // reimplementations from QAbstractTableModel
    int columnCount( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;
    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
    int rowCount( const QModelIndex &parent = QModelIndex() ) const Q_DECL_OVERRIDE;

  public Q_SLOTS:
    void addFont( const Okular::FontInfo &fi );

  private:
    QList<Okular::FontInfo> m_fonts;
};

#endif

/* kate: replace-tabs on; indent-width 4; */
