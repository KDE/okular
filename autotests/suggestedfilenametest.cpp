/*
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <QTest>

#include "../part/signaturepartutils.h"

class SuggestedFileNameTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testSuggestedSignedDocumentName();
    void testSuggestedSignedDocumentName_data();
};

void SuggestedFileNameTest::initTestCase()
{
    qputenv("LC_ALL", "en_US.UTF-8");
}

void SuggestedFileNameTest::testSuggestedSignedDocumentName()
{
    QFETCH(QString, input);
    QFETCH(QString, preferredSuffix);
    QFETCH(QString, expected);

    auto output = SignaturePartUtils::getSuggestedFileNameForSignedFile(input, preferredSuffix);
    QCOMPARE(output, expected);
}

void SuggestedFileNameTest::testSuggestedSignedDocumentName_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("preferredSuffix"); // normally derived from mimetype of document
    QTest::addColumn<QString>("expected");

    QTest::newRow("simple") << QStringLiteral("foo.pdf") << QStringLiteral("pdf") << QStringLiteral("foo_signed.pdf");
    QTest::newRow("double extensions") << QStringLiteral("foo.pdf.gz") << QStringLiteral("pdf") << QStringLiteral("foo_signed.pdf"); // while we might read compressed files, we don't write  them out
    QTest::newRow("versioning") << QStringLiteral("foo-1.2.3.pdf") << QStringLiteral("pdf") << QStringLiteral("foo-1.2.3_signed.pdf");
    QTest::newRow("versioned and double extensions") << QStringLiteral("foo-1.2.3.pdf.gz") << QStringLiteral("pdf") << QStringLiteral("foo-1.2.3_signed.pdf");
    QTest::newRow("gif") << QStringLiteral("foo.gif") << QStringLiteral("pdf") << QStringLiteral("foo_signed.pdf");
    QTest::newRow("version gif") << QStringLiteral("foo-1.2.3.gif") << QStringLiteral("pdf") << QStringLiteral("foo-1.2.3_signed.pdf");
    QTest::newRow("no extension") << QStringLiteral("foo") << QStringLiteral("pdf") << QStringLiteral("foo_signed.pdf");
    QTest::newRow("no extension with versions") << QStringLiteral("foo-1.2.3") << QStringLiteral("pdf") << QStringLiteral("foo-1.2_signed.pdf"); // This is not as such expected behavior but more a documentation of implementation.
}

QTEST_GUILESS_MAIN(SuggestedFileNameTest)

#include "suggestedfilenametest.moc"
