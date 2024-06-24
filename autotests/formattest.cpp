/*
    SPDX-FileCopyrightText: 2019 João Netto <joaonetto901@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include "../settings_core.h"
#include <QLocale>
#include <QMap>
#include <QMimeDatabase>
#include <QMimeType>
#include <core/document.h>
#include <core/form.h>
#include <core/page.h>

class FormatTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testTimeFormat();
    void testTimeFormat_data();
    void testSpecialFormat();
    void testSpecialFormat_data();
    void testFocusInAction();
    void testFocusInAction_data();
    void testFocusOutAction();
    void testFocusOutAction_data();
    void testNumberFormat();
    void testNumberFormat_data();
    void testPercentFormat();
    void testPercentFormat_data();
    void testDateFormat();
    void testDateFormat_data();

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
        if (fft) {
            m_formattedText = fft->text();
        }
    });

    const Okular::Page *page = m_document->page(0);
    const QList<Okular::FormField *> pageFormFields = page->formFields();
    for (Okular::FormField *ff : pageFormFields) {
        m_fields.insert(ff->name(), ff);
    }
}

void FormatTest::testTimeFormat()
{
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(QString, result);

    Okular::FormField *ff = m_fields[fieldName];
    Okular::FormFieldText *fft = static_cast<Okular::FormFieldText *>(ff);
    fft->setText(text);
    m_document->processFormatAction(ff->additionalAction(Okular::FormField::FormatField), ff);

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
    QFETCH(QString, result);

    Okular::FormField *ff = m_fields[fieldName];
    Okular::FormFieldText *fft = static_cast<Okular::FormFieldText *>(ff);
    fft->setText(text);
    m_document->processFormatAction(ff->additionalAction(Okular::FormField::FormatField), ff);

    QCOMPARE(m_formattedText, result);
}

void FormatTest::testSpecialFormat_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("result");

    // The tests which have invalid edited, keep the same value as when it was formatted before.
    QTest::newRow("field validated but not changed") << QStringLiteral("CEP") << QStringLiteral("12345") << QString(QLatin1String(""));
    QTest::newRow("field invalid but not changed") << QStringLiteral("CEP") << QStringLiteral("123456") << QString(QLatin1String(""));
    QTest::newRow("field formatted and changed") << QStringLiteral("8Digits") << QStringLiteral("123456789") << QStringLiteral("12345-6789");
    QTest::newRow("field invalid 10 digits") << QStringLiteral("8Digits") << QStringLiteral("1234567890") << QStringLiteral("12345-6789");
    QTest::newRow("field formatted telephone") << QStringLiteral("telefone") << QStringLiteral("1234567890") << QStringLiteral("(123) 456-7890");
    QTest::newRow("field invalid telephone") << QStringLiteral("telefone") << QStringLiteral("12345678900") << QStringLiteral("(123) 456-7890");
    QTest::newRow("field formatted SSN") << QStringLiteral("CPF") << QStringLiteral("123456789") << QStringLiteral("123-45-6789");
    QTest::newRow("field invalid SSN") << QStringLiteral("CPF") << QStringLiteral("1234567890") << QStringLiteral("123-45-6789");
}

void FormatTest::testFocusInAction()
{
    QFETCH(QString, result);
    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_fields[QStringLiteral("Validate/Focus")]);

    m_document->processFocusAction(fft->additionalAction(Okular::Annotation::FocusIn), fft);
    QCOMPARE(fft->text(), result);
}

void FormatTest::testFocusInAction_data()
{
    QTest::addColumn<QString>("result");

    QTest::newRow("when focuses") << QStringLiteral("No");
}

void FormatTest::testFocusOutAction()
{
    QFETCH(QString, text);
    QFETCH(QString, result);
    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_fields[QStringLiteral("Validate/Focus")]);

    fft->setText(text);
    m_document->processFocusAction(fft->additionalAction(Okular::Annotation::FocusOut), fft);
    QCOMPARE(fft->text(), result);
}

void FormatTest::testFocusOutAction_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("result");

    QTest::newRow("valid text was set") << QStringLiteral("123") << QStringLiteral("valid");
    QTest::newRow("invalid text was set") << QStringLiteral("abc") << QStringLiteral("invalid");
}

void FormatTest::testNumberFormat()
{
    m_formattedText = QString();
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(QString, result);

    Okular::FormField *ff = m_fields[fieldName];
    Okular::FormFieldText *fft = static_cast<Okular::FormFieldText *>(ff);
    fft->setText(text);
    m_document->processFormatAction(ff->additionalAction(Okular::FormField::FormatField), ff);

    QCOMPARE(m_formattedText, result);
}

void FormatTest::testNumberFormat_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("result");

    QTest::newRow("EUR on left") << QStringLiteral("number1") << QStringLiteral("1.20") << QStringLiteral("€ 1.20");
    QTest::newRow("EUR on left with comma") << QStringLiteral("number1") << QStringLiteral("1234.20") << QStringLiteral("€ 1,234.20");
    QTest::newRow("EUR on right") << QStringLiteral("number2") << QStringLiteral("1.20") << QStringLiteral("1.20 €");
    QTest::newRow("EUR on right without comma") << QStringLiteral("number2") << QStringLiteral("1234.20") << QStringLiteral("1234.20 €");
    QTest::newRow("EUR on left using comma sep") << QStringLiteral("number3") << QStringLiteral("1,20") << QStringLiteral("€ 1,20");
    QTest::newRow("EUR on left using comma sep and thousands with dot") << QStringLiteral("number3") << QStringLiteral("1234,20") << QStringLiteral("€ 1.234,20");
    QTest::newRow("EUR on right with comma") << QStringLiteral("number4") << QStringLiteral("1,20") << /*true <<*/ QStringLiteral("1,20 €");
    QTest::newRow("EUR on right with dot sep without thousands sep") << QStringLiteral("number4") << QStringLiteral("1234,20") << QStringLiteral("1234,20 €");
}

