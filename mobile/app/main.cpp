/*************************************************************************************
 *  Copyright (C) 2010 by Aleix Pol <aleixpol@kde.org>                               *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include <QApplication>

#include <KLocalizedContext>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QTimer>

#ifdef __ANDROID__
#include "android.h"

Q_DECL_EXPORT
#endif
int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("okularkirigami"));

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    // parser.setApplicationDescription(i18n("Okular mobile"));
    parser.process(app);
    QQmlApplicationEngine engine;

#ifdef __ANDROID__
    AndroidInstance::handleViewIntent();
    qmlRegisterSingletonType<QObject>("org.kde.okular.app", 2, 0, "AndroidInstance", [](QQmlEngine *, QJSEngine *) -> QObject * { return new AndroidInstance; });
    const QString uri = URIHandler::handler.m_lastUrl;
#else
    qmlRegisterSingletonType<QObject>("org.kde.okular.app", 2, 0, "AndroidInstance", [](QQmlEngine *, QJSEngine *) -> QObject * { return new QObject; });
    const QString uri = parser.positionalArguments().count() == 1 ? QUrl::fromUserInput(parser.positionalArguments().constFirst(), {}, QUrl::AssumeLocalFile).toString() : QString();
#endif
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.rootContext()->setContextProperty(QStringLiteral("uri"), uri);
    QVariantMap paths;
    paths[QStringLiteral("desktop")] = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    paths[QStringLiteral("documents")] = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    paths[QStringLiteral("music")] = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    paths[QStringLiteral("movies")] = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    paths[QStringLiteral("pictures")] = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    paths[QStringLiteral("home")] = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    engine.rootContext()->setContextProperty(QStringLiteral("userPaths"), paths);

    engine.setBaseUrl(QUrl(QStringLiteral("qrc:/package/contents/ui/")));
    engine.load(QUrl(QStringLiteral("qrc:/package/contents/ui/main.qml")));
    return app.exec();
}
