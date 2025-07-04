/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signaturemodel.h"

#include "certificatemodel.h"
#include "signatureguiutils.h"

#include <KLocalizedString>

#include <QFile>
#include <QIcon>
#include <QList>
#include <QPointer>

#include "core/document.h"
#include "core/form.h"
#include "core/observer.h"
#include "core/page.h"
#include "core/signatureutils.h"

struct SignatureItem {
    enum DataType { Root, RevisionInfo, ValidityStatus, CertificateStatus, SigningTime, Reason, Location, FieldInfo, SignatureType };

    SignatureItem();
    SignatureItem(SignatureItem *parent, const Okular::FormFieldSignature *form, DataType type, int page);
    ~SignatureItem();

    SignatureItem(const SignatureItem &) = delete;
    SignatureItem &operator=(const SignatureItem &) = delete;

    QList<SignatureItem *> children;
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
    explicit SignatureModelPrivate(SignatureModel *qq);
    ~SignatureModelPrivate() override;

    void notifySetup(const QList<Okular::Page *> &pages, int setupFlags) override;

    QModelIndex indexForItem(SignatureItem *item) const;

    SignatureModel *q;
    SignatureItem *root;
    QPointer<Okular::Document> document;
    mutable QHash<const Okular::FormFieldSignature *, CertificateModel *> certificateForForm;
};

SignatureModelPrivate::SignatureModelPrivate(SignatureModel *qq)
    : q(qq)
    , root(new SignatureItem)
{
}

SignatureModelPrivate::~SignatureModelPrivate()
{
    qDeleteAll(certificateForForm);
    delete root;
}

static void updateFormFieldSignaturePointer(SignatureItem *item, const QList<Okular::Page *> &pages)
{
    if (item->form) {
        const QList<Okular::FormField *> formFields = pages[item->page]->formFields();
        for (Okular::FormField *f : formFields) {
            if (item->form->id() == f->id()) {
                item->form = static_cast<Okular::FormFieldSignature *>(f);
                break;
            }
        }
        if (!item->form) {
            qWarning() << "Lost signature form field, something went wrong";
        }
    }

    for (SignatureItem *child : std::as_const(item->children)) {
        updateFormFieldSignaturePointer(child, pages);
    }
}

