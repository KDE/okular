/*
    SPDX-FileCopyrightText: 2026 Valentin Pohu <pohuvalentin@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QFile>
#include <QMimeDatabase>
#include <QTest>

#include "../settings_core.h"
#include "core/annotations.h"
#include "core/document.h"
#include "core/movie.h"
#include "core/page.h"

class RichMediaTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testMovie_data();
    void testMovie();
};

void RichMediaTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("richmediatest"));
}

void RichMediaTest::testMovie_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>("hasMovie");
    QTest::addColumn<bool>("autoPlay");

    // Plain videos are read through the instance /Asset, which older Poppler versions do not expose
#ifdef HAVE_RICHMEDIA_INSTANCE_ASSET
    const bool plainVideo = true;
#else
    const bool plainVideo = false;
#endif

    QTest::newRow("flash, asset named by flashVars") << QStringLiteral("richmedia_flash.pdf") << true << false;
    QTest::newRow("flash without a source") << QStringLiteral("richmedia_flash_nosource.pdf") << false << false;
    QTest::newRow("plain video, no flashVars") << QStringLiteral("richmedia_video.pdf") << plainVideo << true;
    QTest::newRow("plain video after a poster") << QStringLiteral("richmedia_video_poster.pdf") << plainVideo << true;
    QTest::newRow("no type, no extension") << QStringLiteral("richmedia_video_unknown.pdf") << plainVideo << true;
    QTest::newRow("video source naming the poster") << QStringLiteral("richmedia_video_wrong_source.pdf") << plainVideo << true;
    QTest::newRow("video asset typed as an image") << QStringLiteral("richmedia_image_only.pdf") << false << false;
}

void RichMediaTest::testMovie()
{
    QFETCH(QString, file);
    QFETCH(bool, hasMovie);
    QFETCH(bool, autoPlay);

    Okular::Document document(nullptr);
    const QString testFile = QStringLiteral(KDESRCDIR "data/") + file;
    QMimeDatabase db;
    QCOMPARE(document.openDocument(testFile, QUrl(), db.mimeTypeForFile(testFile)), Okular::Document::OpenSuccess);

    // The poppler generator only creates a RichMedia annotation once it holds a movie
    const QList<Okular::Annotation *> annotations = document.page(0)->annotations();
    if (!hasMovie) {
        QVERIFY(annotations.isEmpty());
        return;
    }
    QCOMPARE(annotations.size(), 1);
    QCOMPARE(annotations.first()->subType(), Okular::Annotation::ARichMedia);

    const Okular::Movie *movie = static_cast<Okular::RichMediaAnnotation *>(annotations.first())->movie();
    QVERIFY(movie);
    QCOMPARE(movie->autoPlay(), autoPlay);

    // The movie must hold the mp4 asset and never the poster sitting next to it
    QFile played(movie->url());
    QVERIFY(played.open(QIODevice::ReadOnly));
    QCOMPARE(played.read(8).mid(4), QByteArray("ftyp"));
}

QTEST_MAIN(RichMediaTest)
#include "richmediatest.moc"
