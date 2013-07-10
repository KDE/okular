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
#include "../core/page.h"

namespace Okular
{
class EpubTest : public QObject
{
    Q_OBJECT
private slots:
    void testInternalLinks_data();
    void testInternalLinks();
};

void EpubTest::testInternalLinks()
{
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs, KGlobal::mainComponent());
    part.openDocument(KDESRCDIR "data/contents.epub");
    Okular::Document *doc = part.m_document;
    QVERIFY(doc);

    int width = doc->page( 0 )->width();  // 0 because current page is zero
    int height = doc->page( 0 )->height();
    QFETCH( double, yCords );
    QFETCH( int , pagesNos );
    const ObjectRect *rect = doc->page( 0 )->objectRect( ObjectRect::Action, 0.0366667, yCords, width, height);
    QVERIFY( rect );
    const Okular::Action * action = static_cast< const Okular::Action * >( rect->object() );
    doc->processAction( action );
    QCOMPARE( doc->currentPage(), ( uint ) pagesNos );
    part.slotGotoFirst();
}


void EpubTest::testInternalLinks_data()
{
    QTest::addColumn< double >( "yCords" );
    QTest::addColumn< int >( "pagesNos" );
    QTest::newRow( "0.025" ) << 0.025 << 1;
    QTest::newRow( "0.06" ) << 0.06 << 2;
    QTest::newRow( "0.095" ) << 0.095 << 1;
    QTest::newRow( "0.13" ) << 0.13 << 1;
    QTest::newRow( "0.165" ) << 0.165 << 2;
    QTest::newRow( "0.2" ) << 0.2 << 1;
    QTest::newRow( "0.235" ) << 0.235 << 2;
    QTest::newRow( "0.27" ) << 0.27 << 2;
    QTest::newRow( "0.305" ) << 0.305 << 2;
    QTest::newRow( "0.34" ) << 0.34 << 2;
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
