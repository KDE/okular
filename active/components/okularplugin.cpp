/*
 *   Copyright 2012 by Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "okularplugin.h"

#include "documentitem.h"
#include "pageitem.h"
#include "thumbnailitem.h"

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/QDeclarativeEngine>

#include <KGlobalSettings>
#include <KLocale>

void OkularPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.okular"));
    KGlobal::locale()->insertCatalog("org.kde.okular");
    qmlRegisterType<DocumentItem>(uri, 0, 1, "DocumentItem");
    qmlRegisterType<PageItem>(uri, 0, 1, "PageItem");
    qmlRegisterType<ThumbnailItem>(uri, 0, 1, "ThumbnailItem");
}

#include "okularplugin.moc"

