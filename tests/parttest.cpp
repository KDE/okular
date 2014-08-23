/***************************************************************************
 *   Copyright (C) 2013 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

#include "../part.h"
#include "../ui/toc.h"

#include <KConfigDialog>
#include <KGlobal>
#include <KStandardDirs>
#include <KTempDir>
#include <K4AboutData>

#include <QTreeView>

namespace Okular
{
class PartTest
    : public QObject
{
    Q_OBJECT

        static bool openDocument(Okular::Part *part, const QString &filePath);

    private slots:
        void testReload();
        void testCanceledReload();
        void testTOCReload();
        void testFowardPDF();
        void testFowardPDF_data();
        void testGeneratorPreferences();
};

class PartThatHijacksQueryClose : public Okular::Part
{
    public:
        PartThatHijacksQueryClose(QWidget* parentWidget, QObject* parent,
                                  const QVariantList& args, KComponentData componentData)
        : Okular::Part(parentWidget, parent, args),
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

bool PartTest::openDocument(Okular::Part *part, const QString &filePath)
{
    part->openDocument( filePath );
    return part->m_document->isOpened();
}

// Test that Okular doesn't crash after a successful reload
void PartTest::testReload()
{
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs);
    QVERIFY( openDocument(&part, KDESRCDIR "data/file1.pdf") );
    part.reload();
    qApp->processEvents();
}

// Test that Okular doesn't crash after a canceled reload
void PartTest::testCanceledReload()
{
    QVariantList dummyArgs;
    PartThatHijacksQueryClose part(NULL, NULL, dummyArgs, KGlobal::mainComponent());
    QVERIFY( openDocument(&part, KDESRCDIR "data/file1.pdf") );

    // When queryClose() returns false, the reload operation is canceled (as if
    // the user had chosen Cancel in the "Save changes?" message box)
    part.setQueryCloseBehavior(PartThatHijacksQueryClose::ReturnFalse);

    part.reload();

    qApp->processEvents();
}

void PartTest::testTOCReload()
{
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs);
    QVERIFY( openDocument(&part, KDESRCDIR "data/tocreload.pdf") );
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
    Okular::Part part(NULL, NULL, dummyArgs);

    // Create temp dir named like this: ${system temp dir}/${random string}/${dir}
    const QTemporaryDir tempDir;
    const QDir workDir(QDir(tempDir.path()).filePath(dir));
    workDir.mkpath(".");

    QFile f(KDESRCDIR "data/synctextest.tex");
    const QString texDestination = workDir.path() + "/synctextest.tex";
    QVERIFY(f.copy(texDestination));
    QProcess process;
    process.setWorkingDirectory(workDir.path());
    process.start("pdflatex", QStringList() << "-synctex=1" << "-interaction=nonstopmode" << texDestination);
    process.waitForFinished();

    const QString pdfResult = workDir.path() + "/synctextest.pdf";
    
    QVERIFY(QFile::exists(pdfResult));
    
    QVERIFY( openDocument(&part, pdfResult) );
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

    QTest::newRow("non-utf8") << QString::fromUtf8("synctextest");
    QTest::newRow("utf8")     << QString::fromUtf8("ßðđđŋßðđŋ");
}

void PartTest::testGeneratorPreferences()
{
    KConfigDialog * dialog;
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs);

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
#pragma message("KF5 What to do with this?")
    // This is QTEST_KDEMAIN withouth the LC_ALL set
    setenv("LC_ALL", "en_US.UTF-8", 1);
    Q_ASSERT( !QDir::homePath().isEmpty() );
    setenv("KDEHOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test") ).constData(), 1);
    setenv("XDG_DATA_HOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test/xdg/local") ).constData(), 1);
    setenv("XDG_CONFIG_HOME", QFile::encodeName( QDir::homePath() + QString::fromLatin1("/.kde-unit-test/xdg/config") ).constData(), 1);
    setenv("KDE_SKIP_KDERC", "1", 1);
    unsetenv("KDE_COLOR_DEBUG");
    QFile::remove(QDir::homePath() + QString::fromLatin1("/.kde-unit-test/share/config/qttestrc")); 
    K4AboutData aboutData( QByteArray("qttest"), QByteArray(), ki18n("KDE Test Program"), QByteArray("version") );
    KComponentData cData(&aboutData);
    QApplication app( argc, argv );
    app.setApplicationName( QLatin1String("qttest") );
    qRegisterMetaType<KUrl>(); /*as done by kapplication*/
    qRegisterMetaType<QList<QUrl>>();
    Okular::PartTest test;
    KGlobal::ref(); /* don't quit qeventloop after closing a mainwindow */
    return QTest::qExec( &test, argc, argv );
}

#include "parttest.moc"
