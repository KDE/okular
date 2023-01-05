/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signaturepanel.h"

#include "gui/signaturemodel.h"
#include "pageview.h"
#include "revisionviewer.h"
#include "signaturepartutils.h"
#include "signaturepropertiesdialog.h"

#include <kwidgetsaddons_version.h>

#include <KLocalizedString>
#include <KTitleWidget>

#include <QApplication>
#include <QHeaderView>
#include <QMenu>
#include <QTreeView>
#include <QVBoxLayout>

#include "core/document.h"
#include "core/form.h"

class SignaturePanelPrivate
{
public:
    Okular::Document *m_document;
    const Okular::FormFieldSignature *m_currentForm;
    QTreeView *m_view;
    SignatureModel *m_model;
    PageView *m_pageView;
};

SignaturePanel::SignaturePanel(Okular::Document *document, QWidget *parent)
    : QWidget(parent)
    , d_ptr(new SignaturePanelPrivate)
{
    Q_D(SignaturePanel);

    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setLevel(4);
    titleWidget->setText(i18n("Signatures"));

    d->m_view = new QTreeView(this);
    d->m_view->setAlternatingRowColors(true);
    d->m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    d->m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    d->m_view->header()->hide();

    d->m_document = document;
    d->m_model = new SignatureModel(d->m_document, this);

    d->m_view->setModel(d->m_model);
    connect(d->m_view->selectionModel(), &QItemSelectionModel::currentChanged, this, &SignaturePanel::activated);
    connect(d->m_view, &QTreeView::customContextMenuRequested, this, &SignaturePanel::slotShowContextMenu);

    auto vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(6);

    vLayout->addWidget(titleWidget);
    vLayout->setAlignment(titleWidget, Qt::AlignHCenter);
    vLayout->addWidget(d->m_view);
}

void SignaturePanel::activated(const QModelIndex &index)
{
    Q_D(SignaturePanel);
    d->m_currentForm = d->m_model->data(index, SignatureModel::FormRole).value<const Okular::FormFieldSignature *>();
    if (!d->m_currentForm) {
        return;
    }
    const Okular::NormalizedRect nr = d->m_currentForm->rect();
    Okular::DocumentViewport vp;
    vp.pageNumber = d->m_model->data(index, SignatureModel::PageRole).toInt();
    vp.rePos.enabled = true;
    vp.rePos.pos = Okular::DocumentViewport::Center;
    vp.rePos.normalizedX = (nr.right + nr.left) / 2.0;
    vp.rePos.normalizedY = (nr.bottom + nr.top) / 2.0;
    d->m_document->setViewport(vp, nullptr);
    d->m_pageView->highlightSignatureFormWidget(d->m_currentForm);
}

void SignaturePanel::slotShowContextMenu()
{
    Q_D(SignaturePanel);
    if (!d->m_currentForm) {
        return;
    }

    QMenu *menu = new QMenu(this);
    if (d->m_currentForm->signatureType() == Okular::FormFieldSignature::UnsignedSignature) {
        QAction *signAction = new QAction(i18n("&Sign..."), menu);
        connect(signAction, &QAction::triggered, this, &SignaturePanel::signUnsignedSignature);
        menu->addAction(signAction);
    } else {
        QAction *sigProp = new QAction(i18n("Properties"), menu);
        connect(sigProp, &QAction::triggered, this, &SignaturePanel::slotViewProperties);
        menu->addAction(sigProp);
    }
    menu->exec(QCursor::pos());
    delete menu;
}

void SignaturePanel::slotViewProperties()
{
    Q_D(SignaturePanel);
    SignaturePropertiesDialog propDlg(d->m_document, d->m_currentForm, this);
    propDlg.exec();
}

void SignaturePanel::signUnsignedSignature()
{
    Q_D(SignaturePanel);
    SignaturePartUtils::signUnsignedSignature(d->m_currentForm, d->m_pageView, d->m_document);
}

void SignaturePanel::notifySetup(const QVector<Okular::Page *> & /*pages*/, int setupFlags)
{
    if (!(setupFlags & Okular::DocumentObserver::UrlChanged)) {
        return;
    }

    Q_D(SignaturePanel);
    const QVector<const Okular::FormFieldSignature *> signatureForms = SignatureGuiUtils::getSignatureFormFields(d->m_document);
    Q_EMIT documentHasSignatures(!signatureForms.isEmpty());
}

SignaturePanel::~SignaturePanel()
{
    Q_D(SignaturePanel);
    d->m_document->removeObserver(this);
    delete d->m_model;
}

void SignaturePanel::setPageView(PageView *pv)
{
    Q_D(SignaturePanel);
    d->m_pageView = pv;
}
