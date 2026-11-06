/*
    SPDX-FileCopyrightText: 2020 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QBuffer>
#include <QDir>
#include <QImage>
#include <QTest>

#include <KZip>

#include "core/document.h"
#include "core/generator.h"
#include "core/observer.h"
#include "core/page.h"

#include "../document.h"

#include "settings_core.h"

class ComicBookGeneratorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testRotatedImage();
    void testComicInfoBookmarks();
    void cleanupTestCase();
};

void ComicBookGeneratorTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("ComicBookGeneratorTest"));
}

void ComicBookGeneratorTest::cleanupTestCase()
{
}

void ComicBookGeneratorTest::testRotatedImage()
{
    ComicBook::Document document;
    const QString testFile = QStringLiteral(KDESRCDIR "autotests/data/rotated_cb.cbz");
    QVERIFY(document.open(testFile));

    QList<Okular::Page *> pagesVector;
    document.pages(&pagesVector);

    const Okular::Page *p = pagesVector[0];
    QVERIFY(p->height() > p->width());

    const QImage image = document.pageImage(0);
    QVERIFY(image.height() > image.width());
}

void ComicBookGeneratorTest::testComicInfoBookmarks()
{
    const QString fileName = QDir::temp().filePath(QStringLiteral("comicbooktest_bookmarks.cbz"));

    {
        KZip zip(fileName);
        QVERIFY(zip.open(QIODevice::WriteOnly));

        QImage img(10, 10, QImage::Format_ARGB32);
        img.fill(Qt::red);
        for (const char *name : {"p1.png", "p2.png", "p3.png"}) {
            QByteArray data;
            QBuffer buffer(&data);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer, "PNG");
            zip.writeFile(QString::fromLatin1(name), data);
        }

        zip.writeFile(QStringLiteral("ComicInfo.xml"),
                      QByteArrayLiteral("<?xml version=\"1.0\"?><ComicInfo><Pages>"
                                        "<Page Image=\"0\" Bookmark=\"Chapter 1\"/>"
                                        "<Page Image=\"2\" Bookmark=\"Chapter 2\"/>"
                                        "<Page Image=\"99\" Bookmark=\"Out of range\"/>"
                                        "</Pages></ComicInfo>"));
        QVERIFY(zip.close());
    }

    ComicBook::Document document;
    QVERIFY(document.open(fileName));

    QList<Okular::Page *> pagesVector;
    document.pages(&pagesVector);
    QCOMPARE(pagesVector.size(), 3);

    const std::unique_ptr<Okular::DocumentSynopsis> synopsis = document.documentSynopsis();
    QVERIFY(synopsis);
    const QVector<Okular::DocumentSynopsis::Element> bookmarks = synopsis->children();
    QCOMPARE(bookmarks.size(), 2);
    QCOMPARE(bookmarks[0].title(), QStringLiteral("Chapter 1"));
    QCOMPARE(bookmarks[0].viewPort().value_or(Okular::DocumentViewport()).pageNumber, 0);
    QCOMPARE(bookmarks[1].title(), QStringLiteral("Chapter 2"));
    QCOMPARE(bookmarks[1].viewPort().value_or(Okular::DocumentViewport()).pageNumber, 2);

    qDeleteAll(pagesVector);
    QFile::remove(fileName);
}

QTEST_MAIN(ComicBookGeneratorTest)
#include "comicbooktest.moc"

/* kate: replace-tabs on; tab-width 4; */
