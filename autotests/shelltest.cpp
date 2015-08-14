/***************************************************************************
 *   Copyright (C) 2009 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

#include <QDir>
#include <QUrl>

#include "../shell/shellutils.h"

static const QUrl makeUrlFromCwd( const QString& u, const QString& ref = QString() )
{
    QUrl url = QUrl::fromLocalFile( QDir::currentPath() + '/' + u) ;
    if ( !ref.isEmpty() )
        url.setFragment( ref );
    url.setPath(QDir::cleanPath( url.path() ) );
    return url;
}

static bool fileExist_always_Func( const QString& )
{
    return true;
}

static bool fileExist_never_Func( const QString& )
{
    return false;
}

class ShellTest
    : public QObject
{
    Q_OBJECT

    private slots:
        void initTestCase();
        void testUrlArgs_data();
        void testUrlArgs();
};

void ShellTest::initTestCase()
{
    qRegisterMetaType<QUrl>();
}

void ShellTest::testUrlArgs_data()
{
    QTest::addColumn<QString>( "arg" );
    QTest::addColumn<bool>( "exists" );
    QTest::addColumn<QUrl>( "resUrl" );

    // local files
    QTest::newRow( "foo.pdf, exist" )
        << "foo.pdf"
        << true
        << makeUrlFromCwd( "foo.pdf" );
    QTest::newRow( "foo.pdf, !exist" )
        << "foo.pdf"
        << false
        << makeUrlFromCwd( "foo.pdf" );
    QTest::newRow( "foo#bar.pdf, !exist" )
        << "foo#bar.pdf"
        << false
        << makeUrlFromCwd( "foo#bar.pdf" );
    QTest::newRow( "foo.pdf#anchor, !exist" )
        << "foo.pdf#anchor"
        << false
        << makeUrlFromCwd( "foo.pdf", "anchor" );
    QTest::newRow( "#207461" )
        << "file:///tmp/file%20with%20spaces.pdf"
        << true
        << QUrl( "file:///tmp/file%20with%20spaces.pdf" );

    // non-local files
    QTest::newRow( "http://kde.org/foo.pdf" )
        << "http://kde.org/foo.pdf"
        << true
        << QUrl( "http://kde.org/foo.pdf" );
    // make sure we don't have a fragment
    QUrl hashInName( "http://kde.org" );
    QVERIFY( hashInName.path().isEmpty() );
    hashInName.setPath( "/foo#bar.pdf" );
    QVERIFY( hashInName.fragment().isEmpty() );
    QTest::newRow( "http://kde.org/foo#bar.pdf" )
        << "http://kde.org/foo#bar.pdf"
        << true
        << hashInName;
    QUrl withAnchor( "http://kde.org/foo.pdf" );
    withAnchor.setFragment( "anchor" );
    QTest::newRow( "http://kde.org/foo.pdf#anchor" )
        << "http://kde.org/foo.pdf#anchor"
        << true
        << withAnchor;
    QTest::newRow( "#207461" )
        << "http://homepages.inf.ed.ac.uk/mef/file%20with%20spaces.pdf"
        << true
        << QUrl( "http://homepages.inf.ed.ac.uk/mef/file%20with%20spaces.pdf" );
    QUrl openOnPage3 = QUrl( "http://itzsimpl.info/lectures/CG/L2-transformations.pdf" );
    openOnPage3.setFragment( "3" );
    QTest::newRow( "RR124738" )
        << "http://itzsimpl.info/lectures/CG/L2-transformations.pdf#3"
        << true
        << openOnPage3;
}

void ShellTest::testUrlArgs()
{
    QFETCH( QString, arg );
    QFETCH( bool, exists );
    QFETCH( QUrl, resUrl );
    qDebug() << "Expected url:" << resUrl << "path =" <<  resUrl.path() << "fragment =" << resUrl.fragment();
    QUrl url = ShellUtils::urlFromArg( arg, exists ? fileExist_always_Func : fileExist_never_Func );
    QCOMPARE( url, resUrl );
}

QTEST_GUILESS_MAIN( ShellTest )
#include "shelltest.moc"