void SignatureModelPrivate::notifySetup(const QList<Okular::Page *> &pages, int setupFlags)
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

    if (pages.isEmpty()) {
        q->endResetModel();
        Q_EMIT q->countChanged();
        return;
    }

    int revNumber = 1;
    int unsignedSignatureNumber = 1;
    const QList<const Okular::FormFieldSignature *> signatureFormFields = SignatureGuiUtils::getSignatureFormFields(document);
    for (const Okular::FormFieldSignature *sf : signatureFormFields) {
        const int pageNumber = sf->page()->number();

        if (sf->signatureType() == Okular::FormFieldSignature::UnsignedSignature) {
            auto *parentItem = new SignatureItem(root, sf, SignatureItem::RevisionInfo, pageNumber);
            parentItem->displayString = i18nc("Digital signature", "Signature placeholder %1", unsignedSignatureNumber);

            auto childItem = new SignatureItem(parentItem, sf, SignatureItem::FieldInfo, pageNumber);
            childItem->displayString = i18n("Field: %1 on page %2", sf->name(), pageNumber + 1);

            ++unsignedSignatureNumber;
        } else {
            const Okular::SignatureInfo &info = sf->signatureInfo();

            // based on whether or not signature form is a nullptr it is decided if clicking on an item should change the viewport.
            auto *parentItem = new SignatureItem(root, sf, SignatureItem::RevisionInfo, pageNumber);
            parentItem->displayString = i18n("Rev. %1: Signed By %2", revNumber, info.signerName());

            auto childItem1 = new SignatureItem(parentItem, nullptr, SignatureItem::ValidityStatus, pageNumber);
            childItem1->displayString = SignatureGuiUtils::getReadableSignatureStatus(info.signatureStatus());

            auto childItem1a = new SignatureItem(parentItem, nullptr, SignatureItem::CertificateStatus, pageNumber);
            childItem1a->displayString = SignatureGuiUtils::getReadableCertStatus(info.certificateStatus());
            sf->subscribeUpdates([childItem1a, sf, this]() {
                const Okular::SignatureInfo &info = sf->signatureInfo();
                childItem1a->displayString = SignatureGuiUtils::getReadableCertStatus(info.certificateStatus());
                auto index = indexForItem(childItem1a);
                q->dataChanged(index, index);
            });

            auto childItem2 = new SignatureItem(parentItem, nullptr, SignatureItem::SigningTime, pageNumber);
            childItem2->displayString = i18n("Signing Time: %1", QLocale().toString(info.signingTime(), QLocale::LongFormat));

            const QString reason = info.reason();
            if (!reason.isEmpty()) {
                auto childItem3 = new SignatureItem(parentItem, nullptr, SignatureItem::Reason, pageNumber);
                childItem3->displayString = i18n("Reason: %1", reason);
            }
            const QString location = info.location();
            if (!location.isEmpty()) {
                auto childItem3 = new SignatureItem(parentItem, nullptr, SignatureItem::Location, pageNumber);
                childItem3->displayString = i18n("Location: %1", location);
            }

            auto childItem4 = new SignatureItem(parentItem, sf, SignatureItem::FieldInfo, pageNumber);
            childItem4->displayString = i18n("Field: %1 on page %2", sf->name(), pageNumber + 1);
            auto signatureType = [sf] {
                switch (sf->signatureType()) {
                case Okular::FormFieldSignature::G10cPgpSignatureDetached:
                    return i18nc("Digital signature type", "PGP Signature");
                case Okular::FormFieldSignature::AdbePkcs7detached:
                    return i18nc("Digital signature type", "Adobe PKCS7");
                case Okular::FormFieldSignature::AdbePkcs7sha1:
                    return i18nc("Digital signature type", "Adobe PKCS7 Sha1");
                case Okular::FormFieldSignature::EtsiCAdESdetached:
                    return i18nc("Digital signature type", "ETSI CAdES");
                case Okular::FormFieldSignature::UnknownType:
                    return i18nc("Digital signature type", "Unknown");
                case Okular::FormFieldSignature::UnsignedSignature:
                    return i18nc("Digital signature type", "Signature placeholder");
                }
                return QString {};
            }();

            auto childItem5 = new SignatureItem(parentItem, nullptr, SignatureItem::SignatureType, pageNumber);
            childItem5->displayString = i18n("Signature Type: %1", signatureType);

            ++revNumber;
        }
    }
    q->endResetModel();
    Q_EMIT q->countChanged();
}

QModelIndex SignatureModelPrivate::indexForItem(SignatureItem *item) const
{
    if (item->parent) {
        const int index = item->parent->children.indexOf(item);
        if (index >= 0 && index < item->parent->children.count()) {
            return q->createIndex(index, 0, item);
        }
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

    if (!index.isValid()) {
        return QVariant();
    }

    const SignatureItem *item = static_cast<SignatureItem *>(index.internalPointer());
    if (item == d->root) {
        return QVariant();
    }

    const Okular::FormFieldSignature *form = item->form ? item->form : item->parent->form;

    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return item->displayString;
    case Qt::DecorationRole:
        if (item->type == SignatureItem::RevisionInfo) {
            const Okular::SignatureInfo::SignatureStatus signatureStatus = form->signatureInfo().signatureStatus();
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
        return QVariant::fromValue<const Okular::FormFieldSignature *>(form);
    case PageRole:
        return item->page;
    case ReadableStatusRole:
        return SignatureGuiUtils::getReadableSignatureStatus(form->signatureInfo().signatureStatus());
    case ReadableModificationSummary:
        return SignatureGuiUtils::getReadableModificationSummary(form->signatureInfo());
    case SignerNameRole:
        return form->signatureInfo().signerName();
    case SigningTimeRole:
        return QLocale().toString(form->signatureInfo().signingTime(), QLocale::LongFormat);
    case SigningLocationRole:
        return form->signatureInfo().location();
    case SigningReasonRole:
        return form->signatureInfo().reason();
    case CertificateModelRole: {
        auto it = d->certificateForForm.constFind(form);
        if (it != d->certificateForForm.constEnd()) {
            return QVariant::fromValue(it.value());
        }
        CertificateModel *cm = new CertificateModel(form->signatureInfo().certificateInfo());
        d->certificateForForm.insert(form, cm);
        return QVariant::fromValue(cm);
    }
    case SignatureRevisionIndexRole: {
        const Okular::SignatureInfo &signatureInfo = form->signatureInfo();
        const Okular::SignatureInfo::SignatureStatus signatureStatus = signatureInfo.signatureStatus();
        if (signatureStatus != Okular::SignatureInfo::SignatureStatusUnknown && !signatureInfo.signsTotalDocument()) {
            const QList<const Okular::FormFieldSignature *> signatureFormFields = SignatureGuiUtils::getSignatureFormFields(d->document);
            return signatureFormFields.indexOf(form);
        }
        return -1;
    }
    case IsUnsignedSignatureRole: {
        return form->signatureType() == Okular::FormFieldSignature::UnsignedSignature;
    }
    }

    return QVariant();
}

bool SignatureModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return true;
    }

    const SignatureItem *item = static_cast<SignatureItem *>(parent.internalPointer());
    return !item->children.isEmpty();
}

