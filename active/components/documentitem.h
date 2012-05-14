/*
 *   Copyright 2012 by Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef QDOCUMENTITEM_H
#define QDOCUMENTITEM_H

#include <QObject>

#include "settings.h"

#include <okular/core/observer.h>

namespace Okular {
    class Document;
}

class Observer;

class DocumentItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(bool opened READ isOpened NOTIFY openedChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)

public:

    DocumentItem(QObject *parent=0);
    ~DocumentItem();

    void setPath(const QString &path);
    QString path() const;

    bool isOpened() const;

    int pageCount() const;

    //Internal, not binded to qml
    Okular::Document *document();
    Observer *observerFor(int id);

Q_SIGNALS:
    void pathChanged();
    void pageCountChanged();
    void openedChanged();

private:
    Okular::Document *m_document;
    QHash <int, Observer *> m_observers;
};

class Observer : public QObject, public Okular::DocumentObserver
{
    Q_OBJECT

public:
    Observer(DocumentItem *parent, int observerId);
    ~Observer();

    // inherited from DocumentObserver
    uint observerId() const { return m_observerId; }
    void notifyPageChanged(int page, int flags);

Q_SIGNALS:
    void pageChanged(int page, int flags);

private:
    int m_observerId;
    DocumentItem *m_document;
};

#endif
