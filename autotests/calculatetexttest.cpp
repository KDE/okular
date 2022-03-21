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
#include <core/form.h>
#include <core/page.h>

class CalculateTextTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testSimpleCalculate();

private:
    Okular::Document *m_document;
};

void CalculateTextTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("calculatetexttest"));
    m_document = new Okular::Document(nullptr);
}

void CalculateTextTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}

void CalculateTextTest::testSimpleCalculate()
{
    // Force consistent locale
    QLocale locale(QStringLiteral("en_US.UTF-8"));
    if (locale == QLocale::c()) {
        locale = QLocale(QLocale::English, QLocale::UnitedStates);
    }

    QLocale::setDefault(locale);
    const QString testFile = QStringLiteral(KDESRCDIR "data/simpleCalculate.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);
    QCOMPARE(m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess);

    const Okular::Page *page = m_document->page(0);

    QMap<QString, Okular::FormFieldText *> fields;

    // Field names in test document are:
    // field1, field2, field3, Sum, AVG, Prod, Min, Max

    const QList<Okular::FormField *> pageFormFields = page->formFields();
    for (Okular::FormField *ff : pageFormFields) {
        fields.insert(ff->name(), static_cast<Okular::FormFieldText *>(ff));
    }

    // Set some values and do calculation
    Okular::FormFieldText *field1 = fields[QStringLiteral("field1")];
    QVERIFY(field1);
    m_document->editFormText(0, field1, QStringLiteral("10"), 0, 0, 0);

    Okular::FormFieldText *field2 = fields[QStringLiteral("field2")];
    QVERIFY(field2);
    m_document->editFormText(0, field2, QStringLiteral("20"), 0, 0, 0);

    Okular::FormFieldText *field3 = fields[QStringLiteral("field3")];
    QVERIFY(field3);
    m_document->editFormText(0, field3, QStringLiteral("30"), 0, 0, 0);

    // Verify the results
    QCOMPARE(fields[QStringLiteral("Sum")]->text(), QStringLiteral("60"));
    QCOMPARE(fields[QStringLiteral("AVG")]->text(), QStringLiteral("20"));
    QCOMPARE(fields[QStringLiteral("Prod")]->text(), QStringLiteral("6,000"));
    QCOMPARE(fields[QStringLiteral("Min")]->text(), QStringLiteral("10"));
    QCOMPARE(fields[QStringLiteral("Max")]->text(), QStringLiteral("30"));

    // Verify that Sum was not recalculated after set without edit
    QCOMPARE(fields[QStringLiteral("Sum")]->text(), QStringLiteral("60"));

    // Test that multiplication with zero works
    m_document->editFormText(0, field2, QStringLiteral("0"), 0, 0, 0);
    QCOMPARE(fields[QStringLiteral("Prod")]->text(), QStringLiteral("0"));

    // Test that updating the field also worked with sum
    QCOMPARE(fields[QStringLiteral("Sum")]->text(), QStringLiteral("40"));

    // Test that undo / redo works
    QVERIFY(m_document->canUndo());
    m_document->undo();
    QCOMPARE(fields[QStringLiteral("Sum")]->text(), QStringLiteral("60"));

    QVERIFY(m_document->canRedo());
    m_document->redo();
    QCOMPARE(fields[QStringLiteral("Sum")]->text(), QStringLiteral("40"));
}

QTEST_MAIN(CalculateTextTest)
#include "calculatetexttest.moc"
