/***************************************************************************
 *   Copyright (C) 2014 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtest_kde.h>
#include <qprintdialog.h>
#include <qwidget.h>
#include <ktabwidget.h>

#include "../shell/okular_main.h"
#include "../shell/shell.h"
#include "../shell/shellutils.h"
#include "../core/document_p.h"
#include "../ui/presentationwidget.h"
#include "../part.h"
#include "../settings.h"

#include <sys/types.h>
#include <unistd.h>

namespace Okular {
class PartTest
{
public:
    Okular::Document *partDocument(Okular::Part *part) const {
        return part->m_document;
    }
    QWidget *presentationWidget(Okular::Part *part) const {
        return part->m_presentationWidget;
    }
};
}

class ClosePrintDialogHelper : public QObject
{
    Q_OBJECT

public:
    ClosePrintDialogHelper(int expectedTab) : foundDialog(false), m_expectedTab(expectedTab) { }
    bool foundDialog;

private slots:
    void closePrintDialog();

private:
    int m_expectedTab;
};

class MainShellTest : public QObject, public Okular::PartTest
{
    Q_OBJECT

public:
    static KTabWidget* tabWidget(Shell *s)
    {
        return s->m_tabWidget;
    }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testShell_data();
    void testShell();
    void testUnique2FilesError();

private:
};

Shell *findShell(Shell *ignore = 0)
{
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        Shell *s = qobject_cast<Shell*>(widget);
        if (s && s != ignore)
            return s;
    }
    return 0;
}

void MainShellTest::initTestCase()
{
    // Don't pollute people's okular settings
    Okular::Settings::instance( "mainshelltest" );

    // Register in bus as okular
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    const QString myPid = QString::number( getpid() );
    const QString serviceName = "org.kde.okular-"+ myPid;
    QVERIFY( bus->registerService(serviceName) == QDBusConnectionInterface::ServiceRegistered );

    // Tell the presentationWidget to not be annoying
    KSharedConfigPtr c = KGlobal::config();
    KConfigGroup cg = c->group("Notification Messages");
    cg.writeEntry("presentationInfo", false);
}

void MainShellTest::cleanupTestCase()
{
}

void MainShellTest::init()
{
    // Default settings for every test
    Okular::Settings::self()->setDefaults();

    // Clean docdatas
    QList<KUrl> urls;
    urls << KUrl("file://" KDESRCDIR "data/file1.pdf");
    urls << KUrl("file://" KDESRCDIR "data/tocreload.pdf");
    urls << KUrl("file://" KDESRCDIR "data/contents.epub");

    foreach (const KUrl &url, urls)
    {
        QFileInfo fileReadTest( url.toLocalFile() );
        const QString docDataPath = Okular::DocumentPrivate::docDataFileName(url, fileReadTest.size());
        QFile::remove(docDataPath);
    }
}

void MainShellTest::cleanup()
{
    Shell *s;
    while ((s = findShell()))
    {
        delete s;
    }
}

void MainShellTest::testShell_data()
{
    QTest::addColumn<QStringList>("paths");
    QTest::addColumn<QString>("serializedOptions");
    QTest::addColumn<bool>("useTabs");
    QTest::addColumn<QString>("externalProcessPath");
    QTest::addColumn<uint>("expectedPage");
    QTest::addColumn<bool>("expectPresentation");
    QTest::addColumn<bool>("expectPrintDialog");
    QTest::addColumn<bool>("unique");
    QTest::addColumn<uint>("externalProcessExpectedPage");
    QTest::addColumn<bool>("externalProcessExpectPresentation");
    QTest::addColumn<bool>("externalProcessExpectPrintDialog");

    const QStringList contentsEpub = QStringList(KDESRCDIR "data/contents.epub");
    const QStringList file1 = QStringList(KDESRCDIR "data/file1.pdf");
    QStringList file1AndToc;
    file1AndToc << KDESRCDIR "data/file1.pdf";
    file1AndToc << KDESRCDIR "data/tocreload.pdf";
    const QString tocReload = KDESRCDIR "data/tocreload.pdf";

    const QString optionsPage2 = ShellUtils::serializeOptions(false, false, false, false, "2");
    const QString optionsPage2Presentation = ShellUtils::serializeOptions(true, false, false, false, "2");
    const QString optionsPrint = ShellUtils::serializeOptions(false, true, false, false, QString());
    const QString optionsUnique = ShellUtils::serializeOptions(false, false, true, false, QString());

    QTest::newRow("just show shell") << QStringList() << QString() << false << QString() << 0u << false << false << false << 0u << false << false;
    QTest::newRow("open file") << file1 << QString() << false << QString() << 0u << false << false << false << 0u << false << false;
    QTest::newRow("two files no tabs") << file1AndToc << QString() << false << QString() << 0u << false << false << false << 0u << false << false;
    QTest::newRow("two files with tabs") << file1AndToc << QString() << true << QString() << 0u << false << false << false << 0u << false << false;
    QTest::newRow("two files sequence no tabs") << file1 << QString() << false << tocReload << 0u << false << false << false << 0u << false << false;
    QTest::newRow("two files sequence with tabs") << file1 << QString() << true << tocReload << 0u << false << false << false << 0u << false << false;
    QTest::newRow("open file page number") << contentsEpub << optionsPage2 << false << QString() << 1u << false << false << false << 0u << false << false;
    QTest::newRow("open file page number and presentation") << contentsEpub << optionsPage2Presentation << false << QString() << 1u << true << false << false << 0u << false << false;
    QTest::newRow("open file print") << file1 << optionsPrint << false << QString() << 0u << false << true << false << 0u << false << false;
    QTest::newRow("open two files unique") << file1 << optionsUnique << false << tocReload << 0u << false << false << true << 0u << false << false;
    QTest::newRow("open two files unique tabs") << file1 << optionsUnique << true << tocReload << 0u << false << false << true << 0u << false << false;
    QTest::newRow("page number attach tabs") << file1 << QString() << true << contentsEpub[0] << 0u << false << false << false << 2u << false << false;
    QTest::newRow("presentation attach tabs") << file1 << QString() << true << contentsEpub[0] << 0u << false << false << false << 2u << true << false;
    QTest::newRow("print attach tabs") << file1 << QString() << true << contentsEpub[0] << 0u << false << true << false << 2u << false << true;
    QTest::newRow("page number attach unique") << file1 << optionsUnique << false << contentsEpub[0] << 0u << false << false << true << 3u << false << false;
    QTest::newRow("presentation attach unique") << file1 << optionsUnique << false << contentsEpub[0] << 0u << false << false << true << 2u << true << false;
    QTest::newRow("print attach unique") << file1 << optionsUnique << false << contentsEpub[0] << 0u << false << false << true << 2u << false << true;
    QTest::newRow("page number attach unique tabs") << file1 << optionsUnique << true << contentsEpub[0] << 0u << false << false << true << 3u << false << false;
    QTest::newRow("presentation attach unique tabs") << file1 << optionsUnique << true << contentsEpub[0] << 0u << false << false << true << 2u << true << false;
    QTest::newRow("print attach unique tabs") << file1 << optionsUnique << true << contentsEpub[0] << 0u << false << false << true << 2u << false << true;
}

void MainShellTest::testShell()
{
    QFETCH(QStringList, paths);
    QFETCH(QString, serializedOptions);
    QFETCH(bool, useTabs);
    QFETCH(QString, externalProcessPath);
    QFETCH(uint, expectedPage);
    QFETCH(bool, expectPresentation);
    QFETCH(bool, expectPrintDialog);
    QFETCH(bool, unique);
    QFETCH(uint, externalProcessExpectedPage);
    QFETCH(bool, externalProcessExpectPresentation);
    QFETCH(bool, externalProcessExpectPrintDialog);

    QScopedPointer<ClosePrintDialogHelper> helper;

    Okular::Settings::self()->setShellOpenFileInTabs(useTabs);

    if (expectPrintDialog || externalProcessExpectPrintDialog) {
        const int expectedTab = externalProcessExpectPrintDialog && !unique ? 1 : 0;
        helper.reset(new ClosePrintDialogHelper(expectedTab));
        QTimer::singleShot(0, helper.data(), SLOT(closePrintDialog()));
    }

    Okular::Status status = Okular::main(paths, serializedOptions);
    QCOMPARE(status, Okular::Success);
    Shell *s = findShell();
    QVERIFY(s);

    if (paths.count() == 1)
    {
        QCOMPARE(s->m_tabs.count(), 1);
        Okular::Part *part = s->findChild<Okular::Part*>();
        QVERIFY(part);
        QCOMPARE(part->url().url(), QString("file://%1").arg(paths[0]));
        QCOMPARE(partDocument(part)->currentPage(), expectedPage);
    }
    else if (paths.count() == 2)
    {
        if (useTabs)
        {
            QSet<QString> openUrls;
            Shell *s = findShell();
            QVERIFY(s);
            Okular::Part *part = dynamic_cast<Okular::Part*>(s->m_tabs[0].part);
            Okular::Part *part2 = dynamic_cast<Okular::Part*>(s->m_tabs[1].part);
            QCOMPARE(s->m_tabs.count(), 2);
            QCOMPARE(part->url().url(), QString("file://%1").arg(paths[0]));
            QCOMPARE(part2->url().url(), QString("file://%1").arg(paths[1]));
            QCOMPARE(partDocument(part)->currentPage(), expectedPage);
            QCOMPARE(partDocument(part2)->currentPage(), expectedPage);
        }
        else
        {
            QSet<QString> openUrls;
            Shell *s = findShell();
            QVERIFY(s);
            QCOMPARE(s->m_tabs.count(), 1);
            Okular::Part *part = s->findChild<Okular::Part*>();
            QVERIFY(part);
            QCOMPARE(partDocument(part)->currentPage(), expectedPage);
            openUrls << part->url().url();

            Shell *s2 = findShell(s);
            QVERIFY(s2);
            QCOMPARE(s2->m_tabs.count(), 1);
            Okular::Part *part2 = s2->findChild<Okular::Part*>();
            QVERIFY(part2);
            QCOMPARE(partDocument(part2)->currentPage(), expectedPage);
            openUrls << part2->url().url();

            foreach(const QString &path, paths)
            {
                QVERIFY(openUrls.contains(QString("file://%1").arg(path)));
            }
        }
    }

    if (!externalProcessPath.isEmpty())
    {
        Okular::Part *part = s->findChild<Okular::Part*>();

        QProcess p;
        QString command = "okular " + externalProcessPath;
        if (unique)
            command += " -unique";
        if (externalProcessExpectedPage != 0)
            command += QString(" -page %1").arg(externalProcessExpectedPage + 1);
        if (externalProcessExpectPresentation)
            command += QString(" -presentation");
        if (externalProcessExpectPrintDialog)
            command += QString(" -print");
        p.start(command);
        p.waitForStarted();
        QCOMPARE(p.state(), QProcess::Running);

        if (useTabs || unique)
        {
            // It is attaching to us, so will eventually stop
            for (int i = 0; p.state() != QProcess::NotRunning && i < 20; ++i) {
                QTest::qWait(100);
            }
            QCOMPARE(p.state(), QProcess::NotRunning);
            QCOMPARE(p.exitStatus(), QProcess::NormalExit);
            QCOMPARE(p.exitCode(), 0);

            if (unique)
            {
                // It is unique so part got "overriten"
                QCOMPARE(s->m_tabs.count(), 1);
                QCOMPARE(part->url().url(), QString("file://%1").arg(externalProcessPath));
                QCOMPARE(partDocument(part)->currentPage(), externalProcessExpectedPage);
            }
            else
            {
                // It is attaching to us so a second tab is there
                QCOMPARE(s->m_tabs.count(), 2);
                Okular::Part *part2 = dynamic_cast<Okular::Part*>(s->m_tabs[1].part);
                QCOMPARE(part2->url().url(), QString("file://%1").arg(externalProcessPath));
                QCOMPARE(partDocument(part2)->currentPage(), externalProcessExpectedPage);
            }
        }
        else
        {
            QTest::qWait(750);

            // It opened on a new process, so it is still running, we need to kill it
            QCOMPARE(p.state(), QProcess::Running);
            p.terminate();
            p.waitForFinished();
            QCOMPARE(p.exitCode(), 0);

            // It opened on a new process, so no change for us
            QCOMPARE(s->m_tabs.count(), 1);
            QCOMPARE(part->url().url(), QString("file://%1").arg(paths[0]));
            QCOMPARE(partDocument(part)->currentPage(), externalProcessExpectedPage);
        }
    }

    if (expectPresentation)
    {
        QCOMPARE(paths.count(), 1);
        Okular::Part *part = s->findChild<Okular::Part*>();

        // Oh Qt5 i want your QTRY_VERIFY
        for (int i = 0; presentationWidget(part) == 0 && i < 20; ++i) {
            QTest::qWait(100);
        }
        QVERIFY(presentationWidget(part) != 0);
    }

    if (externalProcessExpectPresentation)
    {
        Okular::Part *part;
        if (unique)
        {
            QCOMPARE(s->m_tabs.count(), 1);
            part = dynamic_cast<Okular::Part*>(s->m_tabs[0].part);
        }
        else
        {
            QCOMPARE(s->m_tabs.count(), 2);
            part = dynamic_cast<Okular::Part*>(s->m_tabs[1].part);
        }

        for (int i = 0; presentationWidget(part) == 0 && i < 20; ++i) {
            QTest::qWait(100);
        }
        QVERIFY(presentationWidget(part) != 0);
    }

    if (helper)
    {
        QVERIFY(helper->foundDialog);
    }
}

void ClosePrintDialogHelper::closePrintDialog()
{
    Shell *s = findShell();
    QPrintDialog *dialog = s->findChild<QPrintDialog*>();
    if (!dialog) {
        QTimer::singleShot(0, this, SLOT(closePrintDialog()));
        return;
    }
    QVERIFY(dialog);
    QCOMPARE(MainShellTest::tabWidget(s)->currentIndex(), m_expectedTab);
    dialog->close();
    foundDialog = true;
}

void MainShellTest::testUnique2FilesError()
{
    QStringList paths;
    QString serializedOptions = ShellUtils::serializeOptions(false, false, true, false, QString());
    paths << KDESRCDIR "data/file1.pdf" << KDESRCDIR "data/tocreload.pdf";
    Okular::Status status = Okular::main(paths, serializedOptions);
    QCOMPARE(status, Okular::Error);

    QSet<QString> openUrls;
    Shell *s = findShell();
    QVERIFY(!s);
}


QTEST_KDEMAIN( MainShellTest, GUI )
#include "mainshelltest.moc"
