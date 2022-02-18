/*
    SPDX-FileCopyrightText: 2004 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_THUMBNAILLIST_H_
#define _OKULAR_THUMBNAILLIST_H_

#include <QScrollArea>
#include <QToolBar>

#include "core/observer.h"

class ThumbnailListPrivate;

namespace Okular
{
class Document;
}

/**
 * @short A scrollview that displays page pixmap previews (aka thumbnails).
 *
 * ...
 */
class ThumbnailList : public QScrollArea, public Okular::DocumentObserver
{
    Q_OBJECT
public:
    ThumbnailList(QWidget *parent, Okular::Document *document);
    ~ThumbnailList() override;

    // inherited: create thumbnails ( inherited as a DocumentObserver )
    void notifySetup(const QVector<Okular::Page *> &pages, int setupFlags) override;
    // inherited: hilihght current thumbnail ( inherited as DocumentObserver )
    void notifyCurrentPageChanged(int previous, int current) override;
    // inherited: redraw thumbnail ( inherited as DocumentObserver )
    void notifyPageChanged(int pageNumber, int changedFlags) override;
    // inherited: request all visible pixmap (due to a global change or so..)
    void notifyContentsCleared(int changedFlags) override;
    // inherited: the visible areas of the page have changed
    void notifyVisibleRectsChanged() override;
    // inherited: tell if pixmap is hidden and can be unloaded
    bool canUnloadPixmap(int pageNumber) const override;

    // redraw visible widgets (useful for refreshing contents...)
    void updateWidgets();

    // show current page in Thumbnails view
    void syncThumbnail();

public Q_SLOTS:
    // these are connected to ThumbnailController buttons
    void slotFilterBookmarks(bool filterOn);

protected:
    // scroll up/down the view
    void keyPressEvent(QKeyEvent *keyEvent) override;

    // catch the viewport event and filter them if necessary
    bool viewportEvent(QEvent *) override;

Q_SIGNALS:
    void rightClick(const Okular::Page *, const QPoint);

private:
    friend class ThumbnailListPrivate;
    ThumbnailListPrivate *d;
};

/**
 * @short A vertical boxed container with zero size hint (for insertion on left toolbox)
 */
class ThumbnailsBox : public QWidget
{
    Q_OBJECT

public:
    explicit ThumbnailsBox(QWidget *parent);
    QSize sizeHint() const override;
};

/**
 * @short A toolbar that sets ThumbnailList properties when clicking on items
 *
 * This class is the small toolbar that resides in the bottom of the
 * ThumbnailsBox container (below ThumbnailList and the SearchLine) and
 * emits signals whenever a button is pressed. A click action results
 * in invoking some method (or slot) in ThumbnailList.
 */
class ThumbnailController : public QToolBar
{
    Q_OBJECT

public:
    ThumbnailController(QWidget *parent, ThumbnailList *thumbnailList);
};

#endif
