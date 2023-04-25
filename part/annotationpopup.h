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

class QMenu;

namespace Okular
{
class Annotation;
class Document;
}

class AnnotationPopup : public QObject
{
    Q_OBJECT

public:
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

private:
    void doCopyAnnotation(AnnotPagePair pair);
    void doRemovePageAnnotation(AnnotPagePair pair);
    void doOpenAnnotationWindow(AnnotPagePair pair);
    void doOpenPropertiesDialog(AnnotPagePair pair);
    void doSaveEmbeddedFile(AnnotPagePair pair);

    QWidget *mParent;

    QList<AnnotPagePair> mAnnotations;
    Okular::Document *mDocument;
    MenuMode mMenuMode;
};

#endif
