/***************************************************************************
 *   Copyright (C) 2013 Jaydeep Solanki <jaydp17@gmail.com>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtest_kde.h>

#include "../ui/url_utils.h"

namespace Okular
{
class UrlDetectTest : public QObject
{
    Q_OBJECT
private slots:
    void testURL();
    void testURL_data();
};

void UrlDetectTest::testURL()
{
    QFETCH( QString, selectedText );
    QFETCH( QString, url );
    QCOMPARE( UrlUtils::getUrl( selectedText ), url );
}

void UrlDetectTest::testURL_data()
{
    QTest::addColumn<QString>( "selectedText" );
    QTest::addColumn<QString>( "url" );
    QTest::newRow( "1" ) << QString( "asdfhttp://okular.org" ) << QString();
    QTest::newRow( "2" ) << QString( "google.com/index.php/" ) << QString();
    QTest::newRow( "3" ) << QString( "http://google.com)" ) << QString();

    QTest::newRow( "4" ) << QString( "https://okular.org" ) << QString( "https://okular.org" );
    QTest::newRow( "5" ) << QString( "www.google.com" ) << QString( "http://www.google.com" );
    QTest::newRow( "6" ) << QString( "asdf http://okular.kde.org/" ) << QString( "http://okular.kde.org/" );
    QTest::newRow( "7" ) << QString( "http://www.example.com/wpstyle/?p=364" ) << QString( "http://www.example.com/wpstyle/?p=364" );
    QTest::newRow( "8" ) << QString( "asdf http://okular.org fdsa" ) << QString( "http://okular.org" );
    QTest::newRow( "9" ) << QString( "http://google.com/ø" ) << QString( "http://google.com/ø" );
    QTest::newRow( "10" ) << QString( "http://www.wolframalpha.com/input/?i=Plot[%281%2Be^%28-%282%29v%29%29^%28-2%29+%2B+%282%29+%281%2Be^v%29^%28-2%29%2C+{t%2C-0.5%2C+0.5}]" ) << QString( "http://www.wolframalpha.com/input/?i=Plot[%281%2Be^%28-%282%29v%29%29^%28-2%29+%2B+%282%29+%281%2Be^v%29^%28-2%29%2C+{t%2C-0.5%2C+0.5}]" );
    QTest::newRow( "11" ) << QString( "http://uid:pass@example.com:8080" ) << QString( "http://uid:pass@example.com:8080" );
    QTest::newRow( "12" ) << QString( "www.cs.princeton.edu/~rs/talks/LLRB/LLRB.pdf" ) << QString( "http://www.cs.princeton.edu/~rs/talks/LLRB/LLRB.pdf" );
    QTest::newRow( "13" ) << QString( "http://IISServer/nwind?template=<ROOTxmlns:sql=\"urn:schemas-microsoft-com:xml-sql\"><sql:query>SELECTTOP2*FROM[OrderDetails]WHEREUnitPrice%26lt;10FORXMLAUTO</sql:query></ROOT>" ) << QString( "http://IISServer/nwind?template=<ROOTxmlns:sql=\"urn:schemas-microsoft-com:xml-sql\"><sql:query>SELECTTOP2*FROM[OrderDetails]WHEREUnitPrice%26lt;10FORXMLAUTO</sql:query></ROOT>" );
    QTest::newRow( "14" ) << QString( "https://www.example.com/foo/?bar=baz&inga=42&quux" ) << QString( "https://www.example.com/foo/?bar=baz&inga=42&quux" );
    QTest::newRow( "15" ) << QString( "http://foo.bar/#tag" ) << QString( "http://foo.bar/#tag" );
}

}

QTEST_KDEMAIN_CORE( Okular::UrlDetectTest )

#include "urldetecttest.moc"
