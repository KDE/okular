/*
    SPDX-FileCopyrightText: 2013 Jaydeep Solanki <jaydp17@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include "../part/url_utils.h"

namespace Okular
{
class UrlDetectTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testURL();
    void testURL_data();
};

void UrlDetectTest::testURL()
{
    QFETCH(QString, selectedText);
    QFETCH(QString, url);
    QCOMPARE(UrlUtils::getUrl(selectedText), url);
}

void UrlDetectTest::testURL_data()
{
    QTest::addColumn<QString>("selectedText");
    QTest::addColumn<QString>("url");
    QTest::newRow("1") << QStringLiteral("asdfhttp://okular.org") << QString();
    QTest::newRow("2") << QStringLiteral("google.com/index.php/") << QString();
    QTest::newRow("3") << QStringLiteral("http://google.com)") << QString();

    QTest::newRow("4") << QStringLiteral("https://okular.org") << QStringLiteral("https://okular.org");
    QTest::newRow("5") << QStringLiteral("www.google.com") << QStringLiteral("http://www.google.com");
    QTest::newRow("6") << QStringLiteral("asdf http://okular.kde.org/") << QStringLiteral("http://okular.kde.org/");
    QTest::newRow("7") << QStringLiteral("http://www.example.com/wpstyle/?p=364") << QStringLiteral("http://www.example.com/wpstyle/?p=364");
    QTest::newRow("8") << QStringLiteral("asdf http://okular.org fdsa") << QStringLiteral("http://okular.org");
    QTest::newRow("9") << QStringLiteral("http://google.com/ø") << QStringLiteral("http://google.com/ø");
    QTest::newRow("10") << QStringLiteral("http://www.wolframalpha.com/input/?i=Plot[%281%2Be^%28-%282%29v%29%29^%28-2%29+%2B+%282%29+%281%2Be^v%29^%28-2%29%2C+{t%2C-0.5%2C+0.5}]")
                        << QStringLiteral("http://www.wolframalpha.com/input/?i=Plot[%281%2Be^%28-%282%29v%29%29^%28-2%29+%2B+%282%29+%281%2Be^v%29^%28-2%29%2C+{t%2C-0.5%2C+0.5}]");
    QTest::newRow("11") << QStringLiteral("http://uid:pass@example.com:8080") << QStringLiteral("http://uid:pass@example.com:8080");
    QTest::newRow("12") << QStringLiteral("www.cs.princeton.edu/~rs/talks/LLRB/LLRB.pdf") << QStringLiteral("http://www.cs.princeton.edu/~rs/talks/LLRB/LLRB.pdf");
    QTest::newRow("13") << QStringLiteral("http://IISServer/nwind?template=<ROOTxmlns:sql=\"urn:schemas-microsoft-com:xml-sql\"><sql:query>SELECTTOP2*FROM[OrderDetails]WHEREUnitPrice%26lt;10FORXMLAUTO</sql:query></ROOT>")
                        << QStringLiteral("http://IISServer/nwind?template=<ROOTxmlns:sql=\"urn:schemas-microsoft-com:xml-sql\"><sql:query>SELECTTOP2*FROM[OrderDetails]WHEREUnitPrice%26lt;10FORXMLAUTO</sql:query></ROOT>");
    QTest::newRow("14") << QStringLiteral("https://www.example.com/foo/?bar=baz&inga=42&quux") << QStringLiteral("https://www.example.com/foo/?bar=baz&inga=42&quux");
    QTest::newRow("15") << QStringLiteral("http://foo.bar/#tag") << QStringLiteral("http://foo.bar/#tag");
}

}

QTEST_MAIN(Okular::UrlDetectTest)
#include "urldetecttest.moc"
