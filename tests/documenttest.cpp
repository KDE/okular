/***************************************************************************
 *   Copyright (C) 2013 by Fabio D'Urso <fabiodurso@hotmail.it>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtest_kde.h>

#include <threadweaver/ThreadWeaver.h>

#include "../core/annotations.h"
#include "../core/document.h"
#include "../core/document_p.h"
#include "../core/generator.h"
#include "../core/observer.h"
#include "../core/page.h"
#include "../core/rotationjob_p.h"
#include "../settings_core.h"

class DocumentTest
: public QObject
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
    Okular::SettingsCore::instance( "documenttest" );
    Okular::Document *m_document = new Okular::Document( 0 );
    const QString testFile = KDESRCDIR "data/file1.pdf";
    const KMimeType::Ptr mime = KMimeType::findByPath( testFile );

    Okular::DocumentObserver *dummyDocumentObserver = new Okular::DocumentObserver();
    m_document->addObserver( dummyDocumentObserver );

    m_document->openDocument( testFile, KUrl(), mime );
    m_document->setRotation( 1 );

    // Tell ThreadWeaver not to start any new job
    ThreadWeaver::Weaver::instance()->suspend();

    // Request a pixmap. A RotationJob will be enqueued but not started
    Okular::PixmapRequest *pixmapReq = new Okular::PixmapRequest(
        dummyDocumentObserver, 0, 100, 100, 1, Okular::PixmapRequest::NoFeature );
    m_document->requestPixmaps( QLinkedList<Okular::PixmapRequest*>() << pixmapReq );

    // Delete the document
    delete m_document;

    // Resume job processing and wait for the RotationJob to finish
    ThreadWeaver::Weaver::instance()->resume();
    ThreadWeaver::Weaver::instance()->finish();
    qApp->processEvents();
}

// Test that, if there's a XML file in docdata referring to a document, we
// detect that it must be migrated, that it doesn't get wiped out if you close
// the document without migrating and that it does get wiped out after migrating
void DocumentTest::testDocdataMigration()
{
    Okular::SettingsCore::instance( "documenttest" );

    const KUrl testFileUrl("file://" KDESRCDIR "data/file1.pdf");
    const QString testFilePath = testFileUrl.toLocalFile();
    const qint64 testFileSize = QFileInfo(testFilePath).size();

    // Copy XML file to the docdata/ directory
    const QString docDataPath = Okular::DocumentPrivate::docDataFileName(testFileUrl, testFileSize);
    QFile::remove(docDataPath);
    QVERIFY( QFile::copy(KDESRCDIR "data/file1-docdata.xml", docDataPath) );

    // Open our document
    Okular::Document *m_document = new Okular::Document( 0 );
    const KMimeType::Ptr mime = KMimeType::findByPath( testFilePath );
    QCOMPARE( m_document->openDocument( testFilePath, testFileUrl, mime ), Okular::Document::OpenSuccess );

    // Check that the annotation from file1-docdata.xml was loaded
    QCOMPARE( m_document->page( 0 )->annotations().size(), 1 );
    QCOMPARE( m_document->page( 0 )->annotations().first()->uniqueName(), QString("testannot") );

    // Check that we detect that it must be migrated
    QCOMPARE( m_document->isDocdataMigrationNeeded(), true );
    m_document->closeDocument();

    // Reopen the document and check that the annotation is still present
    // (because we have not migrated)
    QCOMPARE( m_document->openDocument( testFilePath, testFileUrl, mime ), Okular::Document::OpenSuccess );
    QCOMPARE( m_document->page( 0 )->annotations().size(), 1 );
    QCOMPARE( m_document->page( 0 )->annotations().first()->uniqueName(), QString("testannot") );
    QCOMPARE( m_document->isDocdataMigrationNeeded(), true );

    // Pretend the user has done the migration
    m_document->docdataMigrationDone();
    QCOMPARE( m_document->isDocdataMigrationNeeded(), false );
    m_document->closeDocument();

    // Now the docdata file should have no annotations, let's check
    QCOMPARE( m_document->openDocument( testFilePath, testFileUrl, mime ), Okular::Document::OpenSuccess );
    QCOMPARE( m_document->page( 0 )->annotations().size(), 0 );
    QCOMPARE( m_document->isDocdataMigrationNeeded(), false );
    m_document->closeDocument();
}

QTEST_KDEMAIN( DocumentTest, GUI )
#include "documenttest.moc"
