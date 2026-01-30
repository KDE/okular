/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "annotationpopup.h"

#include <KLocalizedString>
#include <QApplication>
#include <QClipboard>
#include <QIcon>
#include <QMenu>

#include "annotationpropertiesdialog.h"

#include "core/annotations.h"
#include "core/bookmarkmanager.h"
#include "core/document.h"
#include "core/page.h"
#include "gui/guiutils.h"
#include "okmenutitle.h"

#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QUrl>

Q_DECLARE_METATYPE(AnnotationPopup::AnnotPagePair)

namespace
{
bool annotationHasFileAttachment(Okular::Annotation *annotation)
{
    return (annotation->subType() == Okular::Annotation::AFileAttachment || annotation->subType() == Okular::Annotation::ARichMedia);
}

Okular::EmbeddedFile *embeddedFileFromAnnotation(Okular::Annotation *annotation)
{
    if (annotation->subType() == Okular::Annotation::AFileAttachment) {
        const Okular::FileAttachmentAnnotation *fileAttachAnnot = static_cast<Okular::FileAttachmentAnnotation *>(annotation);
        return fileAttachAnnot->embeddedFile();
    } else if (annotation->subType() == Okular::Annotation::ARichMedia) {
        const Okular::RichMediaAnnotation *richMediaAnnot = static_cast<Okular::RichMediaAnnotation *>(annotation);
        return richMediaAnnot->embeddedFile();
    } else {
        return nullptr;
    }
}

}

AnnotationPopup::AnnotationPopup(Okular::Document *document, MenuMode mode, QWidget *parent)
    : mParent(parent)
    , mDocument(document)
    , mMenuMode(mode)
{
}

void AnnotationPopup::addAnnotation(Okular::Annotation *annotation, int pageNumber)
{
    AnnotPagePair pair(annotation, pageNumber);
    if (!mAnnotations.contains(pair)) {
        mAnnotations.append(pair);
    }
}

void AnnotationPopup::exec(const QPoint point)
{
    if (mAnnotations.isEmpty()) {
        return;
    }

    QMenu menu(mParent);

    addActionsToMenu(&menu);

    menu.exec(point.isNull() ? QCursor::pos() : point);
}

