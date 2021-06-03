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

private slots:
    void testCloseDuringRotationJob();
    void testDocdataMigration();
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
    m_document->requestPixmaps(QLinkedList<Okular::PixmapRequest *>() << pixmapReq);

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

    const QUrl testFileUrl = QUrl::fromLocalFile(KDESRCDIR "data/file1.pdf");
    const QString testFilePath = testFileUrl.toLocalFile();
    const qint64 testFileSize = QFileInfo(testFilePath).size();

    // Copy XML file to the docdata/ directory
    const QString docDataPath = Okular::DocumentPrivate::docDataFileName(testFileUrl, testFileSize);
    QFile::remove(docDataPath);
    QVERIFY(QFile::copy(KDESRCDIR "data/file1-docdata.xml", docDataPath));

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

QTEST_MAIN(DocumentTest)
#include "documenttest.moc"
