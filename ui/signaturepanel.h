/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SIGNATUREPANEL_H
#define OKULAR_SIGNATUREPANEL_H

#include <QTreeView>

#include "core/observer.h"

namespace Okular {
class Document;
}

class SignatureModel;

class TreeView1 : public QTreeView
{
  Q_OBJECT

  public:
    TreeView1( Okular::Document *document, QWidget *parent = Q_NULLPTR );
  protected:
    void paintEvent( QPaintEvent *event ) override;

  private:
    Okular::Document *m_document;
};

class SignaturePanel : public QWidget
{
    Q_OBJECT
    public:
        SignaturePanel( QWidget * parent, Okular::Document * document );
        ~SignaturePanel();

    private Q_SLOTS:
        void activated( const QModelIndex& );

    private:
         Okular::Document *m_document;
         TreeView1 *m_view;
         SignatureModel *m_model;
};

#endif
