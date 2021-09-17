/*
    SPDX-FileCopyrightText: 2020 Simone Gaiarin <simgunz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// clazy:excludeall=qstring-allocations

#include <QTest>

#include <QMenu>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QWidget>

#include <KActionCollection>
#include <KSelectAction>

#include "../core/page.h"
#include "../part/pageview.h"
#include "../part/part.h"
#include "../part/toggleactionmenu.h"
#include "../settings.h"
#include "../shell/okular_main.h"
#include "../shell/shell.h"
#include "../shell/shellutils.h"
#include "closedialoghelper.h"

namespace Okular
{
class PartTest
{
public:
    Okular::Document *partDocument(Okular::Part *part) const
    {
        return part->m_document;
    }
    PageView *pageView(Okular::Part *part) const
    {
        return part->m_pageView;
    }
};
}

class AnnotationToolBarTest : public QObject, public Okular::PartTest
{
    Q_OBJECT

public:
    static void initMain();
    static QTabWidget *tabWidget(Shell *s)
    {
        return s->m_tabWidget;
    }

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void testAnnotationToolBar();
    void testAnnotationToolBar_data();
    void testAnnotationToolBarActionsEnabledState();
    void testAnnotationToolBarActionsEnabledState_data();
    void testAnnotationToolBarConfigActionsEnabledState();
    void testAnnotationToolBarConfigActionsEnabledState_data();

private:
    bool simulateAddPopupAnnotation(Okular::Part *part, int mouseX, int mouseY);
};

Shell *findShell(Shell *ignore = nullptr)
{
    const QWidgetList wList = QApplication::topLevelWidgets();
    for (QWidget *widget : wList) {
        Shell *s = qobject_cast<Shell *>(widget);
        if (s && s != ignore)
            return s;
    }
    return nullptr;
}

void AnnotationToolBarTest::initMain()
{
    // Ensure consistent configs/caches and Default UI
    QTemporaryDir homeDir;
    Q_ASSERT(homeDir.isValid());
    QByteArray homePath = QFile::encodeName(homeDir.path());
    qputenv("USERPROFILE", homePath);
    qputenv("HOME", homePath);
    qputenv("XDG_DATA_HOME", homePath + "/.local");
    qputenv("XDG_CONFIG_HOME", homePath + "/.kde-unit-test/xdg/config");
}

void AnnotationToolBarTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    // Don't pollute people's okular settings
    Okular::Settings::instance(QStringLiteral("annotationtoolbartest"));
}

void AnnotationToolBarTest::init()
{
    // Default settings for every test
    Okular::Settings::self()->setDefaults();
}

void AnnotationToolBarTest::cleanup()
{
    Shell *s;
    while ((s = findShell())) {
        delete s;
    }
}

bool AnnotationToolBarTest::simulateAddPopupAnnotation(Okular::Part *part, int mouseX, int mouseY)
{
    int annotationCount = partDocument(part)->page(0)->annotations().size();
    QTest::mouseMove(pageView(part)->viewport(), QPoint(mouseX, mouseY));
    QTest::mouseClick(pageView(part)->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(mouseX, mouseY));
    bool annotationAdded = partDocument(part)->page(0)->annotations().size() == annotationCount + 1;
    return annotationAdded;
}

void AnnotationToolBarTest::testAnnotationToolBar()
{
    // Using tabs we test that the annotation toolbar works on each Okular::Part
    Okular::Settings::self()->setShellOpenFileInTabs(true);

    const QStringList paths = {QStringLiteral(KDESRCDIR "data/file1.pdf"), QStringLiteral(KDESRCDIR "data/file2.pdf")};
    QString serializedOptions = ShellUtils::serializeOptions(false, false, false, false, false, QString(), QString());

    Okular::Status status = Okular::main(paths, serializedOptions);
    QCOMPARE(status, Okular::Success);
    Shell *s = findShell();
    QVERIFY(s);
    QVERIFY(QTest::qWaitForWindowExposed(s));
    QFETCH(int, tabIndex);
    s->m_tabWidget->tabBar()->setCurrentIndex(tabIndex);
    Okular::Part *part = dynamic_cast<Okular::Part *>(s->m_tabs[tabIndex].part);
    QVERIFY(part);

    QToolBar *annToolBar = s->findChild<QToolBar *>(QStringLiteral("annotationToolBar"));
    QVERIFY(annToolBar);

    // Check config action default enabled states
    QAction *aQuickTools = part->actionCollection()->action(QStringLiteral("annotation_favorites"));
    QAction *aAddToQuickTools = part->actionCollection()->action(QStringLiteral("annotation_bookmark"));
    QAction *aAdvancedSettings = part->actionCollection()->action(QStringLiteral("annotation_settings_advanced"));
    QAction *aContinuousMode = part->actionCollection()->action(QStringLiteral("annotation_settings_pin"));
    QVERIFY(aQuickTools->isEnabled());
    QVERIFY(!aAddToQuickTools->isEnabled());
    QVERIFY(!aAdvancedSettings->isEnabled());
    QVERIFY(aContinuousMode->isEnabled());

    // Ensure that the 'Quick Annotations' action is correctly populated
    // (at least the 'Configure Annotations...' action must be present)
    QVERIFY(!aQuickTools->menu()->actions().isEmpty());

    // Test annotation toolbar visibility triggers
    QAction *toggleAnnotationToolBar = part->actionCollection()->action(QStringLiteral("mouse_toggle_annotate"));
    QAction *aHideToolBar = part->actionCollection()->action(QStringLiteral("hide_annotation_toolbar"));
    QVERIFY(toggleAnnotationToolBar);
    QVERIFY(aHideToolBar);
    toggleAnnotationToolBar->setChecked(false);
    QTRY_VERIFY(!annToolBar->isVisible());
    toggleAnnotationToolBar->trigger();
    QTRY_VERIFY2(annToolBar->isVisible(), "Annotation action failed to show.");
    toggleAnnotationToolBar->trigger();
    QTRY_VERIFY2(!annToolBar->isVisible(), "Annotation action failed to hide.");

    toggleAnnotationToolBar->setChecked(true);
    QTRY_VERIFY(annToolBar->isVisible());
    aHideToolBar->trigger();
    QTRY_VERIFY2(!annToolBar->isVisible(), "Hide toolbar action failed to hide.");

    toggleAnnotationToolBar->setChecked(false);
    QTRY_VERIFY(!annToolBar->isVisible());
    QTest::keyClick(part->widget(), Qt::Key_1, Qt::AltModifier);
    QTRY_VERIFY2(annToolBar->isVisible(), "ToolBar not shown when triggering annotation using shortcut.");
    toggleAnnotationToolBar->setChecked(false);
    QTRY_VERIFY(!annToolBar->isVisible());
    QTest::keyClick(part->widget(), Qt::Key_3);
    QTRY_VERIFY2(!annToolBar->isVisible(), "ToolBar shown when triggering quick annotation using shortcut.");

    // set mouse mode to browse before starting the tests on the annotation actions
    QAction *aMouseNormal = part->actionCollection()->action(QStringLiteral("mouse_drag"));
    QVERIFY(aMouseNormal);
    aMouseNormal->trigger();
    QTRY_COMPARE(Okular::Settings::mouseMode(), static_cast<int>(Okular::Settings::EnumMouseMode::Browse));

    // Click an annotation action to enable it
    QAction *aPopupNote = part->actionCollection()->action(QStringLiteral("annotation_popup_note"));
    QVERIFY(aPopupNote);
    aPopupNote->trigger();
    int mouseX = 350;
    int mouseY = 100;
    QCOMPARE(simulateAddPopupAnnotation(part, mouseX, mouseY), true);
    QTRY_COMPARE(aMouseNormal->isChecked(), false);

    // Click again the same annotation action to disable it
    aPopupNote->trigger();
    mouseY = 150;
    QCOMPARE(simulateAddPopupAnnotation(part, mouseX, mouseY), false);
    QTRY_COMPARE(aMouseNormal->isChecked(), true);

    // Trigger the action using a shortcut
    QTest::keyClick(part->widget(), Qt::Key_7, Qt::AltModifier);
    mouseY = 200;
    QCOMPARE(simulateAddPopupAnnotation(part, mouseX, mouseY), true);
    QTRY_COMPARE(aMouseNormal->isChecked(), false);

    // Click Esc to disable all annotations
    QTest::keyClick(pageView(part), Qt::Key_Escape);
    mouseY = 250;
    QCOMPARE(simulateAddPopupAnnotation(part, mouseX, mouseY), false);
    QTRY_COMPARE(aMouseNormal->isChecked(), true);

    // Trigger the action using a quick annotation shortcut
    QTest::keyClick(part->widget(), Qt::Key_6);
    QCOMPARE(simulateAddPopupAnnotation(part, mouseX, mouseY), true);
    QTRY_COMPARE(aMouseNormal->isChecked(), false);

    // Test pin/continuous mode action
    QVERIFY(aContinuousMode->isEnabled());
    QCOMPARE(simulateAddPopupAnnotation(part, mouseX, mouseY), true);
    QCOMPARE(simulateAddPopupAnnotation(part, mouseX, mouseY), true);
    aContinuousMode->trigger();
    QCOMPARE(simulateAddPopupAnnotation(part, mouseX, mouseY), true);
    QCOMPARE(simulateAddPopupAnnotation(part, mouseX, mouseY), false);

    // Test adding a tool to the quick tool list using the bookmark action
    QScopedPointer<TestingUtils::CloseDialogHelper> closeDialogHelper;
    closeDialogHelper.reset(new TestingUtils::CloseDialogHelper(QDialogButtonBox::Ok));
    aPopupNote->trigger();
    QVERIFY(aPopupNote->isChecked());
    int quickActionCount = aQuickTools->menu()->actions().size();
    aAddToQuickTools->trigger();
    QTRY_COMPARE(aQuickTools->menu()->actions().size(), quickActionCount + 1);
    // Trigger the quick tool that was just added
    aQuickTools->menu()->actions().at(6)->trigger();
    QCOMPARE(simulateAddPopupAnnotation(part, mouseX, mouseY), true);
}

void AnnotationToolBarTest::testAnnotationToolBar_data()
{
    QTest::addColumn<int>("tabIndex");
    QTest::addRow("first tab") << 0;
    QTest::addRow("second tab") << 1;
}

void AnnotationToolBarTest::testAnnotationToolBarActionsEnabledState()
{
    QFETCH(QString, document);

    const QStringList paths = {document};
    QString serializedOptions = ShellUtils::serializeOptions(false, false, false, false, false, QString(), QString());

    Okular::Status status = Okular::main(paths, serializedOptions);
    QCOMPARE(status, Okular::Success);
    Shell *s = findShell();
    QVERIFY(s);
    QVERIFY(QTest::qWaitForWindowExposed(s));

    Okular::Part *part = s->findChild<Okular::Part *>();
    QVERIFY(part);

    KActionCollection *ac = part->actionCollection();
    QAction *aQuickTools = ac->action(QStringLiteral("annotation_favorites"));
    QAction *aHighlighter = ac->action(QStringLiteral("annotation_highlighter"));
    QAction *aUnderline = ac->action(QStringLiteral("annotation_underline"));
    QAction *aSquiggle = ac->action(QStringLiteral("annotation_squiggle"));
    QAction *aStrikeout = ac->action(QStringLiteral("annotation_strike_out"));
    QAction *aTypewriter = ac->action(QStringLiteral("annotation_typewriter"));
    QAction *aInlineNote = ac->action(QStringLiteral("annotation_inline_note"));
    QAction *aPopupNote = ac->action(QStringLiteral("annotation_popup_note"));
    QAction *aFreehandLine = ac->action(QStringLiteral("annotation_freehand_line"));
    QAction *aGeomShapes = ac->action(QStringLiteral("annotation_geometrical_shape"));
    QAction *aStamp = ac->action(QStringLiteral("annotation_stamp"));

    QFETCH(bool, aQuickToolsEnabled);
    QFETCH(bool, aHighlighterEnabled);
    QFETCH(bool, aUnderlineEnabled);
    QFETCH(bool, aSquiggleEnabled);
    QFETCH(bool, aStrikeoutEnabled);
    QFETCH(bool, aTypewriterEnabled);
    QFETCH(bool, aInlineNoteEnabled);
    QFETCH(bool, aPopupNoteEnabled);
    QFETCH(bool, aFreehandLineEnabled);
    QFETCH(bool, aGeomShapesEnabled);
    QFETCH(bool, aStampEnabled);

    QCOMPARE(aQuickTools->isEnabled(), aQuickToolsEnabled);
    QCOMPARE(aHighlighter->isEnabled(), aHighlighterEnabled);
    QCOMPARE(aUnderline->isEnabled(), aUnderlineEnabled);
    QCOMPARE(aSquiggle->isEnabled(), aSquiggleEnabled);
    QCOMPARE(aStrikeout->isEnabled(), aStrikeoutEnabled);
    QCOMPARE(aTypewriter->isEnabled(), aTypewriterEnabled);
    QCOMPARE(aInlineNote->isEnabled(), aInlineNoteEnabled);
    QCOMPARE(aPopupNote->isEnabled(), aPopupNoteEnabled);
    QCOMPARE(aFreehandLine->isEnabled(), aFreehandLineEnabled);
    QCOMPARE(aGeomShapes->isEnabled(), aGeomShapesEnabled);
    QCOMPARE(aStamp->isEnabled(), aStampEnabled);

    // trigger a reparsing of the tools to ensure that the enabled/disabled state is not changed (bug: 424296)
    QAction *aMouseSelect = ac->action(QStringLiteral("mouse_select"));
    QAction *aMouseNormal = ac->action(QStringLiteral("mouse_drag"));
    aMouseSelect->trigger();
    aMouseNormal->trigger();

    QCOMPARE(aQuickTools->isEnabled(), aQuickToolsEnabled);
    QCOMPARE(aHighlighter->isEnabled(), aHighlighterEnabled);
    QCOMPARE(aUnderline->isEnabled(), aUnderlineEnabled);
    QCOMPARE(aSquiggle->isEnabled(), aSquiggleEnabled);
    QCOMPARE(aStrikeout->isEnabled(), aStrikeoutEnabled);
    QCOMPARE(aTypewriter->isEnabled(), aTypewriterEnabled);
    QCOMPARE(aInlineNote->isEnabled(), aInlineNoteEnabled);
    QCOMPARE(aPopupNote->isEnabled(), aPopupNoteEnabled);
    QCOMPARE(aFreehandLine->isEnabled(), aFreehandLineEnabled);
    QCOMPARE(aGeomShapes->isEnabled(), aGeomShapesEnabled);
    QCOMPARE(aStamp->isEnabled(), aStampEnabled);
}

void AnnotationToolBarTest::testAnnotationToolBarActionsEnabledState_data()
{
    QTest::addColumn<QString>("document");
    QTest::addColumn<bool>("aQuickToolsEnabled");
    QTest::addColumn<bool>("aHighlighterEnabled");
    QTest::addColumn<bool>("aUnderlineEnabled");
    QTest::addColumn<bool>("aSquiggleEnabled");
    QTest::addColumn<bool>("aStrikeoutEnabled");
    QTest::addColumn<bool>("aTypewriterEnabled");
    QTest::addColumn<bool>("aInlineNoteEnabled");
    QTest::addColumn<bool>("aPopupNoteEnabled");
    QTest::addColumn<bool>("aFreehandLineEnabled");
    QTest::addColumn<bool>("aGeomShapesEnabled");
    QTest::addColumn<bool>("aStampEnabled");

    QTest::addRow("pdf") << QStringLiteral(KDESRCDIR "data/file1.pdf") << true << true << true << true << true << true << true << true << true << true << true;
    QTest::addRow("protected-pdf") << QStringLiteral(KDESRCDIR "data/protected.pdf") << false << false << false << false << false << false << false << false << false << false << false;
    QTest::addRow("image") << QStringLiteral(KDESRCDIR "data/potato.jpg") << true << false << false << false << false << true << true << true << true << true << true;
}

void AnnotationToolBarTest::testAnnotationToolBarConfigActionsEnabledState()
{
    const QStringList paths = {QStringLiteral(KDESRCDIR "data/file1.pdf")};
    QString serializedOptions = ShellUtils::serializeOptions(false, false, false, false, false, QString(), QString());

    Okular::Status status = Okular::main(paths, serializedOptions);
    QCOMPARE(status, Okular::Success);
    Shell *s = findShell();
    QVERIFY(s);
    QVERIFY(QTest::qWaitForWindowExposed(s));

    Okular::Part *part = s->findChild<Okular::Part *>();
    QVERIFY(part);

    KActionCollection *ac = part->actionCollection();
    QAction *aWidth = ac->action(QStringLiteral("annotation_settings_width"));
    QAction *aColor = ac->action(QStringLiteral("annotation_settings_color"));
    QAction *aInnerColor = ac->action(QStringLiteral("annotation_settings_inner_color"));
    QAction *aOpacity = ac->action(QStringLiteral("annotation_settings_opacity"));
    QAction *aFont = ac->action(QStringLiteral("annotation_settings_font"));

    QFETCH(QString, annotationActionName);
    QFETCH(bool, widthEnabled);
    QFETCH(bool, colorEnabled);
    QFETCH(bool, innerColorEnabled);
    QFETCH(bool, opacityEnabled);
    QFETCH(bool, fontEnabled);

    QAction *annotationAction = ac->action(annotationActionName);
    annotationAction->trigger();

    QCOMPARE(aWidth->isEnabled(), widthEnabled);
    QCOMPARE(aColor->isEnabled(), colorEnabled);
    QCOMPARE(aInnerColor->isEnabled(), innerColorEnabled);
    QCOMPARE(aOpacity->isEnabled(), opacityEnabled);
    QCOMPARE(aFont->isEnabled(), fontEnabled);
}

void AnnotationToolBarTest::testAnnotationToolBarConfigActionsEnabledState_data()
{
    QTest::addColumn<QString>("annotationActionName");
    QTest::addColumn<bool>("widthEnabled");
    QTest::addColumn<bool>("colorEnabled");
    QTest::addColumn<bool>("innerColorEnabled");
    QTest::addColumn<bool>("opacityEnabled");
    QTest::addColumn<bool>("fontEnabled");

    QTest::addRow("annotation_highlighter") << QStringLiteral("annotation_highlighter") << false << true << false << true << false;
    QTest::addRow("annotation_underline") << QStringLiteral("annotation_underline") << false << true << false << true << false;
    QTest::addRow("annotation_squiggle") << QStringLiteral("annotation_squiggle") << false << true << false << true << false;
    QTest::addRow("annotation_strike_out") << QStringLiteral("annotation_strike_out") << false << true << false << true << false;
    QTest::addRow("annotation_typewriter") << QStringLiteral("annotation_typewriter") << false << true << false << true << true;
    QTest::addRow("annotation_inline_note") << QStringLiteral("annotation_inline_note") << false << true << false << true << true;
    QTest::addRow("annotation_popup_note") << QStringLiteral("annotation_popup_note") << false << true << false << true << false;
    QTest::addRow("annotation_freehand_line") << QStringLiteral("annotation_freehand_line") << true << true << false << true << false;
    QTest::addRow("annotation_line") << QStringLiteral("annotation_straight_line") << true << true << false << true << false;
    QTest::addRow("annotation_rectangle") << QStringLiteral("annotation_rectangle") << true << true << true << true << false;
}

QTEST_MAIN(AnnotationToolBarTest)
#include "annotationtoolbartest.moc"
