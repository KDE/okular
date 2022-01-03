/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SIGNATUREPANEL_H
#define OKULAR_SIGNATUREPANEL_H

#include <QHash>
#include <QWidget>

#include "core/observer.h"

namespace Okular
{
class Document;
}

class PageView;

class SignaturePanelPrivate;

class SignaturePanel : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
public:
    SignaturePanel(Okular::Document *document, QWidget *parent);
    ~SignaturePanel() override;

    // inherited from DocumentObserver
    void notifySetup(const QVector<Okular::Page *> &pages, int setupFlags) override;

    void setPageView(PageView *pv);

Q_SIGNALS:
    void documentHasSignatures(bool hasSignatures);

private Q_SLOTS:
    void activated(const QModelIndex &);
    void slotShowContextMenu();
    void slotViewProperties();
    void signUnsignedSignature();

private:
    Q_DECLARE_PRIVATE(SignaturePanel)
    QScopedPointer<SignaturePanelPrivate> d_ptr;
};

#endif
