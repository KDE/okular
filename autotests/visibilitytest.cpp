/*
    SPDX-FileCopyrightText: 2018 Intevation GmbH <intevation@intevation.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include "../settings_core.h"
#include "core/document.h"
#include <QMap>
#include <QMimeDatabase>
#include <QMimeType>
#include <QTemporaryFile>
#include <core/form.h>
#include <core/page.h>

class VisibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testJavaScriptVisibility();
    void testSaveLoad();
    void testActionVisibility();

private:
    void verifyTargetStates(bool visible);

    Okular::Document *m_document;
    QMap<QString, Okular::FormField *> m_fields;
};

void VisibilityTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("visibilitytest"));
    m_document = new Okular::Document(nullptr);

    const QString testFile = QStringLiteral(KDESRCDIR "data/visibilitytest.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);
    QCOMPARE(m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess);

    // The test document has four buttons:
    // HideScriptButton -> Hides targets with JavaScript
    // ShowScriptButton -> Shows targets with JavaScript
    // HideActionButton -> Hides targets with HideAction
    // ShowActionButton -> Shows targets with HideAction
    //
    // The target fields are:
    // TargetButton TargetText TargetCheck TargetDropDown TargetRadio
    //
    // With two radio buttons named TargetRadio.

    const Okular::Page *page = m_document->page(0);
    const QList<Okular::FormField *> pageFormFields = page->formFields();
    for (Okular::FormField *ff : pageFormFields) {
        m_fields.insert(ff->name(), ff);
    }
}

void VisibilityTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}

void VisibilityTest::verifyTargetStates(bool visible)
{
    QCOMPARE(m_fields[QStringLiteral("TargetButton")]->isVisible(), visible);
    QCOMPARE(m_fields[QStringLiteral("TargetText")]->isVisible(), visible);
    QCOMPARE(m_fields[QStringLiteral("TargetCheck")]->isVisible(), visible);
    QCOMPARE(m_fields[QStringLiteral("TargetDropDown")]->isVisible(), visible);

    // Radios do not properly inherit a name from the parent group so
    // this does not work yet (And would probably need some list handling).
    // QCOMPARE( m_fields[QStringLiteral( "TargetRadio" )].isVisible(), visible );
}

void VisibilityTest::testJavaScriptVisibility()
{
    auto hideBtn = m_fields[QStringLiteral("HideScriptButton")];
    auto showBtn = m_fields[QStringLiteral("ShowScriptButton")];

    // We start with all fields visible
    verifyTargetStates(true);

    m_document->processAction(hideBtn->activationAction());

    // Now all should be hidden
    verifyTargetStates(false);

    // And show again
    m_document->processAction(showBtn->activationAction());
    verifyTargetStates(true);
}

void VisibilityTest::testSaveLoad()
{
    auto hideBtn = m_fields[QStringLiteral("HideScriptButton")];
    auto showBtn = m_fields[QStringLiteral("ShowScriptButton")];

    // We start with all fields visible
    verifyTargetStates(true);

    m_document->processAction(hideBtn->activationAction());

    // Now all should be hidden
    verifyTargetStates(false);

    // Save the changed states
    QTemporaryFile saveFile(QStringLiteral("%1/okrXXXXXX.pdf").arg(QDir::tempPath()));
    QVERIFY(saveFile.open());
    saveFile.close();

    QVERIFY(m_document->saveChanges(saveFile.fileName()));

    auto newDoc = new Okular::Document(nullptr);

    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(saveFile.fileName());
    QCOMPARE(newDoc->openDocument(saveFile.fileName(), QUrl(), mime), Okular::Document::OpenSuccess);

    const Okular::Page *page = newDoc->page(0);

    bool anyChecked = false; // Saveguard against accidental test passing here ;-)
    const QList<Okular::FormField *> pageFormFields = page->formFields();
    for (Okular::FormField *ff : pageFormFields) {
        if (ff->name().startsWith(QStringLiteral("Target"))) {
            QVERIFY(!ff->isVisible());
            anyChecked = true;
        }
    }
    QVERIFY(anyChecked);

    newDoc->closeDocument();
    delete newDoc;

    // Restore the state of the member document
    m_document->processAction(showBtn->activationAction());
}

void VisibilityTest::testActionVisibility()
{
    auto hideBtn = m_fields[QStringLiteral("HideActionButton")];
    auto showBtn = m_fields[QStringLiteral("ShowActionButton")];

    verifyTargetStates(true);

    m_document->processAction(hideBtn->activationAction());

    verifyTargetStates(false);

    m_document->processAction(showBtn->activationAction());

    verifyTargetStates(true);
}

QTEST_MAIN(VisibilityTest)
#include "visibilitytest.moc"