void AnnotationPopup::addActionsToMenu(QMenu *menu)
{
    QAction *action = nullptr;

    if (mMenuMode == SingleAnnotationMode) {
        const bool onlyOne = (mAnnotations.count() == 1);

        const AnnotPagePair &pair = mAnnotations.at(0);

        menu->addAction(new OKMenuTitle(menu, i18ncp("Menu title", "Annotation", "%1 Annotations", mAnnotations.count())));

        action = menu->addAction(QIcon::fromTheme(QStringLiteral("comment")), i18n("&Open Pop-up Note"));
        action->setEnabled(onlyOne);
        connect(action, &QAction::triggered, menu, [this, pair] { doOpenAnnotationWindow(pair); });

        Okular::DocumentViewport vp = calculateAnnotationViewport(pair);
        bool isBookmarked = mDocument->bookmarkManager()->isBookmarked(vp);

        if (isBookmarked) {
            action = menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-remove")), i18n("Remove Bookmark"));
            action->setEnabled(onlyOne);
            connect(action, &QAction::triggered, menu, [this, pair] { doRemoveAnnotationBookmark(pair); });
        } else {
            action = menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Add Bookmark"));
            action->setEnabled(onlyOne);
            connect(action, &QAction::triggered, menu, [this, pair] { doAddAnnotationBookmark(pair); });
        }

        if (!pair.annotation->contents().isEmpty()) {
            action = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Text to Clipboard"));
            const bool copyAllowed = mDocument->isAllowed(Okular::AllowCopy);
            if (!copyAllowed) {
                action->setEnabled(false);
                action->setText(i18n("Copy forbidden by DRM"));
            }
            connect(action, &QAction::triggered, menu, [this, pair] { doCopyAnnotation(pair); });
        }

        action = menu->addAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("&Delete"));
        action->setEnabled(mDocument->isAllowed(Okular::AllowNotes));
        connect(action, &QAction::triggered, menu, [this] {
            for (const AnnotPagePair &pair : std::as_const(mAnnotations)) {
                doRemovePageAnnotation(pair);
            }
        });

        for (const AnnotPagePair &annot : std::as_const(mAnnotations)) {
            if (!mDocument->canRemovePageAnnotation(annot.annotation)) {
                action->setEnabled(false);
            }
        }

        action = menu->addAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("&Properties"));
        action->setEnabled(onlyOne);
        connect(action, &QAction::triggered, menu, [this, pair] { doOpenPropertiesDialog(pair); });

        if (onlyOne && annotationHasFileAttachment(pair.annotation)) {
            const Okular::EmbeddedFile *embeddedFile = embeddedFileFromAnnotation(pair.annotation);
            if (embeddedFile) {
                const QString openText = i18nc("%1 is the name of the file to open", "&Open '%1'…", embeddedFile->name());
                action = menu->addAction(QIcon::fromTheme(QStringLiteral("document-open")), openText);
                connect(action, &QAction::triggered, menu, [this, pair] { doOpenEmbeddedFile(pair); });

                const QString saveText = i18nc("%1 is the name of the file to save", "&Save '%1'…", embeddedFile->name());
                menu->addSeparator();
                action = menu->addAction(QIcon::fromTheme(QStringLiteral("document-save")), saveText);
                connect(action, &QAction::triggered, menu, [this, pair] { doSaveEmbeddedFile(pair); });
            }
        }
    } else {
        for (const AnnotPagePair &pair : std::as_const(mAnnotations)) {
            menu->addAction(new OKMenuTitle(menu, GuiUtils::captionForAnnotation(pair.annotation)));

            action = menu->addAction(QIcon::fromTheme(QStringLiteral("comment")), i18n("&Open Pop-up Note"));
            connect(action, &QAction::triggered, menu, [this, pair] { doOpenAnnotationWindow(pair); });

            Okular::DocumentViewport vp = calculateAnnotationViewport(pair);
            bool isBookmarked = mDocument->bookmarkManager()->isBookmarked(vp);

            if (isBookmarked) {
                action = menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-remove")), i18n("Remove Bookmark"));
                connect(action, &QAction::triggered, menu, [this, pair] { doRemoveAnnotationBookmark(pair); });
            } else {
                action = menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Add Bookmark"));
                connect(action, &QAction::triggered, menu, [this, pair] { doAddAnnotationBookmark(pair); });
            }

            if (!pair.annotation->contents().isEmpty()) {
                action = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Text to Clipboard"));
                const bool copyAllowed = mDocument->isAllowed(Okular::AllowCopy);
                if (!copyAllowed) {
                    action->setEnabled(false);
                    action->setText(i18n("Copy forbidden by DRM"));
                }
                connect(action, &QAction::triggered, menu, [this, pair] { doCopyAnnotation(pair); });
            }

            action = menu->addAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("&Delete"));
            action->setEnabled(mDocument->isAllowed(Okular::AllowNotes) && mDocument->canRemovePageAnnotation(pair.annotation));
            connect(action, &QAction::triggered, menu, [this, pair] { doRemovePageAnnotation(pair); });

            action = menu->addAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("&Properties"));
            connect(action, &QAction::triggered, menu, [this, pair] { doOpenPropertiesDialog(pair); });

            if (annotationHasFileAttachment(pair.annotation)) {
                const Okular::EmbeddedFile *embeddedFile = embeddedFileFromAnnotation(pair.annotation);
                if (embeddedFile) {
                    const QString openText = i18nc("%1 is the name of the file to open", "&Open '%1'…", embeddedFile->name());
                    action = menu->addAction(QIcon::fromTheme(QStringLiteral("document-open")), openText);
                    connect(action, &QAction::triggered, menu, [this, pair] { doOpenEmbeddedFile(pair); });

                    const QString saveText = i18nc("%1 is the name of the file to save", "&Save '%1'…", embeddedFile->name());
                    menu->addSeparator();
                    action = menu->addAction(QIcon::fromTheme(QStringLiteral("document-save")), saveText);
                    connect(action, &QAction::triggered, menu, [this, pair] { doSaveEmbeddedFile(pair); });
                }
            }
        }
    }
}

