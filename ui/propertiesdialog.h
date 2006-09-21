/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
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

namespace Okular {
class Document;
}

class PropertiesDialog : public KPageDialog
{
  public:
  	PropertiesDialog( QWidget *parent, Okular::Document *doc );
};

class LocalFontInfoStruct;

class FontsListModel
  : public QAbstractTableModel
{
  Q_OBJECT

  public:
    FontsListModel( QObject * parent = 0 );
    virtual ~FontsListModel();

    void addFont( const QString &name, const QString &type, const QString &embedded, const QString &file );

    // reimplementations from QAbstractTableModel
    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;

  private:
    QList<LocalFontInfoStruct*> m_fonts;
};

#endif
