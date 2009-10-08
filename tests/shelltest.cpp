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
    KUrl url( KUrl( QDir::currentPath() + "/" ), u );
    if ( !ref.isEmpty() )
        url.setRef( ref );
    url.cleanPath();
    return url;
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
    QTest::newRow( "foo.pdf" )
        << "foo.pdf"
        << true
        << makeUrlFromCwd( "foo.pdf" );
    QTest::newRow( "foo.pdf" )
        << "foo.pdf"
        << false
        << makeUrlFromCwd( "foo.pdf" );
    QTest::newRow( "foo#bar.pdf" )
        << "foo#bar.pdf"
        << false
        << makeUrlFromCwd( "foo#bar.pdf" );

    // non-local files
    QTest::newRow( "http://kde.org/foo.pdf" )
        << "http://kde.org/foo.pdf"
        << true
        << makeUrlFromCwd( "http://kde.org/foo.pdf" );
    QTest::newRow( "http://kde.org/foo#bar.pdf" )
        << "http://kde.org/foo#bar.pdf"
        << true
        << makeUrlFromCwd( "http://kde.org/foo#bar.pdf" );
}

void ShellTest::testUrlArgs()
{
    QFETCH( QString, arg );
    QFETCH( bool, exists );
    QFETCH( KUrl, resUrl );

    // note: below is a snippet taken from the Shell ctor
    arg.replace(QRegExp("^file:/{1,3}"), "/");
    KUrl url = KCmdLineArgs::makeURL(arg.toUtf8());
    int sharpPos = -1;
    if (!url.isLocalFile() || !exists)
    {
        sharpPos = arg.lastIndexOf(QLatin1Char('#'));
    }
    if (sharpPos != -1)
    {
      url = KCmdLineArgs::makeURL(arg.left(sharpPos).toUtf8());
      url.setHTMLRef(arg.mid(sharpPos + 1));
    }
    QCOMPARE( url, resUrl );
}

QTEST_KDEMAIN_CORE( ShellTest )

#include "shelltest.moc"
