/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signaturemodel.h"
#include "signatureguiutils.h"

#include <KLocalizedString>

#include <QIcon>
#include <QPointer>
#include <QVector>

#include "core/document.h"
#include "core/form.h"
#include "core/observer.h"
#include "core/page.h"
#include "core/signatureutils.h"

struct SignatureItem {
    enum DataType { Root, RevisionInfo, ValidityStatus, SigningTime, Reason, FieldInfo };

    SignatureItem();
    SignatureItem(SignatureItem *parent, const Okular::FormFieldSignature *form, DataType type, int page);
    ~SignatureItem();

    SignatureItem(const SignatureItem &) = delete;
    SignatureItem &operator=(const SignatureItem &) = delete;

    QVector<SignatureItem *> children;
    SignatureItem *parent;
    const Okular::FormFieldSignature *form;
    QString displayString;
    DataType type;
    int page;
};

SignatureItem::SignatureItem()
    : parent(nullptr)
    , form(nullptr)
    , type(Root)
    , page(-1)
{
}

SignatureItem::SignatureItem(SignatureItem *_parent, const Okular::FormFieldSignature *_form, DataType _type, int _page)
    : parent(_parent)
    , form(_form)
    , type(_type)
    , page(_page)
{
    Q_ASSERT(parent);
    parent->children.append(this);
}

SignatureItem::~SignatureItem()
{
    qDeleteAll(children);
}

class SignatureModelPrivate : public Okular::DocumentObserver
{
public:
    SignatureModelPrivate(SignatureModel *qq);
    ~SignatureModelPrivate() override;

    void notifySetup(const QVector<Okular::Page *> &pages, int setupFlags) override;

    QModelIndex indexForItem(SignatureItem *item) const;

    SignatureModel *q;
    SignatureItem *root;
    QPointer<Okular::Document> document;
};

SignatureModelPrivate::SignatureModelPrivate(SignatureModel *qq)
    : q(qq)
    , root(new SignatureItem)
{
}

SignatureModelPrivate::~SignatureModelPrivate()
{
    delete root;
}

static void updateFormFieldSignaturePointer(SignatureItem *item, const QVector<Okular::Page *> &pages)
{
    if (item->form) {
        const QLinkedList<Okular::FormField *> formFields = pages[item->page]->formFields();
        for (Okular::FormField *f : formFields) {
            if (item->form->id() == f->id()) {
                item->form = static_cast<Okular::FormFieldSignature *>(f);
                break;
            }
        }
        if (!item->form)
            qWarning() << "Lost signature form field, something went wrong";
    }

    for (SignatureItem *child : qAsConst(item->children)) {
        updateFormFieldSignaturePointer(child, pages);
    }
}

void SignatureModelPrivate::notifySetup(const QVector<Okular::Page *> &pages, int setupFlags)
{
    if (!(setupFlags & Okular::DocumentObserver::DocumentChanged)) {
        if (setupFlags & Okular::DocumentObserver::UrlChanged) {
            updateFormFieldSignaturePointer(root, pages);
        }
        return;
    }

    q->beginResetModel();
    qDeleteAll(root->children);
    root->children.clear();
    for (const Okular::Page *page : pages) {
        const int currentPage = page->number();
        // get form fields page by page so that page number and index of the form can be determined.
        const QVector<const Okular::FormFieldSignature *> signatureFormFields = SignatureGuiUtils::getSignatureFormFields(document, false, currentPage);
        if (signatureFormFields.isEmpty())
            continue;

        for (int i = 0; i < signatureFormFields.count(); i++) {
            const Okular::FormFieldSignature *sf = signatureFormFields[i];
            const Okular::SignatureInfo &info = sf->signatureInfo();

            // based on whether or not signature form is a nullptr it is decided if clicking on an item should change the viewport.
            auto *parentItem = new SignatureItem(root, sf, SignatureItem::RevisionInfo, currentPage);
            parentItem->displayString = i18n("Rev. %1: Signed By %2", i + 1, info.signerName());

            auto childItem1 = new SignatureItem(parentItem, nullptr, SignatureItem::ValidityStatus, currentPage);
            childItem1->displayString = SignatureGuiUtils::getReadableSignatureStatus(info.signatureStatus());

            auto childItem2 = new SignatureItem(parentItem, nullptr, SignatureItem::SigningTime, currentPage);
            childItem2->displayString = i18n("Signing Time: %1", info.signingTime().toString(Qt::DefaultLocaleLongDate));

            const QString reason = info.reason();
            if (!reason.isEmpty()) {
                auto childItem3 = new SignatureItem(parentItem, nullptr, SignatureItem::Reason, currentPage);
                childItem3->displayString = i18n("Reason: %1", reason);
            }

            auto childItem4 = new SignatureItem(parentItem, sf, SignatureItem::FieldInfo, currentPage);
            childItem4->displayString = i18n("Field: %1 on page %2", sf->name(), currentPage + 1);
        }
    }
    q->endResetModel();
}