void FormatTest::testPercentFormat()
{
    m_formattedText = QString();
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(QString, result);

    Okular::FormField *ff = m_fields[fieldName];
    Okular::FormFieldText *fft = static_cast<Okular::FormFieldText *>(ff);
    fft->setText(text);
    m_document->processFormatAction(ff->additionalAction(Okular::FormField::FormatField), ff);

    QCOMPARE(m_formattedText, result);
}

void FormatTest::testPercentFormat_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("result");

    QTest::newRow(". as decimal; , as thousand with 6 digits") << QStringLiteral("pct1") << QStringLiteral("1") << QStringLiteral("100.00%");
    QTest::newRow(". as decimal; , as thousand with 4 digits") << QStringLiteral("pct1") << QStringLiteral("12.34") << QStringLiteral("1,234.00%");
    QTest::newRow(". as decimal; no thousand sep with 6 digits") << QStringLiteral("pct2") << QStringLiteral("1") << QStringLiteral("100.00%");
    QTest::newRow(". as decimal; no thousand sep with 4 digits") << QStringLiteral("pct2") << QStringLiteral("12.34") << QStringLiteral("1234.00%");
    QTest::newRow(", as decimal; . as thousand sep with 6 digits") << QStringLiteral("pct3") << QStringLiteral("1") << QStringLiteral("100,00%");
    QTest::newRow(", as decimal; . as thousand sep with 4 digits") << QStringLiteral("pct3") << QStringLiteral("12,34") << QStringLiteral("1.234,00%");
    QTest::newRow(", as decimal; no thousand sep with 6 digits") << QStringLiteral("pct4") << QStringLiteral("1") << QStringLiteral("100,00%");
    QTest::newRow(", as decimal; no thousand sep with 4 digits") << QStringLiteral("pct4") << QStringLiteral("12,34") << QStringLiteral("1234,00%");
    QTest::newRow(". as decimal; ’ as thousand sep with 6 digits") << QStringLiteral("pct5") << QStringLiteral("1") << QStringLiteral("100.00%");
    QTest::newRow(". as decimal; ’ as thousand sep with 4 digits") << QStringLiteral("pct5") << QStringLiteral("12,34") << QStringLiteral("1’234.00%"); // The thousand separator is an apostrophe symbol not single quote
}

void FormatTest::testDateFormat()
{
    m_formattedText = QString();
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(QString, result);

    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_fields[fieldName]);
    fft->setText(text);
    m_document->processFormatAction(fft->additionalAction(Okular::FormField::FormatField), fft);

    QCOMPARE(m_formattedText, result);
}

