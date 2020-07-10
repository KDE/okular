/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _SIDEBAR_H_
#define _SIDEBAR_H_

#include "okularpart_export.h"
#include <qwidget.h>

class QIcon;
class QListWidgetItem;

class OKULARPART_EXPORT Sidebar : public QWidget
{
    Q_OBJECT
public:
    explicit Sidebar(QWidget *parent = nullptr);
    ~Sidebar() override;

    int addItem(QWidget *widget, const QIcon &icon, const QString &text);

    void setMainWidget(QWidget *widget);
    void setBottomWidget(QWidget *widget);

    void setCurrentItem(QWidget *widget);
    QWidget *currentItem() const;

    void setSidebarVisibility(bool visible);
    bool isSidebarVisible() const;

    void moveSplitter(int sideWidgetSize);

Q_SIGNALS:
    void urlsDropped(const QList<QUrl> &urls);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private Q_SLOTS:
    void splitterMoved(int pos, int index);

private:
    void saveSplitterSize() const;

    // private storage
    class Private;
    Private *d;
};

#endif
