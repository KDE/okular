/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "annotationpopup.h"

#include <KLocalizedString>
#include <QIcon>
#include <QMenu>

#include "annotationpropertiesdialog.h"

#include "core/annotations.h"
#include "core/document.h"
#include "guiutils.h"
#include "okmenutitle.h"

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
    if (!mAnnotations.contains(pair))
        mAnnotations.append(pair);
}

void AnnotationPopup::exec(const QPoint point)
{
    if (mAnnotations.isEmpty())
        return;

    QMenu menu(mParent);

    QAction *action = nullptr;

    const char *actionTypeId = "actionType";

    const QString openId = QStringLiteral("open");
    const QString deleteId = QStringLiteral("delete");
    const QString deleteAllId = QStringLiteral("deleteAll");
    const QString propertiesId = QStringLiteral("properties");
    const QString saveId = QStringLiteral("save");

    if (mMenuMode == SingleAnnotationMode) {
        const bool onlyOne = (mAnnotations.count() == 1);

        const AnnotPagePair &pair = mAnnotations.at(0);

        menu.addAction(new OKMenuTitle(&menu, i18np("Annotation", "%1 Annotations", mAnnotations.count())));

        action = menu.addAction(QIcon::fromTheme(QStringLiteral("comment")), i18n("&Open Pop-up Note"));
        action->setData(QVariant::fromValue(pair));
        action->setEnabled(onlyOne);
        action->setProperty(actionTypeId, openId);

        action = menu.addAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("&Delete"));
        action->setEnabled(mDocument->isAllowed(Okular::AllowNotes));
        action->setProperty(actionTypeId, deleteAllId);

        for (const AnnotPagePair &pair : qAsConst(mAnnotations)) {
            if (!mDocument->canRemovePageAnnotation(pair.annotation))
                action->setEnabled(false);
        }

        action = menu.addAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("&Properties"));
        action->setData(QVariant::fromValue(pair));
        action->setEnabled(onlyOne);
        action->setProperty(actionTypeId, propertiesId);

        if (onlyOne && annotationHasFileAttachment(pair.annotation)) {
            const Okular::EmbeddedFile *embeddedFile = embeddedFileFromAnnotation(pair.annotation);
            if (embeddedFile) {
                const QString saveText = i18nc("%1 is the name of the file to save", "&Save '%1'...", embeddedFile->name());

                menu.addSeparator();
                action = menu.addAction(QIcon::fromTheme(QStringLiteral("document-save")), saveText);
                action->setData(QVariant::fromValue(pair));
                action->setProperty(actionTypeId, saveId);
            }
        }
    } else {
        for (const AnnotPagePair &pair : qAsConst(mAnnotations)) {
            menu.addAction(new OKMenuTitle(&menu, GuiUtils::captionForAnnotation(pair.annotation)));

            action = menu.addAction(QIcon::fromTheme(QStringLiteral("comment")), i18n("&Open Pop-up Note"));
            action->setData(QVariant::fromValue(pair));
            action->setProperty(actionTypeId, openId);

            action = menu.addAction(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("&Delete"));
            action->setEnabled(mDocument->isAllowed(Okular::AllowNotes) && mDocument->canRemovePageAnnotation(pair.annotation));
            action->setData(QVariant::fromValue(pair));
            action->setProperty(actionTypeId, deleteId);

            action = menu.addAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("&Properties"));
            action->setData(QVariant::fromValue(pair));
            action->setProperty(actionTypeId, propertiesId);

            if (annotationHasFileAttachment(pair.annotation)) {
                const Okular::EmbeddedFile *embeddedFile = embeddedFileFromAnnotation(pair.annotation);
                if (embeddedFile) {
                    const QString saveText = i18nc("%1 is the name of the file to save", "&Save '%1'...", embeddedFile->name());

                    menu.addSeparator();
                    action = menu.addAction(QIcon::fromTheme(QStringLiteral("document-save")), saveText);
                    action->setData(QVariant::fromValue(pair));
                    action->setProperty(actionTypeId, saveId);
                }
            }
        }
    }

    QAction *choice = menu.exec(point.isNull() ? QCursor::pos() : point);

    // check if the user really selected an action
    if (choice) {
        const AnnotPagePair pair = choice->data().value<AnnotPagePair>();

        const QString actionType = choice->property(actionTypeId).toString();
        if (actionType == openId) {
            emit openAnnotationWindow(pair.annotation, pair.pageNumber);
        } else if (actionType == deleteId) {
            if (pair.pageNumber != -1)
                mDocument->removePageAnnotation(pair.pageNumber, pair.annotation);
        } else if (actionType == deleteAllId) {
            for (const AnnotPagePair &pair : qAsConst(mAnnotations)) {
                if (pair.pageNumber != -1)
                    mDocument->removePageAnnotation(pair.pageNumber, pair.annotation);
            }
        } else if (actionType == propertiesId) {
            if (pair.pageNumber != -1) {
                AnnotsPropertiesDialog propdialog(mParent, mDocument, pair.pageNumber, pair.annotation);
                propdialog.exec();
            }
        } else if (actionType == saveId) {
            Okular::EmbeddedFile *embeddedFile = embeddedFileFromAnnotation(pair.annotation);
            GuiUtils::saveEmbeddedFile(embeddedFile, mParent);
        }
    }
}