void FormatTest::testDateFormat_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("result");

    QTest::newRow("d/m with full date") << QStringLiteral("data1") << QStringLiteral("25/06/2024") << QStringLiteral("25/6");
    QTest::newRow("d/m with month in words") << QStringLiteral("data1") << QStringLiteral("25th June 2024") << QStringLiteral("25/6");
    QTest::newRow("d/m with random characters at end") << QStringLiteral("data1") << QStringLiteral("25 - 06 - 24 bleh bleh") << QStringLiteral("25/6");
    QTest::newRow("dd-mmm-yy wtih time info") << QStringLiteral("data2") << QStringLiteral("25/06/2003 20:08 am") << QStringLiteral("25-Jun-03");
    QTest::newRow("dd-mmm-yy with spaces") << QStringLiteral("data2") << QStringLiteral("25 06 2003") << QStringLiteral("25-Jun-03");
    QTest::newRow("d/m/yyyy with padding in months") << QStringLiteral("data3") << QStringLiteral("25/06/2024") << QStringLiteral("25/6/2024");
    QTest::newRow("dd-mm-yy with leap date") << QStringLiteral("data4") << QStringLiteral("29/2/2024") << QStringLiteral("29-02-24");
    QTest::newRow("dd/mm/yyyy with random characters in between") << QStringLiteral("data5") << QStringLiteral("25 abc 06 def 2024") << QStringLiteral("25/06/2024");
    QTest::newRow("mm/yy with month name and year") << QStringLiteral("data6") << QStringLiteral("June 2024") << QStringLiteral("06/24");
    QTest::newRow("mm/yy with month and year in nums") << QStringLiteral("data6") << QStringLiteral("6/24") << QStringLiteral("06/24");
    QTest::newRow("mm/yyyy with short month name and year") << QStringLiteral("data7") << QStringLiteral("Aug 2024") << QStringLiteral("08/2024");
    QTest::newRow("mm/yyyy with date in words") << QStringLiteral("data7") << QStringLiteral("6/2024") << QStringLiteral("06/2024");
    QTest::newRow("d-mmm with date and month name") << QStringLiteral("data8") << QStringLiteral("13 June") << QStringLiteral("13-Jun");
    QTest::newRow("d-mmm with date, month and year") << QStringLiteral("data8") << QStringLiteral("25/06/2024") << QStringLiteral("25-Jun");
    QTest::newRow("d-mmm with half month name") << QStringLiteral("data8") << QStringLiteral("Dec 13") << QStringLiteral("13-Dec");
    QTest::newRow("d-mmm-yy with input in dd/mm/yy") << QStringLiteral("data9") << QStringLiteral("13/08/2010") << QStringLiteral("13-Aug-10");
    QTest::newRow("d-mmm-yy with month name") << QStringLiteral("data9") << QStringLiteral("25th of June 2024") << QStringLiteral("25-Jun-24");
    QTest::newRow("d mmm, yyyy with input in dd/mm/yy") << QStringLiteral("data19") << QStringLiteral("25/06/2024") << QStringLiteral("25 Jun, 2024");
    QTest::newRow("d/m/yy h:MM tt with complete date and time") << QStringLiteral("data21") << QStringLiteral("25/06/2024 20:08") << QStringLiteral("25/6/24 8:08 pm");
    QTest::newRow("d/m/yy h:MM tt with only date") << QStringLiteral("data21") << QStringLiteral("25/06/2024") << QStringLiteral("25/6/24 12:00 am");
    QTest::newRow("d-mmm-yyyy with padding in input date") << QStringLiteral("data10") << QStringLiteral("06/12/1921") << QStringLiteral("6-Dec-1921");
    QTest::newRow("d-mmm-yyyy with input in d/m/yyyy") << QStringLiteral("data10") << QStringLiteral("1/8/2010") << QStringLiteral("1-Aug-2010");
    QTest::newRow("d-mmm-yy with input in d/m/yyyy") << QStringLiteral("data11") << QStringLiteral("13/6/2024") << QStringLiteral("13-Jun-24");
    QTest::newRow("dd-mmm-yyyy with input in d/m/yyyy") << QStringLiteral("data12") << QStringLiteral("1/1/2011") << QStringLiteral("01-Jan-2011");
    // Skip data13, same input already done in data4
    QTest::newRow("dd-mm-yyyy with input in d/m/yyyy") << QStringLiteral("data14") << QStringLiteral("1/1/2011") << QStringLiteral("01-01-2011");
    QTest::newRow("mmm-yy full month name") << QStringLiteral("data15") << QStringLiteral("October 2018") << QStringLiteral("Oct-18");
    QTest::newRow("mmmm-yy partial month name") << QStringLiteral("data16") << QStringLiteral("Oct 2018") << QStringLiteral("October-18");
    QTest::newRow("mmmm-yy input format : mm/yy") << QStringLiteral("data16") << QStringLiteral("09/17") << QStringLiteral("September-17");
    QTest::newRow("d mmm, yyyy with input in dd/mm/yyyy") << QStringLiteral("data18") << QStringLiteral("13/08/2002") << QStringLiteral("13 Aug, 2002");
    QTest::newRow("d/m/yy h:MM tt with complete datetime as input") << QStringLiteral("data20") << QStringLiteral("7/2/1991 20:08") << QStringLiteral("7/2/91 8:08 pm");
    QTest::newRow("d/m/yy h:MM tt with complete datetime as input2") << QStringLiteral("data20") << QStringLiteral("15/3/2018 5:10 am") << QStringLiteral("15/3/18 5:10 am");
    QTest::newRow("d/m/yy HH:MM with datetime including seconds") << QStringLiteral("data22") << QStringLiteral("25/02/2020 13:13:13") << QStringLiteral("25/2/20 13:13");
    QTest::newRow("d/m/yyyy HH:MM with datetime including seconds") << QStringLiteral("data24") << QStringLiteral("13/10/1966 13:13:13") << QStringLiteral("13/10/1966 13:13");
}

void FormatTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}

QTEST_MAIN(FormatTest)
#include "formattest.moc"
