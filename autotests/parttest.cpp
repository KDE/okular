/***************************************************************************
 *   Copyright (C) 2013 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

#include "../core/annotations.h"
#include "../core/page.h"
#include "../part.h"
#include "../ui/toc.h"
#include "../ui/pageview.h"

#include <KConfigDialog>

#include <QClipboard>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTreeView>
#include <QUrl>
#include <QDesktopServices>
#include <QMenu>

namespace Okular
{
class PartTest
    : public QObject
{
    Q_OBJECT

        static bool openDocument(Okular::Part *part, const QString &filePath);

    signals:
        void urlHandler(const QUrl &url);

    private slots:
        void testReload();
        void testCanceledReload();
        void testTOCReload();
        void testForwardPDF();
        void testForwardPDF_data();
        void testGeneratorPreferences();
        void testSelectText();
        void testClickInternalLink();
        void testSaveAs();
        void testSaveAs_data();
        void testMouseMoveOverLinkWhileInSelectionMode();
        void testClickUrlLinkWhileInSelectionMode();
        void testeTextSelectionOverAndAcrossLinks_data();
        void testeTextSelectionOverAndAcrossLinks();
        void testClickUrlLinkWhileLinkTextIsSelected();
        void testRClickWhileLinkTextIsSelected();
        void testRClickOverLinkWhileLinkTextIsSelected();
        void testRClickOnSelectionModeShoulShowFollowTheLinkMenu();
        void testClickAnywhereAfterSelectionShouldUnselect();
        void testeRectSelectionStartingOnLinks();

    private:
        void simulateMouseSelection(double startX, double startY, double endX, double endY, QWidget *target);
};

class PartThatHijacksQueryClose : public Okular::Part
{
    public:
        PartThatHijacksQueryClose(QWidget* parentWidget, QObject* parent,
                                  const QVariantList& args)
        : Okular::Part(parentWidget, parent, args),
          behavior(PassThru)
        {}

        enum Behavior { PassThru, ReturnTrue, ReturnFalse };

        void setQueryCloseBehavior(Behavior new_behavior)
        {
            behavior = new_behavior;
        }

        bool queryClose() override
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
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY( openDocument(&part, QStringLiteral(KDESRCDIR "data/file1.pdf")) );
    part.reload();
    qApp->processEvents();
}

// Test that Okular doesn't crash after a canceled reload
void PartTest::testCanceledReload()
{
    QVariantList dummyArgs;
    PartThatHijacksQueryClose part(nullptr, nullptr, dummyArgs);
    QVERIFY( openDocument(&part, QStringLiteral(KDESRCDIR "data/file1.pdf")) );

    // When queryClose() returns false, the reload operation is canceled (as if
    // the user had chosen Cancel in the "Save changes?" message box)
    part.setQueryCloseBehavior(PartThatHijacksQueryClose::ReturnFalse);

    part.reload();

    qApp->processEvents();
}

void PartTest::testTOCReload()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY( openDocument(&part, QStringLiteral(KDESRCDIR "data/tocreload.pdf")) );
    QCOMPARE(part.m_toc->expandedNodes().count(), 0);
    part.m_toc->m_treeView->expandAll();
    QCOMPARE(part.m_toc->expandedNodes().count(), 3);
    part.reload();
    qApp->processEvents();
    QCOMPARE(part.m_toc->expandedNodes().count(), 3);
}

void PartTest::testForwardPDF()
{
    QFETCH(QString, dir);

    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);

    // Create temp dir named like this: ${system temp dir}/${random string}/${dir}
    const QTemporaryDir tempDir;
    const QDir workDir(QDir(tempDir.path()).filePath(dir));
    workDir.mkpath(QStringLiteral("."));

    QFile f(QStringLiteral(KDESRCDIR "data/synctextest.tex"));
    const QString texDestination = workDir.path() + QStringLiteral("/synctextest.tex");
    QVERIFY(f.copy(texDestination));
    QProcess process;
    process.setWorkingDirectory(workDir.path());
    process.start(QStringLiteral("pdflatex"), QStringList() << QStringLiteral("-synctex=1") << QStringLiteral("-interaction=nonstopmode") << texDestination);
    bool started = process.waitForStarted();
    if (!started) {
        qDebug() << "start error:" << process.error();
        qDebug() << "start stdout:" << process.readAllStandardOutput();
        qDebug() << "start stderr:" << process.readAllStandardError();
    }
    QVERIFY(started);

    process.waitForFinished();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qDebug() << "exit error:" << process.error() << "status" << process.exitStatus() << "code" << process.exitCode();
        qDebug() << "exit stdout:" << process.readAllStandardOutput();
        qDebug() << "exit stderr:" << process.readAllStandardError();
    }

    const QString pdfResult = workDir.path() + QStringLiteral("/synctextest.pdf");
    
    QVERIFY(QFile::exists(pdfResult));
    
    QVERIFY( openDocument(&part, pdfResult) );
    part.m_document->setViewportPage(0);
    QCOMPARE(part.m_document->currentPage(), 0u);
    part.closeUrl();
    
    QUrl u(QUrl::fromLocalFile(pdfResult));
    u.setFragment(QStringLiteral("src:100") + texDestination);
    part.openUrl(u);
    QCOMPARE(part.m_document->currentPage(), 1u);
}

void PartTest::testForwardPDF_data()
{
    QTest::addColumn<QString>("dir");

    QTest::newRow("non-utf8") << QString::fromUtf8("synctextest");
    QTest::newRow("utf8")     << QString::fromUtf8("ßðđđŋßðđŋ");
}

void PartTest::testGeneratorPreferences()
{
    KConfigDialog * dialog;
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);

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

void PartTest::testSelectText()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/file2.pdf")));
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const int mouseY = height * 0.052;
    const int mouseStartX = width * 0.12;
    const int mouseEndX = width * 0.7;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    QApplication::clipboard()->clear();
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "copyTextSelection"));

    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("Hola que tal\n"));
}

void PartTest::testClickInternalLink()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/file2.pdf")));
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseNormal");

    QCOMPARE(part.m_document->currentPage(), 0u);
    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * 0.17, height * 0.05));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(width * 0.17, height * 0.05));
    QTRY_COMPARE(part.m_document->currentPage(), 1u);
}

// cursor switches to Hand when hovering over link in TextSelect mode.
void PartTest::testMouseMoveOverLinkWhileInSelectionMode()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    // move mouse over link
    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * 0.250, height * 0.127));

    // check if mouse icon changed to proper icon
    QTRY_COMPARE(part.m_pageView->cursor().shape(), Qt::PointingHandCursor);
}

// clicking on hyperlink jumps to destination in TextSelect mode.
void PartTest::testClickUrlLinkWhileInSelectionMode()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    // overwrite urlHandler for 'mailto' urls
    QDesktopServices::setUrlHandler("mailto", this, "urlHandler");
    QSignalSpy openUrlSignalSpy(this, SIGNAL(urlHandler(QUrl)));

    // click on url
    QTest::mouseMove(part.m_pageView->viewport(), QPoint(width * 0.250, height * 0.127));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(width * 0.250, height * 0.127));

    // expect that the urlHandler signal was called
    QTRY_COMPARE(openUrlSignalSpy.count(), 1);
    QList<QVariant> arguments = openUrlSignalSpy.takeFirst();
    QCOMPARE(arguments.at(0).value<QUrl>(), QUrl("mailto:foo@foo.bar"));
}

void PartTest::testeTextSelectionOverAndAcrossLinks_data()
{
    QTest::addColumn<double>("mouseStartX");
    QTest::addColumn<double>("mouseEndX");
    QTest::addColumn<QString>("expectedResult");

    // can text-select "over and across" hyperlink.
    QTest::newRow("start selection before link") << 0.1564 << 0.2943 << QStringLiteral(" a link: foo@foo.b");
    // can text-select starting at text and ending selection in middle of hyperlink.
    QTest::newRow("start selection in the middle of the link") << 0.28 << 0.382 << QStringLiteral("o.bar\n");
    // text selection works when selecting left to right or right to left
    QTest::newRow("start selection after link") << 0.40 << 0.05 << QStringLiteral("This is a link: foo@foo.bar\n");
}

// can text-select "over and across" hyperlink.
void PartTest::testeTextSelectionOverAndAcrossLinks()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const double mouseY = height * 0.127;
    QFETCH(double, mouseStartX);
    QFETCH(double, mouseEndX);

    mouseStartX = width * mouseStartX;
    mouseEndX = width * mouseEndX;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    QApplication::clipboard()->clear();
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "copyTextSelection"));

    QFETCH(QString, expectedResult);
    QCOMPARE(QApplication::clipboard()->text(), expectedResult);
}

// can jump to link while there's an active selection of text.
void PartTest::testClickUrlLinkWhileLinkTextIsSelected()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const double mouseY = height * 0.127;
    const double mouseStartX = width * 0.13;
    const double mouseEndX = width * 0.40;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    // overwrite urlHandler for 'mailto' urls
    QDesktopServices::setUrlHandler("mailto", this, "urlHandler");
    QSignalSpy openUrlSignalSpy(this, SIGNAL(urlHandler(QUrl)));

    // click on url
    const double mouseClickX = width * 0.2997;
    const double mouseClickY = height * 0.1293;

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(mouseClickX, mouseClickY));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(mouseClickX, mouseClickY), 1000);

    // expect that the urlHandler signal was called
    QTRY_COMPARE(openUrlSignalSpy.count(), 1);
    QList<QVariant> arguments = openUrlSignalSpy.takeFirst();
    QCOMPARE(arguments.at(0).value<QUrl>(), QUrl("mailto:foo@foo.bar"));
}

// r-click on the selected text gives the "Go To:" content menu option
void PartTest::testRClickWhileLinkTextIsSelected()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const double mouseY = height * 0.162;
    const double mouseStartX = width * 0.42;
    const double mouseEndX = width * 0.60;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    // Need to do this because the pop-menu will have his own mainloop and will block tests until
    // the menu disappear
    PageView *view = part.m_pageView;
    QTimer::singleShot(2000, [view]() {
        // check if popup menu is active and visible
        QMenu *menu = qobject_cast<QMenu*>(view->findChild<QMenu*>("PopupMenu"));
        QVERIFY(menu);
        QVERIFY(menu->isVisible());

        // check if the menu contains go-to link action
        QAction *goToAction = qobject_cast<QAction*>(menu->findChild<QAction*>("GoToAction"));
        QVERIFY(goToAction);

        // check if the "follow this link" action is not visible
        QAction *processLinkAction = qobject_cast<QAction*>(menu->findChild<QAction*>("ProcessLinkAction"));
        QVERIFY(!processLinkAction);

        // check if the "copy link address" action is not visible
        QAction *copyLinkLocation = qobject_cast<QAction*>(menu->findChild<QAction*>("CopyLinkLocationAction"));
        QVERIFY(!copyLinkLocation);

        // close menu to continue test
        menu->close();
    });

    // click on url
    const double mouseClickX = width * 0.425;
    const double mouseClickY = height * 0.162;

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(mouseClickX, mouseClickY));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::RightButton, Qt::NoModifier, QPoint(mouseClickX, mouseClickY), 1000);

    // will continue after pop-menu get closed
}


// r-click on the link gives the "follow this link" content menu option
void PartTest::testRClickOverLinkWhileLinkTextIsSelected()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const double mouseY = height * 0.162;
    const double mouseStartX = width * 0.42;
    const double mouseEndX = width * 0.60;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    // Need to do this because the pop-menu will have his own mainloop and will block tests until
    // the menu disappear
    PageView *view = part.m_pageView;
    QTimer::singleShot(2000, [view]() {
        // check if popup menu is active and visible
        QMenu *menu = qobject_cast<QMenu*>(view->findChild<QMenu*>("PopupMenu"));
        QVERIFY(menu);
        QVERIFY(menu->isVisible());

        // check if the menu contains "follow this link" action
        QAction *processLinkAction = qobject_cast<QAction*>(menu->findChild<QAction*>("ProcessLinkAction"));
        QVERIFY(processLinkAction);

        // check if the menu contains "copy link address" action
        QAction *copyLinkLocation = qobject_cast<QAction*>(menu->findChild<QAction*>("CopyLinkLocationAction"));
        QVERIFY(copyLinkLocation);

        // close menu to continue test
        menu->close();
    });

    // click on url
    const double mouseClickX = width * 0.593;
    const double mouseClickY = height * 0.162;

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(mouseClickX, mouseClickY));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::RightButton, Qt::NoModifier, QPoint(mouseClickX, mouseClickY), 1000);

    // will continue after pop-menu get closed
}

void PartTest::testRClickOnSelectionModeShoulShowFollowTheLinkMenu()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    // Need to do this because the pop-menu will have his own mainloop and will block tests until
    // the menu disappear
    PageView *view = part.m_pageView;
    QTimer::singleShot(2000, [view]() {
        // check if popup menu is active and visible
        QMenu *menu = qobject_cast<QMenu*>(view->findChild<QMenu*>("PopupMenu"));
        QVERIFY(menu);
        QVERIFY(menu->isVisible());

        // check if the menu contains "Follow this link" action
        QAction *processLink = qobject_cast<QAction*>(menu->findChild<QAction*>("ProcessLinkAction"));
        QVERIFY(processLink);

        // chek if the menu contains  "Copy Link Address" action
        QAction *actCopyLinkLocation = qobject_cast<QAction*>(menu->findChild<QAction*>("CopyLinkLocationAction"));
        QVERIFY(actCopyLinkLocation);

        // close menu to continue test
        menu->close();
    });

    // r-click on url
    const double mouseClickX = width * 0.604;
    const double mouseClickY = height * 0.162;

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(mouseClickX, mouseClickY));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::RightButton, Qt::NoModifier, QPoint(mouseClickX, mouseClickY), 1000);
    QTest::qWait(3000);

    // will continue after pop-menu get closed
}

void PartTest::testClickAnywhereAfterSelectionShouldUnselect()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseTextSelect"));

    const double mouseY = height * 0.162;
    const double mouseStartX = width * 0.42;
    const double mouseEndX = width * 0.60;

    simulateMouseSelection(mouseStartX, mouseY, mouseEndX, mouseY, part.m_pageView->viewport());

    // click on url
    const double mouseClickX = width * 0.10;

    QTest::mouseMove(part.m_pageView->viewport(), QPoint(mouseClickX, mouseY));
    QTest::mouseClick(part.m_pageView->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(mouseClickX, mouseY), 1000);

    QApplication::clipboard()->clear();
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "copyTextSelection"));

    // check if copied text is empty what means no text selected
    QVERIFY(QApplication::clipboard()->text().isEmpty());
}

void PartTest::testeRectSelectionStartingOnLinks()
{
    QVariantList dummyArgs;
    Okular::Part part(nullptr, nullptr, dummyArgs);
    QVERIFY(openDocument(&part, QStringLiteral(KDESRCDIR "data/pdf_with_links.pdf")));
    // resize window to avoid problem with selection areas
    part.widget()->resize(800, 600);
    part.widget()->show();
    QTest::qWaitForWindowExposed(part.widget());

    const int width = part.m_pageView->horizontalScrollBar()->maximum() +
                      part.m_pageView->viewport()->width();
    const int height = part.m_pageView->verticalScrollBar()->maximum() +
                       part.m_pageView->viewport()->height();

    part.m_document->setViewportPage(0);

    // wait for pixmap
    QTRY_VERIFY(part.m_document->page(0)->hasPixmap(part.m_pageView));

    // enter text-selection mode
    QVERIFY(QMetaObject::invokeMethod(part.m_pageView, "slotSetMouseSelect"));

    const double mouseStartY = height * 0.127;
    const double mouseEndY = height * 0.127;
    const double mouseStartX = width *  0.28;
    const double mouseEndX = width *  0.382;

    // Need to do this because the pop-menu will have his own mainloop and will block tests until
    // the menu disappear
    PageView *view = part.m_pageView;
    QTimer::singleShot(2000, [view]() {
        QApplication::clipboard()->clear();

        // check if popup menu is active and visible
        QMenu *menu = qobject_cast<QMenu*>(view->findChild<QMenu*>("PopupMenu"));
        QVERIFY(menu);
        QVERIFY(menu->isVisible());

        // check if the copy selected text to clipboard is present
        QAction *copyAct = qobject_cast<QAction*>(menu->findChild<QAction*>("CopyTextToClipboard"));
        QVERIFY(copyAct);

        menu->close();
    });

    simulateMouseSelection(mouseStartX, mouseStartY, mouseEndX, mouseEndY, part.m_pageView->viewport());

    // wait menu get closed
}


void PartTest::simulateMouseSelection(double startX, double startY, double endX, double endY, QWidget *target)
{
    QTestEventList events;
    events.addMouseMove(QPoint(startX, startY));
    events.addMousePress(Qt::LeftButton, Qt::NoModifier, QPoint(startX, startY));
    events.addMouseMove(QPoint(endX, endY));
    // without this wait the test fails. 100ms were enough on my local system, but when running under valgrind
    // or on the CI server we need to wait longer.
    events.addDelay(1000);
    events.addMouseRelease(Qt::LeftButton, Qt::NoModifier, QPoint(endX, endY));

    events.simulate(target);
}

void PartTest::testSaveAs()
{
    QFETCH(QString, file);
    QFETCH(QString, extension);
    QFETCH(bool, nativelySupportsAnnotations);
    QFETCH(bool, canSwapBackingFile);

    // If we expect not to be able to preserve annotations when we write a
    // native file, disable the warning otherwise the test will wait for the
    // user to confirm. On the other end, if we expect annotations to be
    // preserved (and thus no warning), we keep the warning on so that if it
    // shows the test timeouts and we can notice that something is wrong.
    const Part::SaveAsFlags extraSaveAsFlags = nativelySupportsAnnotations ?
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
        Okular::Part part(nullptr, nullptr, QVariantList());
        part.openDocument( file );

        QCOMPARE(part.m_document->canSwapBackingFile(), canSwapBackingFile);

        Okular::Annotation *annot = new Okular::TextAnnotation();
        annot->setBoundingRectangle( Okular::NormalizedRect( 0.1, 0.1, 0.15, 0.15 ) );
        annot->setContents( "annot contents" );
        part.m_document->addPageAnnotation( 0, annot );
        annotName = annot->uniqueName();

        QVERIFY( part.saveAs( QUrl::fromLocalFile( archiveSave.fileName() ), extraSaveAsFlags | Part::SaveAsOkularArchive ) );
        QVERIFY( part.saveAs( QUrl::fromLocalFile( nativeDirectSave.fileName() ), extraSaveAsFlags ) );

        part.closeUrl();
    }

    qDebug() << "Open the .okular, check that the annotation is present and save to native";
    {
        Okular::Part part(nullptr, nullptr, QVariantList());
        part.openDocument( archiveSave.fileName() );

        QCOMPARE( part.m_document->page( 0 )->annotations().size(), 1 );
        QCOMPARE( part.m_document->page( 0 )->annotations().first()->uniqueName(), annotName );

        QVERIFY( part.saveAs( QUrl::fromLocalFile( nativeFromArchiveFile.fileName() ), extraSaveAsFlags ) );

        part.closeUrl();
    }

    qDebug() << "Open the native file saved directly, and check that the annot"
        << "is there iff we expect it";
    {
        Okular::Part part(nullptr, nullptr, QVariantList());
        part.openDocument( nativeDirectSave.fileName() );

        QCOMPARE( part.m_document->page( 0 )->annotations().size(), nativelySupportsAnnotations ? 1 : 0 );
        if ( nativelySupportsAnnotations )
            QCOMPARE( part.m_document->page( 0 )->annotations().first()->uniqueName(), annotName );

        part.closeUrl();
    }

    qDebug() << "Open the native file saved from the .okular, and check that the annot"
        << "is there iff we expect it";
    {
        Okular::Part part(nullptr, nullptr, QVariantList());
        part.openDocument( nativeFromArchiveFile.fileName() );

        QCOMPARE( part.m_document->page( 0 )->annotations().size(), nativelySupportsAnnotations ? 1 : 0 );
        if ( nativelySupportsAnnotations )
            QCOMPARE( part.m_document->page( 0 )->annotations().first()->uniqueName(), annotName );

        part.closeUrl();
    }
}

void PartTest::testSaveAs_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<bool>("nativelySupportsAnnotations");
    QTest::addColumn<bool>("canSwapBackingFile");

    QTest::newRow("pdf") << KDESRCDIR "data/file1.pdf" << "pdf" << true << true;
    QTest::newRow("epub") << KDESRCDIR "data/contents.epub" << "epub" << false << false;
    QTest::newRow("jpg") << KDESRCDIR "data/potato.jpg" << "jpg" << false << true;
}

}

int main(int argc, char *argv[])
{
    // Force consistent locale
    QLocale locale(QStringLiteral("en_US.UTF-8"));
    if (locale == QLocale::c()) { // This is the way to check if the above worked
        locale = QLocale(QLocale::English, QLocale::UnitedStates);
    }

    QLocale::setDefault(locale);
    qputenv("LC_ALL", "en_US.UTF-8"); // For UNIX, third-party libraries

    // Ensure consistent configs/caches
    QTemporaryDir homeDir; // QTemporaryDir automatically cleans up when it goes out of scope
    Q_ASSERT(homeDir.isValid());
    QByteArray homePath = QFile::encodeName(homeDir.path());
    qDebug() << homePath;
    qputenv("USERPROFILE", homePath);
    qputenv("HOME", homePath);
    qputenv("XDG_DATA_HOME", homePath + "/.local");
    qputenv("XDG_CONFIG_HOME", homePath + "/.kde-unit-test/xdg/config");

    // Disable fancy debug output
    qunsetenv("QT_MESSAGE_PATTERN");

    QApplication app( argc, argv );
    app.setApplicationName(QLatin1String("okularparttest"));
    app.setOrganizationDomain(QLatin1String("kde.org"));
    app.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<QUrl>(); /*as done by kapplication*/
    qRegisterMetaType<QList<QUrl>>();

    Okular::PartTest test;

    return QTest::qExec( &test, argc, argv );
}

#include "parttest.moc"
