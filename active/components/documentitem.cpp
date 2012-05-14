/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "documentitem.h"


#include <okular/core/document.h>


DocumentItem::DocumentItem(QObject *parent)
    : QObject(parent)
{
    Okular::Settings::instance("okularproviderrc");
    m_document = new Okular::Document(0);
}


DocumentItem::~DocumentItem()
{
    delete m_document;
}

void DocumentItem::setPath(const QString &path)
{
    //TODO: remote urls
    m_document->openDocument(path, KUrl(), KMimeType::findByUrl(KUrl(path)));

    emit pathChanged();
    emit pageCountChanged();
    emit openedChanged();
}

QString DocumentItem::path() const
{
    return m_document->currentDocument().prettyUrl();
}

bool DocumentItem::isOpened() const
{
    return m_document->isOpened();
}

int DocumentItem::pageCount() const
{
    return m_document->pages();
}

Okular::Document *DocumentItem::document()
{
    return m_document;
}

Observer *DocumentItem::observerFor(int id)
{
    if (!m_observers.contains(id)) {
        m_observers[id] = new Observer(this, id);
    }

    return m_observers.value(id);
}

//Observer

Observer::Observer(DocumentItem *parent, int id)
    : QObject(parent),
      m_observerId(id),
      m_document(parent)
{
    parent->document()->addObserver(this);
}

Observer::~Observer()
{
    m_document->document()->removeObserver(this);
}

void Observer::notifyPageChanged(int page, int flags)
{
    emit pageChanged(page, flags);
}


#include "documentitem.moc"
