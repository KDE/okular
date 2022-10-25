/*
    SPDX-FileCopyrightText: 2010 Aleix Pol <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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

#include "aboutdata.h"

#ifdef __ANDROID__
#include "android.h"

Q_DECL_EXPORT
#endif
int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("okularkirigami"));

    KLocalizedString::setApplicationDomain("org.kde.active.documentviewer");

    KAboutData aboutData = okularAboutData();
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    // parser.setApplicationDescription(i18n("Okular mobile"));
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    QQmlApplicationEngine engine;

#ifdef __ANDROID__
    AndroidInstance::handleViewIntent();
    qmlRegisterSingletonType<QObject>("org.kde.okular.app", 2, 0, "AndroidInstance", [](QQmlEngine *, QJSEngine *) -> QObject * { return new AndroidInstance; });
    const QString uri = URIHandler::handler.m_lastUrl;
#else
    qmlRegisterSingletonType<QObject>("org.kde.okular.app", 2, 0, "AndroidInstance", [](QQmlEngine *, QJSEngine *) -> QObject * { return new QObject; });
    const QString uri = parser.positionalArguments().count() == 1 ? QUrl::fromUserInput(parser.positionalArguments().constFirst(), {}, QUrl::AssumeLocalFile).toString() : QString();
#endif
    // TODO move away from context property when possible
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.rootContext()->setContextProperty(QStringLiteral("uri"), uri);
    engine.rootContext()->setContextProperty(QStringLiteral("about"), QVariant::fromValue(aboutData));
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
