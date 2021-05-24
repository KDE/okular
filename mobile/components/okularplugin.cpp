/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
