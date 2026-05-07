/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ANNOTATIONPOPUP_H
#define ANNOTATIONPOPUP_H

#include <QList>
#include <QObject>
#include <QPair>
#include <QPoint>

#include "core/area.h"
#include "okularpart_export.h"

class QMenu;

namespace Okular
{
class Annotation;
class Document;
class DocumentViewport;
}

class OKULARPART_EXPORT AnnotationPopup : public QObject
{
    Q_OBJECT

public:
    static constexpr const char *annotationClipboardMimeType = "application/x-okular-annotation+xml";
    static constexpr const int annotationClipboardFormatVersion = 1;

    /**
     * Describes the structure of the popup menu.
     */
    enum MenuMode {
        SingleAnnotationMode, ///< The menu shows only entries to manipulate a single annotation, or multiple annotations as a group.
        MultiAnnotationMode   ///< The menu shows entries to manipulate multiple annotations.
    };

    AnnotationPopup(Okular::Document *document, MenuMode mode, QWidget *parent = nullptr);

    void addAnnotation(Okular::Annotation *annotation, int pageNumber);

    /* You only need to use this if you don't plan on using exec() */
    void addActionsToMenu(QMenu *menu);

    void exec(const QPoint point = QPoint());
    void pasteAnnotationToPage(int pageNumber, const Okular::NormalizedPoint *targetPoint = nullptr);

    static bool clipboardHasAnnotations();

Q_SIGNALS:
    void openAnnotationWindow(Okular::Annotation *annotation, int pageNumber);

public:
    struct AnnotPagePair {
        AnnotPagePair()
            : annotation(nullptr)
            , pageNumber(-1)
        {
        }

        AnnotPagePair(Okular::Annotation *a, int pn)
            : annotation(a)
            , pageNumber(pn)
        {
        }

        bool operator==(const AnnotPagePair pair) const
        {
            return annotation == pair.annotation && pageNumber == pair.pageNumber;
        }

        Okular::Annotation *annotation;
        int pageNumber;
    };

    void doCopyAnnotation(AnnotPagePair pair);

private:
    Okular::DocumentViewport calculateAnnotationViewport(AnnotPagePair pair) const;
    void doCopyAnnotationText(AnnotPagePair pair);
    void doPasteAnnotation(AnnotPagePair pair);
    void doRemovePageAnnotation(AnnotPagePair pair);
    void doOpenAnnotationWindow(AnnotPagePair pair);
    void doOpenPropertiesDialog(AnnotPagePair pair);
    void doSaveEmbeddedFile(AnnotPagePair pair);
    void doOpenEmbeddedFile(AnnotPagePair pair);
    void doAddAnnotationBookmark(AnnotPagePair pair);
    void doRemoveAnnotationBookmark(AnnotPagePair pair);

    QWidget *mParent;

    QList<AnnotPagePair> mAnnotations;
    Okular::Document *mDocument;
    MenuMode mMenuMode;
};

#endif