QModelIndex SignatureModelPrivate::indexForItem(SignatureItem *item) const
{
    if (item->parent) {
        const int index = item->parent->children.indexOf(item);
        if (index >= 0 && index < item->parent->children.count())
            return q->createIndex(index, 0, item);
    }
    return QModelIndex();
}

SignatureModel::SignatureModel(Okular::Document *doc, QObject *parent)
    : QAbstractItemModel(parent)
    , d_ptr(new SignatureModelPrivate(this))
{
    Q_D(SignatureModel);
    d->document = doc;
    d->document->addObserver(d);
}

SignatureModel::~SignatureModel()
{
    Q_D(SignatureModel);
    d->document->removeObserver(d);
}

int SignatureModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant SignatureModel::data(const QModelIndex &index, int role) const
{
    Q_D(const SignatureModel);

    if (!index.isValid())
        return QVariant();

    const SignatureItem *item = static_cast<SignatureItem *>(index.internalPointer());
    if (item == d->root)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return item->displayString;
    case Qt::DecorationRole:
        if (item->type == SignatureItem::RevisionInfo) {
            const Okular::SignatureInfo::SignatureStatus signatureStatus = item->form->signatureInfo().signatureStatus();
            switch (signatureStatus) {
            case Okular::SignatureInfo::SignatureValid:
                return QIcon::fromTheme(QStringLiteral("dialog-ok"));
            case Okular::SignatureInfo::SignatureInvalid:
                return QIcon::fromTheme(QStringLiteral("dialog-close"));
            case Okular::SignatureInfo::SignatureDigestMismatch:
                return QIcon::fromTheme(QStringLiteral("dialog-warning"));
            default:
                return QIcon::fromTheme(QStringLiteral("dialog-question"));
            }
        }
        return QIcon();
    case FormRole:
        return QVariant::fromValue<const Okular::FormFieldSignature *>(item->form);
    case PageRole:
        return item->page;
    }

    return QVariant();
}

bool SignatureModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return true;

    const SignatureItem *item = static_cast<SignatureItem *>(parent.internalPointer());
    return !item->children.isEmpty();
}

QModelIndex SignatureModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const SignatureModel);

    if (row < 0 || column != 0)
        return QModelIndex();

    const SignatureItem *item = parent.isValid() ? static_cast<SignatureItem *>(parent.internalPointer()) : d->root;
    if (row < item->children.count())
        return createIndex(row, column, item->children.at(row));

    return QModelIndex();
}

QModelIndex SignatureModel::parent(const QModelIndex &index) const
{
    Q_D(const SignatureModel);

    if (!index.isValid())
        return QModelIndex();

    const SignatureItem *item = static_cast<SignatureItem *>(index.internalPointer());
    return d->indexForItem(item->parent);
}

int SignatureModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const SignatureModel);

    const SignatureItem *item = parent.isValid() ? static_cast<SignatureItem *>(parent.internalPointer()) : d->root;
    return item->children.count();
}

#include "moc_signaturemodel.cpp"