void AnnotationPopup::doCopyAnnotation(AnnotPagePair pair)
{
    const QString text = pair.annotation->contents();
    if (!text.isEmpty()) {
        QClipboard *cb = QApplication::clipboard();
        cb->setText(text, QClipboard::Clipboard);
    }
}

void AnnotationPopup::doRemovePageAnnotation(AnnotPagePair pair)
{
    if (pair.pageNumber != -1) {
        mDocument->removePageAnnotation(pair.pageNumber, pair.annotation);
    }
}

void AnnotationPopup::doOpenAnnotationWindow(AnnotPagePair pair)
{
    Q_EMIT openAnnotationWindow(pair.annotation, pair.pageNumber);
}

void AnnotationPopup::doOpenPropertiesDialog(AnnotPagePair pair)
{
    if (pair.pageNumber != -1) {
        AnnotsPropertiesDialog propdialog(mParent, mDocument, pair.pageNumber, pair.annotation);
        propdialog.exec();
    }
}

void AnnotationPopup::doSaveEmbeddedFile(AnnotPagePair pair)
{
    Okular::EmbeddedFile *embeddedFile = embeddedFileFromAnnotation(pair.annotation);
    GuiUtils::saveEmbeddedFile(embeddedFile, mParent);
}

void AnnotationPopup::doOpenEmbeddedFile(AnnotPagePair pair)
{
    Okular::EmbeddedFile *embeddedFile = embeddedFileFromAnnotation(pair.annotation);
    if (!embeddedFile) {
        return;
    }
    // preserve the file extension so the OS knows which app to use
    const QString name = embeddedFile->name();
    const QString extension = QFileInfo(name).suffix();
    const QString templateName = QDir::tempPath() + QLatin1Char('/') + QFileInfo(name).baseName() + QStringLiteral(".XXXXXX.") + extension;
    QTemporaryFile tempFile(templateName);
    tempFile.setAutoRemove(false);

    if (tempFile.open()) {
        tempFile.write(embeddedFile->data());
        tempFile.setPermissions(QFile::ReadOwner);
        const QString fileName = tempFile.fileName();
        tempFile.close();
        auto *job = new KIO::OpenUrlJob(QUrl::fromLocalFile(fileName));
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, mParent));
        job->start();
    }
}

// This code comes from Reviews::activated in side_reviews.cpp
Okular::DocumentViewport AnnotationPopup::calculateAnnotationViewport(AnnotPagePair pair) const
{
    Okular::DocumentViewport vp;

    const Okular::Page *page = mDocument->page(pair.pageNumber);
    if (!page || !pair.annotation) {
        return vp; // Return empty viewport on error
    }

    QRect rect = Okular::AnnotationUtils::annotationGeometry(pair.annotation, page->width(), page->height());
    Okular::NormalizedRect nr(rect, (int)page->width(), (int)page->height());

    vp.pageNumber = pair.pageNumber;
    vp.rePos.enabled = true;
    vp.rePos.pos = Okular::DocumentViewport::Center;
    vp.rePos.normalizedX = (nr.right + nr.left) / 2.0;
    vp.rePos.normalizedY = (nr.bottom + nr.top) / 2.0;

    return vp;
}

void AnnotationPopup::doAddAnnotationBookmark(AnnotPagePair pair)
{
    if (pair.pageNumber != -1) {
        Okular::DocumentViewport vp = calculateAnnotationViewport(pair);
        QString title = pair.annotation->contents();
        if (title.isEmpty()) {
            mDocument->bookmarkManager()->addBookmark(mDocument->currentDocument(), vp);
        } else {
            mDocument->bookmarkManager()->addBookmark(mDocument->currentDocument(), vp, title);
        }
    }
}

void AnnotationPopup::doRemoveAnnotationBookmark(AnnotPagePair pair)
{
    if (pair.pageNumber != -1) {
        Okular::DocumentViewport vp = calculateAnnotationViewport(pair);
        mDocument->bookmarkManager()->removeBookmark(vp);
    }
}
