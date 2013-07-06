/***************************************************************************
 *   Copyright (C) 2013 Jaydeep Solanki <jaydp17@gmail.com>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtest_kde.h>

#include "../part.h"
#include "../shell/shell.h"
#include "../core/page.h"
#include "../ui/pageview.h"
#include <QTest>
#include <QtTestGui>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QTimer>

namespace Okular
{
class EpubTest : public QObject
{
    Q_OBJECT
private slots:
    void testInternalLinks();
};

void EpubTest::testInternalLinks()
{
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs, KGlobal::mainComponent());
    part.openDocument(KDESRCDIR "data/contents.epub");
    Okular::Document *doc = part.m_document;
    QVERIFY(doc);

    QWidget *wid = part.widget();
    //PageView *wid = part.m_pageView;
    QVERIFY(wid);
//    wid->show();

    qDebug() << " width = " << wid->width();
    qDebug() << "height = " << wid->height();
    QTest::mouseClick(wid, Qt::LeftButton, 0, QPoint(90,20));
}

}

int main(int argc, char *argv[])
{
    // This is QTEST_KDEMAIN withouth the LC_ALL set
    setenv("LC_ALL", "en_US.UTF-8", 1);
    assert( !QDir::homePath().isEmpty() );
    setenv("KDEHOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test") ), 1);
    setenv("XDG_DATA_HOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test/xdg/local") ), 1);
    setenv("XDG_CONFIG_HOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test/xdg/config") ), 1);
    setenv("KDE_SKIP_KDERC", "1", 1);
    unsetenv("KDE_COLOR_DEBUG");
    QFile::remove(QDir::homePath() + QString::fromLatin1("/.kde-unit-test/share/config/qttestrc"));
    KAboutData aboutData( QByteArray("qttest"), QByteArray(), ki18n("KDE Test Program"), QByteArray("version") );
    KComponentData cData(&aboutData);
    QApplication app( argc, argv );
    app.setApplicationName( QLatin1String("qttest") );
    qRegisterMetaType<KUrl>(); /*as done by kapplication*/
    qRegisterMetaType<KUrl::List>();
    Okular::EpubTest test;
    KGlobal::ref(); /* don't quit qeventloop after closing a mainwindow */
    return QTest::qExec( &test, argc, argv );
}

#include "epubtest.moc"
