/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <pino@kde.org>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "annotationmodel.h"

#include <QList>
#include <QPointer>

#include <KLocalizedString>
#include <QIcon>

#include "core/annotations.h"
#include "core/document.h"
#include "core/observer.h"
#include "core/page.h"
#include "gui/guiutils.h"

struct AnnItem {
    AnnItem();
    AnnItem(AnnItem *parent, Okular::Annotation *ann);
    AnnItem(AnnItem *parent, int page);
    ~AnnItem();

    AnnItem(const AnnItem &) = delete;
    AnnItem &operator=(const AnnItem &) = delete;

    AnnItem *parent;
    QList<AnnItem *> children;

    Okular::Annotation *annotation;
    int page;
};

static QList<Okular::Annotation *> filterOutWidgetAnnotations(const QList<Okular::Annotation *> &annotations)
{
    QList<Okular::Annotation *> result;

    for (Okular::Annotation *annotation : annotations) {
        if (annotation->subType() == Okular::Annotation::AWidget) {
            continue;
        }

        result.append(annotation);
    }

    return result;
}

class AnnotationModelPrivate : public Okular::DocumentObserver
{
public:
    explicit AnnotationModelPrivate(AnnotationModel *qq);
    ~AnnotationModelPrivate() override;

    void notifySetup(const QVector<Okular::Page *> &pages, int setupFlags) override;
    void notifyPageChanged(int page, int flags) override;

    QModelIndex indexForItem(AnnItem *item) const;
    void rebuildTree(const QVector<Okular::Page *> &pages);
    AnnItem *findItem(int page, int *index) const;

    AnnotationModel *q;
    AnnItem *root;
    QPointer<Okular::Document> document;
};

AnnItem::AnnItem()
    : parent(nullptr)
    , annotation(nullptr)
    , page(-1)
{
}

AnnItem::AnnItem(AnnItem *_parent, Okular::Annotation *ann)
    : parent(_parent)
    , annotation(ann)
    , page(_parent->page)
{
    Q_ASSERT(!parent->annotation);
    parent->children.append(this);
}

AnnItem::AnnItem(AnnItem *_parent, int _page)
    : parent(_parent)
    , annotation(nullptr)
    , page(_page)
{
    Q_ASSERT(!parent->parent);
    parent->children.append(this);
}

AnnItem::~AnnItem()
{
    qDeleteAll(children);
}

AnnotationModelPrivate::AnnotationModelPrivate(AnnotationModel *qq)
    : q(qq)
    , root(new AnnItem)
{
}

AnnotationModelPrivate::~AnnotationModelPrivate()
{
    delete root;
}

static void updateAnnotationPointer(AnnItem *item, const QVector<Okular::Page *> &pages)
{
    if (item->annotation) {
        item->annotation = pages[item->page]->annotation(item->annotation->uniqueName());
        if (!item->annotation) {
            qWarning() << "Lost annotation on document save, something went wrong";
        }
    }

    for (AnnItem *child : qAsConst(item->children)) {
        updateAnnotationPointer(child, pages);
    }
}

void AnnotationModelPrivate::notifySetup(const QVector<Okular::Page *> &pages, int setupFlags)
{
    if (!(setupFlags & Okular::DocumentObserver::DocumentChanged)) {
        if (setupFlags & Okular::DocumentObserver::UrlChanged) {
            // Here with UrlChanged and no document changed it means we
            // need to update all the Annotation* otherwise
            // they still point to the old document ones, luckily the old ones are still
            // around so we can look for the new ones using unique ids, etc
            updateAnnotationPointer(root, pages);
        }
        return;
    }

    q->beginResetModel();
    qDeleteAll(root->children);
    root->children.clear();

    rebuildTree(pages);
    q->endResetModel();
}

void AnnotationModelPrivate::notifyPageChanged(int page, int flags)
{
    // we are strictly interested in annotations
    if (!(flags & Okular::DocumentObserver::Annotations)) {
        return;
    }

    const QList<Okular::Annotation *> annots = filterOutWidgetAnnotations(document->page(page)->annotations());
    int annItemIndex = -1;
    AnnItem *annItem = findItem(page, &annItemIndex);
    // case 1: the page has no more annotations
    //         => remove the branch, if any
    if (annots.isEmpty()) {
        if (annItem) {
            q->beginRemoveRows(indexForItem(root), annItemIndex, annItemIndex);
            delete root->children.at(annItemIndex);
            root->children.removeAt(annItemIndex);
            q->endRemoveRows();
        }
        return;
    }
    // case 2: no existing branch
    //         => add a new branch, and add the annotations for the page
    if (!annItem) {
        int i = 0;
        while (i < root->children.count() && root->children.at(i)->page < page) {
            ++i;
        }

        AnnItem *annItem = new AnnItem();
        annItem->page = page;
        annItem->parent = root;
        q->beginInsertRows(indexForItem(root), i, i);
        annItem->parent->children.insert(i, annItem);
        q->endInsertRows();
        int newid = 0;
        for (Okular::Annotation *annot : annots) {
            q->beginInsertRows(indexForItem(annItem), newid, newid);
            new AnnItem(annItem, annot);
            q->endInsertRows();
            ++newid;
        }
        return;
    }
    // case 3: existing branch, less annotations than items
    //         => lookup and remove the annotations
    if (annItem->children.count() > annots.count()) {
        for (int i = annItem->children.count(); i > 0; --i) {
            Okular::Annotation *ref = annItem->children.at(i - 1)->annotation;
            bool found = false;
            for (Okular::Annotation *annot : annots) {
                if (annot == ref) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                q->beginRemoveRows(indexForItem(annItem), i - 1, i - 1);
                delete annItem->children.at(i - 1);
                annItem->children.removeAt(i - 1);
                q->endRemoveRows();
            }
        }
        return;
    }
    // case 4: existing branch, less items than annotations
    //         => lookup and add annotations if not in the branch
    if (annots.count() > annItem->children.count()) {
        for (Okular::Annotation *ref : annots) {
            bool found = false;
            int count = annItem->children.count();
            for (int i = 0; !found && i < count; ++i) {
                if (ref == annItem->children.at(i)->annotation) {
                    found = true;
                }
            }
            if (!found) {
                q->beginInsertRows(indexForItem(annItem), count, count);
                new AnnItem(annItem, ref);
                q->endInsertRows();
            }
        }
        return;
    }
    // case 5: the data of some annotation changed
    // TODO: what do we do in this case?
    // FIXME: for now, update ALL the annotations for that page
    for (int i = 0; i < annItem->children.count(); ++i) {
        QModelIndex index = indexForItem(annItem->children.at(i));
        Q_EMIT q->dataChanged(index, index);
    }
}

