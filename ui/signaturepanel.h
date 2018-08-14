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

#include <QHash>
#include <QWidget>

#include "core/observer.h"

namespace Okular {
class Document;
class FormFieldSignature;
}

class QTreeView;
class SignatureModel;
class PageView;

class SignaturePanelPrivate;

class SignaturePanel : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
    public:
        SignaturePanel( QWidget *parent, Okular::Document *document );
        ~SignaturePanel();

        // inherited from DocumentObserver
        void notifySetup( const QVector<Okular::Page *> &pages, int setupFlags ) override;
        // inherited from QWidget
        bool event( QEvent *event ) override;

        void setPageView( PageView *pv );

    Q_SIGNALS:
        void signaturePanelVisible( bool visible );

    private Q_SLOTS:
        void activated( const QModelIndex& );
        void slotShowContextMenu();
        void slotViewRevision();
        void slotViewProperties();

    private:
        Q_DECLARE_PRIVATE( SignaturePanel )
        QScopedPointer<SignaturePanelPrivate> d_ptr;
};

#endif
