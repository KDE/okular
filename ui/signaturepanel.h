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

#include <QWidget>

#include "core/observer.h"

namespace Okular {
class Document;
}

class QTreeView;
class SignatureModel;

class SignaturePanel : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
    public:
        SignaturePanel( QWidget * parent, Okular::Document * document );
        ~SignaturePanel();

    private Q_SLOTS:
        void activated( const QModelIndex& );

    private:
         Okular::Document *m_document;
         QTreeView *m_view;
         SignatureModel *m_model;
};

#endif
