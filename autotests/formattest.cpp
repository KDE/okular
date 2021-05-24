/*
    SPDX-FileCopyrightText: 2019 João Netto <joaonetto901@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QtTest>

#include "../settings_core.h"
#include <QLocale>
#include <QMap>
#include <QMimeDatabase>
#include <QMimeType>
#include <core/document.h>
#include <core/form.h>
#include <core/page.h>

#include "../generators/poppler/config-okular-poppler.h"

class FormatTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testTimeFormat();
    void testTimeFormat_data();
    void testSpecialFormat();
    void testSpecialFormat_data();
    void testFocusAction();
    void testFocusAction_data();
    void testValidateAction();
    void testValidateAction_data();

private:
    Okular::Document *m_document;
    QMap<QString, Okular::FormField *> m_fields;
    QString m_formattedText;
};

void FormatTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("formattest"));
    m_document = new Okular::Document(nullptr);

    // Force consistent locale
    QLocale locale(QStringLiteral("en_US"));
    QLocale::setDefault(locale);

    const QString testFile = QStringLiteral(KDESRCDIR "data/formattest.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);
    QCOMPARE(m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess);

    connect(m_document, &Okular::Document::refreshFormWidget, this, [this](Okular::FormField *form) {
        Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(form);
        if (fft)
            m_formattedText = fft->text();
    });

    const Okular::Page *page = m_document->page(0);
    const QLinkedList<Okular::FormField *> pageFormFields = page->formFields();
    for (Okular::FormField *ff : pageFormFields) {
        m_fields.insert(ff->name(), ff);
    }
}

void FormatTest::testTimeFormat()
{
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(QString, result);

    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_fields[fieldName]);
    fft->setText(text);
    m_document->processFormatAction(fft->additionalAction(Okular::FormField::FormatField), fft);

    QCOMPARE(m_formattedText, result);
}

void FormatTest::testTimeFormat_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("result");

    QTest::newRow("field hh:mm") << QStringLiteral("time1") << QStringLiteral("1:20") << QStringLiteral("01:20");
    QTest::newRow("field hh:mm with pm") << QStringLiteral("time1") << QStringLiteral("1:20 pm") << QStringLiteral("13:20");
    QTest::newRow("field hh:mm invalid one number") << QStringLiteral("time1") << QStringLiteral("1") << QString(QLatin1String(""));
    QTest::newRow("field hh:mm invalid time") << QStringLiteral("time1") << QStringLiteral("25:12") << QString(QLatin1String(""));
    QTest::newRow("field hh:mm invalid only letters") << QStringLiteral("time1") << QStringLiteral("abcd") << QString(QLatin1String(""));
    QTest::newRow("field hh:mm ap") << QStringLiteral("time2") << QStringLiteral("1:20") << QStringLiteral("1:20 am");
    QTest::newRow("field hh:mm ap remove zero") << QStringLiteral("time2") << QStringLiteral("01:20 pm") << QStringLiteral("1:20 pm");
    QTest::newRow("field hh:mm ap change to AM/PM") << QStringLiteral("time2") << QStringLiteral("13:20") << QStringLiteral("1:20 pm");
    QTest::newRow("field hh:mm:ss without seconds") << QStringLiteral("time3") << QStringLiteral("1:20") << QStringLiteral("01:20:00");
    QTest::newRow("field hh:mm:ss with pm") << QStringLiteral("time3") << QStringLiteral("1:20:00 pm") << QStringLiteral("13:20:00");
    QTest::newRow("field hh:mm:ss ap without am") << QStringLiteral("time4") << QStringLiteral("1:20:00") << QStringLiteral("1:20:00 am");
    QTest::newRow("field hh:mm:ss ap remove 0") << QStringLiteral("time4") << QStringLiteral("01:20:00 pm") << QStringLiteral("1:20:00 pm");
    QTest::newRow("field hh:mm:ss ap change to AM/PM") << QStringLiteral("time4") << QStringLiteral("13:20:00") << QStringLiteral("1:20:00 pm");
}

void FormatTest::testSpecialFormat()
{
    m_formattedText = QLatin1String("");
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(bool, edited);
    QFETCH(QString, result);

    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_fields[fieldName]);
    fft->setText(text);
    bool ok = false;
    m_document->processFormatAction(fft->additionalAction(Okular::FormField::FormatField), fft);
    m_document->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, ok);

    QCOMPARE(m_formattedText, result);
    QCOMPARE(ok, edited);
}

void FormatTest::testSpecialFormat_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("edited");
    QTest::addColumn<QString>("result");

    // The tests which have invalid edited, keep the same value as when it was formatted before.
    QTest::newRow("field validated but not changed") << QStringLiteral("CEP") << QStringLiteral("12345") << true << QString(QLatin1String(""));
    QTest::newRow("field invalid but not changed") << QStringLiteral("CEP") << QStringLiteral("123456") << false << QString(QLatin1String(""));
    QTest::newRow("field formatted and changed") << QStringLiteral("8Digits") << QStringLiteral("123456789") << true << QStringLiteral("12345-6789");
    QTest::newRow("field invalid 10 digits") << QStringLiteral("8Digits") << QStringLiteral("1234567890") << false << QStringLiteral("12345-6789");
    QTest::newRow("field formatted telephone") << QStringLiteral("telefone") << QStringLiteral("1234567890") << true << QStringLiteral("(123) 456-7890");
    QTest::newRow("field invalid telephone") << QStringLiteral("telefone") << QStringLiteral("12345678900") << false << QStringLiteral("(123) 456-7890");
    QTest::newRow("field formatted SSN") << QStringLiteral("CPF") << QStringLiteral("123456789") << true << QStringLiteral("123-45-6789");
    QTest::newRow("field invalid SSN") << QStringLiteral("CPF") << QStringLiteral("1234567890") << false << QStringLiteral("123-45-6789");
}

void FormatTest::testFocusAction()
{
    QFETCH(QString, result);
    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_fields[QStringLiteral("Validate/Focus")]);

    m_document->processFocusAction(fft->additionalAction(Okular::Annotation::FocusIn), fft);
    QCOMPARE(fft->text(), result);
}

void FormatTest::testFocusAction_data()
{
    QTest::addColumn<QString>("result");

    QTest::newRow("when focuses") << QStringLiteral("No");
}

void FormatTest::testValidateAction()
{
    QFETCH(QString, text);
    QFETCH(QString, result);
    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_fields[QStringLiteral("Validate/Focus")]);

    fft->setText(text);
    bool ok = false;
    m_document->processValidateAction(fft->additionalAction(Okular::Annotation::FocusOut), fft, ok);
    QCOMPARE(fft->text(), result);
    QVERIFY(ok);
}

void FormatTest::testValidateAction_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("result");

    QTest::newRow("valid text was set") << QStringLiteral("123") << QStringLiteral("valid");
    QTest::newRow("invalid text was set") << QStringLiteral("abc") << QStringLiteral("invalid");
}

void FormatTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}

QTEST_MAIN(FormatTest)
#include "formattest.moc"
