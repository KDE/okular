/*
    SPDX-FileCopyrightText: 2013 Fabio D 'Urso <fabiodurso@hotmail.it>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QMimeDatabase>
#include <QTemporaryFile>
#include <QTest>

#include <threadweaver/queue.h>

#include "../core/annotations.h"
#include "../core/document.h"
#include "../core/document_p.h"
#include "../core/generator.h"
#include "../core/observer.h"
#include "../core/page.h"
#include "../core/rotationjob_p.h"
#include "../settings_core.h"

class DocumentTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCloseDuringRotationJob();
    void testDocdataMigration();
    void testEvaluateKeystrokeEventChange_data();
    void testEvaluateKeystrokeEventChange();
};

// Test that we don't crash if the document is closed while a RotationJob
// is enqueued/running
void DocumentTest::testCloseDuringRotationJob()
{
    Okular::SettingsCore::instance(QStringLiteral("documenttest"));
    Okular::Document *m_document = new Okular::Document(nullptr);
    const QString testFile = QStringLiteral(KDESRCDIR "data/file1.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);

    Okular::DocumentObserver *dummyDocumentObserver = new Okular::DocumentObserver();
    m_document->addObserver(dummyDocumentObserver);

    m_document->openDocument(testFile, QUrl(), mime);
    m_document->setRotation(1);

    // Tell ThreadWeaver not to start any new job
    ThreadWeaver::Queue::instance()->suspend();

    // Request a pixmap. A RotationJob will be enqueued but not started
    Okular::PixmapRequest *pixmapReq = new Okular::PixmapRequest(dummyDocumentObserver, 0, 100, 100, qApp->devicePixelRatio(), 1, Okular::PixmapRequest::NoFeature);
    m_document->requestPixmaps({pixmapReq});

    // Delete the document
    delete m_document;

    // Resume job processing and wait for the RotationJob to finish
    ThreadWeaver::Queue::instance()->resume();
    ThreadWeaver::Queue::instance()->finish();
    qApp->processEvents();

    delete dummyDocumentObserver;
}

// Test that, if there's a XML file in docdata referring to a document, we
// detect that it must be migrated, that it doesn't get wiped out if you close
// the document without migrating and that it does get wiped out after migrating
void DocumentTest::testDocdataMigration()
{
    Okular::SettingsCore::instance(QStringLiteral("documenttest"));

    const QUrl testFileUrl = QUrl::fromLocalFile(QStringLiteral(KDESRCDIR "data/file1.pdf"));
    const QString testFilePath = testFileUrl.toLocalFile();
    const qint64 testFileSize = QFileInfo(testFilePath).size();

    // Copy XML file to the docdata/ directory
    const QString docDataPath = Okular::DocumentPrivate::docDataFileName(testFileUrl, testFileSize);
    QFile::remove(docDataPath);
    QVERIFY(QFile::copy(QStringLiteral(KDESRCDIR "data/file1-docdata.xml"), docDataPath));

    // Open our document
    Okular::Document *m_document = new Okular::Document(nullptr);
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFilePath);
    QCOMPARE(m_document->openDocument(testFilePath, testFileUrl, mime), Okular::Document::OpenSuccess);

    // Check that the annotation from file1-docdata.xml was loaded
    QCOMPARE(m_document->page(0)->annotations().size(), 1);
    QCOMPARE(m_document->page(0)->annotations().first()->uniqueName(), QStringLiteral("testannot"));

    // Check that we detect that it must be migrated
    QVERIFY(m_document->isDocdataMigrationNeeded());
    m_document->closeDocument();

    // Reopen the document and check that the annotation is still present
    // (because we have not migrated)
    QCOMPARE(m_document->openDocument(testFilePath, testFileUrl, mime), Okular::Document::OpenSuccess);
    QCOMPARE(m_document->page(0)->annotations().size(), 1);
    QCOMPARE(m_document->page(0)->annotations().first()->uniqueName(), QStringLiteral("testannot"));
    QVERIFY(m_document->isDocdataMigrationNeeded());

    // Do the migration
    QTemporaryFile migratedSaveFile(QStringLiteral("%1/okrXXXXXX.pdf").arg(QDir::tempPath()));
    QVERIFY(migratedSaveFile.open());
    migratedSaveFile.close();
    QVERIFY(m_document->saveChanges(migratedSaveFile.fileName()));
    m_document->docdataMigrationDone();
    QVERIFY(!m_document->isDocdataMigrationNeeded());
    m_document->closeDocument();

    // Now the docdata file should have no annotations, let's check
    QCOMPARE(m_document->openDocument(testFilePath, testFileUrl, mime), Okular::Document::OpenSuccess);
    QCOMPARE(m_document->page(0)->annotations().size(), 0);
    QVERIFY(!m_document->isDocdataMigrationNeeded());
    m_document->closeDocument();

    // And the new file should have 1 annotation, let's check
    QCOMPARE(m_document->openDocument(migratedSaveFile.fileName(), QUrl::fromLocalFile(migratedSaveFile.fileName()), mime), Okular::Document::OpenSuccess);
    QCOMPARE(m_document->page(0)->annotations().size(), 1);
    QVERIFY(!m_document->isDocdataMigrationNeeded());
    m_document->closeDocument();

    delete m_document;
}

void DocumentTest::testEvaluateKeystrokeEventChange_data()
{
    QTest::addColumn<QString>("oldVal");
    QTest::addColumn<QString>("newVal");
    QTest::addColumn<int>("selStart");
    QTest::addColumn<int>("selEnd");
    QTest::addColumn<QString>("expectedDiff");

    QTest::addRow("empty") << ""
                           << "" << 0 << 0 << "";
    QTest::addRow("a") << ""
                       << "a" << 0 << 0 << "a";
    QTest::addRow("ab") << "a"
                        << "b" << 0 << 1 << "b";
    QTest::addRow("ab2") << "a"
                         << "ab" << 1 << 1 << "b";
    QTest::addRow("kaesekuchen") << "Käse"
                                 << "Käsekuchen" << 4 << 4 << "kuchen";
    QTest::addRow("replace") << "kuchen"
                             << "wurst" << 0 << 6 << "wurst";
    QTest::addRow("okular") << "Oku"
                            << "Okular" << 3 << 3 << "lar";
    QTest::addRow("okular2") << "Oku"
                             << "Okular" << 0 << 3 << "Okular";
    QTest::addRow("removal1") << "a"
                              << "" << 0 << 1 << "";
    QTest::addRow("removal2") << "ab"
                              << "a" << 1 << 2 << "";
    QTest::addRow("overlapping chang") << "abcd"
                                       << "abclmnopd" << 1 << 3 << "bclmnop";
    QTest::addRow("unicode") << "☮🤌"
                             << "☮🤌❤️" << 2 << 2 << "❤️";
    QTest::addRow("unicode2") << "☮"
                              << "☮🤌❤️" << 1 << 1 << "🤌❤️";
    QTest::addRow("unicode3") << "🤍"
                              << "🤌" << 0 << 1 << "🤌";
}

void DocumentTest::testEvaluateKeystrokeEventChange()
{
    QFETCH(QString, oldVal);
    QFETCH(QString, newVal);
    QFETCH(int, selStart);
    QFETCH(int, selEnd);
    QFETCH(QString, expectedDiff);

    QCOMPARE(Okular::DocumentPrivate::evaluateKeystrokeEventChange(oldVal, newVal, selStart, selEnd), expectedDiff);
}

QTEST_MAIN(DocumentTest)
#include "documenttest.moc"
