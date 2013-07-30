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

#include "../core/document.h"
#include "../core/generator.h"
#include "../core/observer.h"
#include "../core/rotationjob_p.h"
#include "../settings_core.h"

class DocumentTest
: public QObject
{
    Q_OBJECT

    private slots:
        void testCloseDuringRotationJob();
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

QTEST_KDEMAIN( DocumentTest, GUI )
#include "documenttest.moc"