QModelIndex AnnotationModelPrivate::indexForItem(AnnItem *item) const
{
    if (item->parent) {
        int id = item->parent->children.indexOf(item);
        if (id >= 0 && id < item->parent->children.count()) {
            return q->createIndex(id, 0, item);
        }
    }
    return QModelIndex();
}

void AnnotationModelPrivate::rebuildTree(const QVector<Okular::Page *> &pages)
{
    if (pages.isEmpty()) {
        return;
    }

    Q_EMIT q->layoutAboutToBeChanged();
    for (int i = 0; i < pages.count(); ++i) {
        const QList<Okular::Annotation *> annots = filterOutWidgetAnnotations(pages.at(i)->annotations());
        if (annots.isEmpty()) {
            continue;
        }

        AnnItem *annItem = new AnnItem(root, i);
        for (Okular::Annotation *annot : annots) {
            new AnnItem(annItem, annot);
        }
    }
    Q_EMIT q->layoutChanged();
}

AnnItem *AnnotationModelPrivate::findItem(int page, int *index) const
{
    for (int i = 0; i < root->children.count(); ++i) {
        AnnItem *tmp = root->children.at(i);
        if (tmp->page == page) {
            if (index) {
                *index = i;
            }
            return tmp;
        }
    }
    if (index) {
        *index = -1;
    }
    return nullptr;
}

AnnotationModel::AnnotationModel(Okular::Document *document, QObject *parent)
    : QAbstractItemModel(parent)
    , d(new AnnotationModelPrivate(this))
{
    d->document = document;

    d->document->addObserver(d);
}

AnnotationModel::~AnnotationModel()
{
    if (d->document) {
        d->document->removeObserver(d);
    }

    delete d;
}

int AnnotationModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant AnnotationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    AnnItem *item = static_cast<AnnItem *>(index.internalPointer());
    if (!item->annotation) {
        if (role == Qt::DisplayRole) {
            return i18n("Page %1", item->page + 1);
        } else if (role == Qt::DecorationRole) {
            return QIcon::fromTheme(QStringLiteral("text-plain"));
        } else if (role == PageRole) {
            return item->page;
        }

        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole:
        return GuiUtils::captionForAnnotation(item->annotation);
        break;
    case Qt::DecorationRole:
        return QIcon::fromTheme(QStringLiteral("okular"));
        break;
    case Qt::ToolTipRole:
        return GuiUtils::prettyToolTip(item->annotation);
        break;
    case AuthorRole:
        return item->annotation->author();
        break;
    case PageRole:
        return item->page;
        break;
    }
    return QVariant();
}

bool AnnotationModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return true;
    }

    AnnItem *item = static_cast<AnnItem *>(parent.internalPointer());
    return !item->children.isEmpty();
}

QVariant AnnotationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    if (section == 0 && role == Qt::DisplayRole) {
        return QString::fromLocal8Bit("Annotations");
    }

    return QVariant();
}

QModelIndex AnnotationModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column != 0) {
        return QModelIndex();
    }

    AnnItem *item = parent.isValid() ? static_cast<AnnItem *>(parent.internalPointer()) : d->root;
    if (row < item->children.count()) {
        return createIndex(row, column, item->children.at(row));
    }

    return QModelIndex();
}

QModelIndex AnnotationModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    AnnItem *item = static_cast<AnnItem *>(index.internalPointer());
    return d->indexForItem(item->parent);
}

int AnnotationModel::rowCount(const QModelIndex &parent) const
{
    AnnItem *item = parent.isValid() ? static_cast<AnnItem *>(parent.internalPointer()) : d->root;
    return item->children.count();
}

bool AnnotationModel::isAnnotation(const QModelIndex &index) const
{
    return annotationForIndex(index);
}

Okular::Annotation *AnnotationModel::annotationForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    AnnItem *item = static_cast<AnnItem *>(index.internalPointer());
    return item->annotation;
}

#include "moc_annotationmodel.cpp"
