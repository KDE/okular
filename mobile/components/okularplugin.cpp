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
#include "okularsingleton.h"
#include "pageitem.h"
#include "thumbnailitem.h"

#include <QApplication>
#include <QPluginLoader>
#include <QQmlEngine>

void OkularPlugin::registerTypes(const char *uri)
{
    if (!qobject_cast<QApplication *>(qApp)) {
        qWarning() << "The Okular QML components require a QApplication";
        return;
    }
    if (QString::fromLocal8Bit(uri) != QLatin1String("org.kde.okular")) {
        return;
    }
    qmlRegisterSingletonType<OkularSingleton>(uri, 2, 0, "Okular", [](QQmlEngine *, QJSEngine *) -> QObject * { return new OkularSingleton; });
    qmlRegisterType<DocumentItem>(uri, 2, 0, "DocumentItem");
    qmlRegisterType<PageItem>(uri, 2, 0, "PageItem");
    qmlRegisterType<ThumbnailItem>(uri, 2, 0, "ThumbnailItem");
}
