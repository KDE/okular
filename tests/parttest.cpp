/***************************************************************************
 *   Copyright (C) 2013 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtest_kde.h>

#include "../part.h"
#include "../core/annotations.h"
#include "../core/page.h"
#include "../ui/toc.h"

#include <KConfigDialog>
#include <KStandardDirs>
#include <KTempDir>

#include <QTreeView>

namespace Okular
{
class PartTest
    : public QObject
{
    Q_OBJECT

    private slots:
        void testReload();
        void testCanceledReload();
        void testTOCReload();
        void testFowardPDF();
        void testFowardPDF_data();
        void testSaveAs();
        void testSaveAs_data();
        void testGeneratorPreferences();
};

class PartThatHijacksQueryClose : public Okular::Part
{
    public:
        PartThatHijacksQueryClose(QWidget* parentWidget, QObject* parent,
                                  const QVariantList& args, KComponentData componentData)
        : Okular::Part(parentWidget, parent, args, componentData),
          behavior(PassThru)
        {}

        enum Behavior { PassThru, ReturnTrue, ReturnFalse };

        void setQueryCloseBehavior(Behavior new_behavior)
        {
            behavior = new_behavior;
        }

        bool queryClose()
        {
             if (behavior == PassThru)
                 return Okular::Part::queryClose();
             else // ReturnTrue or ReturnFalse
                 return (behavior == ReturnTrue);
        }
    private:
        Behavior behavior;
};

// Test that Okular doesn't crash after a successful reload
void PartTest::testReload()
{
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs, KGlobal::mainComponent());
    part.openDocument(KDESRCDIR "data/file1.pdf");
    part.reload();
    qApp->processEvents();
}

// Test that Okular doesn't crash after a canceled reload
void PartTest::testCanceledReload()
{
    QVariantList dummyArgs;
    PartThatHijacksQueryClose part(NULL, NULL, dummyArgs, KGlobal::mainComponent());
    part.openDocument(KDESRCDIR "data/file1.pdf");

    // When queryClose() returns false, the reload operation is canceled (as if
    // the user had chosen Cancel in the "Save changes?" message box)
    part.setQueryCloseBehavior(PartThatHijacksQueryClose::ReturnFalse);

    part.reload();

    qApp->processEvents();
}

void PartTest::testTOCReload()
{
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs, KGlobal::mainComponent());
    part.openDocument(KDESRCDIR "data/tocreload.pdf");
    QCOMPARE(part.m_toc->expandedNodes().count(), 0);
    part.m_toc->m_treeView->expandAll();
    QCOMPARE(part.m_toc->expandedNodes().count(), 3);
    part.reload();
    qApp->processEvents();
    QCOMPARE(part.m_toc->expandedNodes().count(), 3);
}

void PartTest::testFowardPDF()
{
    QFETCH(QString, dir);

    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs, KGlobal::mainComponent());

    KTempDir tempDir(dir);
    QFile f(KDESRCDIR "data/synctextest.tex");
    const QString texDestination = tempDir.name() + "synctextest.tex";
    QVERIFY(f.copy(texDestination));

    QProcess process;
    process.setWorkingDirectory(tempDir.name());
    process.start("pdflatex", QStringList() << "-synctex=1" << "-interaction=nonstopmode" << texDestination);
    process.waitForFinished();

    const QString pdfResult = tempDir.name() + "synctextest.pdf";
    
    QVERIFY(QFile::exists(pdfResult));
    
    part.openDocument(pdfResult);
    part.m_document->setViewportPage(0);
    QCOMPARE(part.m_document->currentPage(), 0u);
    part.closeUrl();
    
    KUrl u(pdfResult);
    u.setHTMLRef("src:100" + texDestination);
    part.openUrl(u);
    QCOMPARE(part.m_document->currentPage(), 1u);
}

void PartTest::testFowardPDF_data()
{
    QTest::addColumn<QString>("dir");

    QTest::newRow("non-utf8") << QString(KGlobal::dirs()->resourceDirs("tmp")[0] + QString::fromUtf8("synctextest"));
    QTest::newRow("utf8")     << QString(KGlobal::dirs()->resourceDirs("tmp")[0] + QString::fromUtf8("ßðđđŋßðđŋ"));
}

void PartTest::testSaveAs()
{
    QFETCH(QString, file);
    QFETCH(QString, extension);
    QFETCH(bool, nativelySupportsAnnotations);

    // If we expect not to be able to preserve annotations when we write a
    // native file, disable the warning otherwise the test will wait for the
    // user to confirm. On the other end, if we expect annotations to be
    // preserved (and thus no warning), we keep the warning on so that if it
    // shows the test timeouts and we can notice that something is wrong.
    const Part::SaveAsFlags saveAsNativeFlags = nativelySupportsAnnotations ?
        Part::NoSaveAsFlags : Part::SaveAsDontShowWarning;

    QString annotName;
    QTemporaryFile archiveSave( QString( "%1/okrXXXXXX.okular" ).arg( QDir::tempPath() ) );
    QTemporaryFile nativeDirectSave( QString( "%1/okrXXXXXX.%2" ).arg( QDir::tempPath() ).arg ( extension ) );
    QTemporaryFile nativeFromArchiveFile( QString( "%1/okrXXXXXX.%2" ).arg( QDir::tempPath() ).arg ( extension ) );
    QVERIFY( archiveSave.open() ); archiveSave.close();
    QVERIFY( nativeDirectSave.open() ); nativeDirectSave.close();
    QVERIFY( nativeFromArchiveFile.open() ); nativeFromArchiveFile.close();

    qDebug() << "Open file, add annotation and save both natively and to .okular";
    {
        Okular::Part part(NULL, NULL, QVariantList(), KGlobal::mainComponent());
        part.openDocument( file );

        Okular::Annotation *annot = new Okular::TextAnnotation();
        annot->setBoundingRectangle( Okular::NormalizedRect( 0.1, 0.1, 0.15, 0.15 ) );
        annot->setContents( "annot contents" );
        part.m_document->addPageAnnotation( 0, annot );
        annotName = annot->uniqueName();

        QVERIFY( part.saveAs( KUrl( archiveSave.fileName() ), Part::SaveAsOkularArchive ) );
        QVERIFY( part.saveAs( KUrl( nativeDirectSave.fileName() ), saveAsNativeFlags ) );

        part.closeUrl();
    }

    qDebug() << "Open the .okular, check that the annotation is present and save to native";
    {
        Okular::Part part(NULL, NULL, QVariantList(), KGlobal::mainComponent());
        part.openDocument( archiveSave.fileName() );

        QVERIFY( part.m_document->page( 0 )->annotations().size() == 1 );
        QVERIFY( part.m_document->page( 0 )->annotations().first()->uniqueName() == annotName );

        QVERIFY( part.saveAs( KUrl( nativeFromArchiveFile.fileName() ), saveAsNativeFlags ) );

        part.closeUrl();
    }

    qDebug() << "Open the native file saved directly, and check that the annot"
        << "is there iff we expect it";
    {
        Okular::Part part(NULL, NULL, QVariantList(), KGlobal::mainComponent());
        part.openDocument( nativeDirectSave.fileName() );

        QCOMPARE( part.m_document->page( 0 )->annotations().size(), nativelySupportsAnnotations ? 1 : 0 );
        if ( nativelySupportsAnnotations )
            QVERIFY( part.m_document->page( 0 )->annotations().first()->uniqueName() == annotName );

        part.closeUrl();
    }

    qDebug() << "Open the native file saved from the .okular, and check that the annot"
        << "is there iff we expect it";
    {
        Okular::Part part(NULL, NULL, QVariantList(), KGlobal::mainComponent());
        part.openDocument( nativeFromArchiveFile.fileName() );

        QCOMPARE( part.m_document->page( 0 )->annotations().size(), nativelySupportsAnnotations ? 1 : 0 );
        if ( nativelySupportsAnnotations )
            QVERIFY( part.m_document->page( 0 )->annotations().first()->uniqueName() == annotName );

        part.closeUrl();
    }
}

void PartTest::testSaveAs_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<bool>("nativelySupportsAnnotations");

    QTest::newRow("pdf") << KDESRCDIR "data/file1.pdf" << "pdf" << true;
    QTest::newRow("epub") << KDESRCDIR "data/contents.epub" << "epub" << false;
}

void PartTest::testGeneratorPreferences()
{
    KConfigDialog * dialog;
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs, KGlobal::mainComponent());

    // Test that we don't crash while opening the dialog
    dialog = part.slotGeneratorPreferences();
    qApp->processEvents();
    delete dialog; // closes the dialog and recursively destroys all widgets

    // Test that we don't crash while opening a new instance of the dialog
    // This catches attempts to reuse widgets that have been destroyed
    dialog = part.slotGeneratorPreferences();
    qApp->processEvents();
    delete dialog;
}

}

int main(int argc, char *argv[])
{
    // This is QTEST_KDEMAIN withouth the LC_ALL set
    setenv("LC_ALL", "en_US.UTF-8", 1);
    assert( !QDir::homePath().isEmpty() );
    setenv("KDEHOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test") ), 1);
    setenv("XDG_DATA_HOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test/xdg/local") ), 1);
    setenv("XDG_CONFIG_HOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test/xdg/config") ), 1);
    setenv("KDE_SKIP_KDERC", "1", 1);
    unsetenv("KDE_COLOR_DEBUG");
    QFile::remove(QDir::homePath() + QString::fromLatin1("/.kde-unit-test/share/config/qttestrc")); 
    KAboutData aboutData( QByteArray("qttest"), QByteArray(), ki18n("KDE Test Program"), QByteArray("version") ); 
    KComponentData cData(&aboutData);
    QApplication app( argc, argv );
    app.setApplicationName( QLatin1String("qttest") );
    qRegisterMetaType<KUrl>(); /*as done by kapplication*/
    qRegisterMetaType<KUrl::List>();
    Okular::PartTest test;
    KGlobal::ref(); /* don't quit qeventloop after closing a mainwindow */
    return QTest::qExec( &test, argc, argv );
}

#include "parttest.moc"
