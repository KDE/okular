/***************************************************************************
 *   Copyright (C) 2009 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtest_kde.h>
#include <qdir.h>
#include <kurl.h>

#include "../shell/shellutils.h"

namespace QTest
{
template<>
char* toString( const KUrl& url )
{
    return qstrdup( url.url().toLocal8Bit() );
}
}

static const KUrl makeUrlFromCwd( const QString& u, const QString& ref = QString() )
{
    KUrl url( KUrl( QDir::currentPath() + '/' ), u );
    if ( !ref.isEmpty() )
        url.setRef( ref );
    url.cleanPath();
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
    qRegisterMetaType<KUrl>();

    KCmdLineArgs::setCwd( QDir::currentPath().toLocal8Bit() );
}

void ShellTest::testUrlArgs_data()
{
    QTest::addColumn<QString>( "arg" );
    QTest::addColumn<bool>( "exists" );
    QTest::addColumn<KUrl>( "resUrl" );

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
        << KUrl( "file:///tmp/file%20with%20spaces.pdf" );

    // non-local files
    QTest::newRow( "http://kde.org/foo.pdf" )
        << "http://kde.org/foo.pdf"
        << true
        << makeUrlFromCwd( "http://kde.org/foo.pdf" );
    QTest::newRow( "http://kde.org/foo#bar.pdf" )
        << "http://kde.org/foo#bar.pdf"
        << true
        << makeUrlFromCwd( "http://kde.org/foo#bar.pdf" );
    QTest::newRow( "http://kde.org/foo.pdf#anchor" )
        << "http://kde.org/foo.pdf#anchor"
        << true
        << makeUrlFromCwd( "http://kde.org/foo.pdf", "anchor" );
    QTest::newRow( "#207461" )
        << "http://homepages.inf.ed.ac.uk/mef/file%20with%20spaces.pdf"
        << true
        << KUrl( "http://homepages.inf.ed.ac.uk/mef/file%20with%20spaces.pdf" );
}

void ShellTest::testUrlArgs()
{
    QFETCH( QString, arg );
    QFETCH( bool, exists );
    QFETCH( KUrl, resUrl );

    KUrl url = ShellUtils::urlFromArg( arg, exists ? fileExist_always_Func : fileExist_never_Func );
    QCOMPARE( url, resUrl );
}

QTEST_KDEMAIN_CORE( ShellTest )

#include "shelltest.moc"