QModelIndex SignatureModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const SignatureModel);

    if (row < 0 || column != 0) {
        return QModelIndex();
    }

    const SignatureItem *item = parent.isValid() ? static_cast<SignatureItem *>(parent.internalPointer()) : d->root;
    if (row < item->children.count()) {
        return createIndex(row, column, item->children.at(row));
    }

    return QModelIndex();
}

QModelIndex SignatureModel::parent(const QModelIndex &index) const
{
    Q_D(const SignatureModel);

    if (!index.isValid()) {
        return QModelIndex();
    }

    const SignatureItem *item = static_cast<SignatureItem *>(index.internalPointer());
    return d->indexForItem(item->parent);
}

int SignatureModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const SignatureModel);

    const SignatureItem *item = parent.isValid() ? static_cast<SignatureItem *>(parent.internalPointer()) : d->root;
    return item->children.count();
}

QHash<int, QByteArray> SignatureModel::roleNames() const
{
    static QHash<int, QByteArray> res;
    if (res.isEmpty()) {
        res = QAbstractItemModel::roleNames();
        res.insert(FormRole, "signatureFormField");
        res.insert(PageRole, "page");
        res.insert(ReadableStatusRole, "readableStatus");
        res.insert(ReadableModificationSummary, "readableModificationSummary");
        res.insert(SignerNameRole, "signerName");
        res.insert(SigningTimeRole, "signingTime");
        res.insert(SigningLocationRole, "signingLocation");
        res.insert(SigningReasonRole, "signingReason");
        res.insert(CertificateModelRole, "certificateModel");
        res.insert(SignatureRevisionIndexRole, "signatureRevisionIndex");
        res.insert(IsUnsignedSignatureRole, "isUnsignedSignature");
    }

    return res;
}

bool SignatureModel::saveSignedVersion(int signatureRevisionIndex, const QUrl &filePath) const
{
    Q_D(const SignatureModel);
    const QList<const Okular::FormFieldSignature *> signatureFormFields = SignatureGuiUtils::getSignatureFormFields(d->document);
    if (signatureRevisionIndex < 0 || signatureRevisionIndex >= signatureFormFields.count()) {
        qWarning() << "Invalid signatureRevisionIndex given to saveSignedVersion";
        return false;
    }
    const Okular::FormFieldSignature *signature = signatureFormFields[signatureRevisionIndex];
    const QByteArray signedRevisionData = d->document->requestSignedRevisionData(signature->signatureInfo());

    if (!filePath.isLocalFile()) {
        qWarning() << "Unexpected non local path given to saveSignedVersion" << filePath;
        return false;
    }
    QFile f(filePath.toLocalFile());
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open path for writing in saveSignedVersion" << filePath;
        return false;
    }
    if (f.write(signedRevisionData) != signedRevisionData.size()) {
        qWarning() << "Failed to write all data in saveSignedVersion" << filePath;
        return false;
    }
    return true;
}

#include "moc_signaturemodel.cpp"
