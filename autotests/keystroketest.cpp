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
#include <qtestcase.h>

class KeystrokeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testCommit();
    void testKeystroke();
    void testTimeKeystrokeNoCommit();
    void testTimeKeystrokeNoCommit_data();
    void testTimeKeystrokeCommit();
    void testTimeKeystrokeCommit_data();
    void testSpecialKeystrokeNoCommit();
    void testSpecialKeystrokeNoCommit_data();
    void testSpecialKeystrokeCommit();
    void testSpecialKeystrokeCommit_data();
    void testPercentKeystrokeNoCommit();
    void testPercentKeystrokeNoCommit_data();
    void testPercentKeystrokeCommit();
    void testPercentKeystrokeCommit_data();

private:
    Okular::Document *m_genericTestsDocument;
    QMap<QString, Okular::FormField *> m_genericTestsFields;
    Okular::Document *m_AFMethodsTestsDocument;
    QMap<QString, Okular::FormField *> m_AFMethodsTestsFields;
};

void KeystrokeTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("keystroketest"));
    m_genericTestsDocument = new Okular::Document(nullptr);

    // Force consistent locale
    QLocale locale(QStringLiteral("en_US"));
    QLocale::setDefault(locale);

    const QString genericTestFile = QStringLiteral(KDESRCDIR "data/keystroketest.pdf");
    QMimeDatabase db;
    const QMimeType genericTestMime = db.mimeTypeForFile(genericTestFile);
    QCOMPARE(m_genericTestsDocument->openDocument(genericTestFile, QUrl(), genericTestMime), Okular::Document::OpenSuccess);

    const Okular::Page *genericTestPage = m_genericTestsDocument->page(0);
    const QList<Okular::FormField *> genericTestPageFormFields = genericTestPage->formFields();
    for (Okular::FormField *ff : genericTestPageFormFields) {
        m_genericTestsFields.insert(ff->name(), ff);
    }

    m_AFMethodsTestsDocument = new Okular::Document(nullptr);
    const QString AFMethodsTestFile = QStringLiteral(KDESRCDIR "data/formattest.pdf");
    const QMimeType AFMethodsTestMime = db.mimeTypeForFile(AFMethodsTestFile);
    QCOMPARE(m_AFMethodsTestsDocument->openDocument(AFMethodsTestFile, QUrl(), AFMethodsTestMime), Okular::Document::OpenSuccess);

    const Okular::Page *AFMethodsTestPage = m_AFMethodsTestsDocument->page(0);
    const QList<Okular::FormField *> AFMethodsTestPageFormFields = AFMethodsTestPage->formFields();
    for (Okular::FormField *ff : AFMethodsTestPageFormFields) {
        m_AFMethodsTestsFields.insert(ff->name(), ff);
    }
}

void KeystrokeTest::testCommit()
{
    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_genericTestsFields[QStringLiteral("field2")]);

    // text that will be accepted
    fft->setText(QStringLiteral("Lorem ipsum"));
    m_genericTestsDocument->processKeystrokeCommitAction(fft->additionalAction(Okular::FormField::FieldModified), fft);
    QCOMPARE(fft->text(), QStringLiteral("Lorem ipsum"));

    // text that will be rejected
    fft->setText(QStringLiteral("foo"));
    m_genericTestsDocument->processKeystrokeCommitAction(fft->additionalAction(Okular::FormField::FieldModified), fft);
    QEXPECT_FAIL("", "reset to commited value not implemented", Continue);
    QCOMPARE(fft->text(), QStringLiteral("Lorem ipsum"));
}

void KeystrokeTest::testKeystroke()
{
    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_genericTestsFields[QStringLiteral("field3")]);

    // accept
    m_genericTestsDocument->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, QStringLiteral("hello"), 0, 0);
    QCOMPARE(fft->text(), QStringLiteral("hello"));

    // accept
    m_genericTestsDocument->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, QStringLiteral("e"), 0, 5);
    QCOMPARE(fft->text(), QStringLiteral("e"));

    // accept
    m_genericTestsDocument->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, QStringLiteral("ee"), 1, 1);
    QCOMPARE(fft->text(), QStringLiteral("ee"));

    // accept
    m_genericTestsDocument->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, QStringLiteral("eee"), 2, 2);
    QCOMPARE(fft->text(), QStringLiteral("eee"));

    // reject
    m_genericTestsDocument->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, QStringLiteral("eeef"), 3, 3);
    QCOMPARE(fft->text(), QStringLiteral("eee"));
}

void KeystrokeTest::testTimeKeystrokeNoCommit()
{
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(int, selStart);
    QFETCH(int, selEnd);
    QFETCH(QString, result);

    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_AFMethodsTestsFields[fieldName]);
    m_AFMethodsTestsDocument->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, text, selStart, selEnd);

    QCOMPARE(fft->text(), result);
}

void KeystrokeTest::testTimeKeystrokeNoCommit_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("selStart");
    QTest::addColumn<int>("selEnd");
    QTest::addColumn<QString>("result");

    QTest::newRow("AM") << QStringLiteral("time1") << QStringLiteral("am") << 0 << 0 << QStringLiteral("am");
    QTest::newRow("PM") << QStringLiteral("time1") << QStringLiteral("pm") << 0 << 2 << QStringLiteral("pm");
    QTest::newRow("hh:mm") << QStringLiteral("time1") << QStringLiteral("20:08") << 0 << 2 << QStringLiteral("20:08");
    QTest::newRow("hh:mm:ss") << QStringLiteral("time1") << QStringLiteral("20:08:04") << 0 << 5 << QStringLiteral("20:08:04");
    QTest::newRow("hh:mm  pm") << QStringLiteral("time1") << QStringLiteral("20:08  pm") << 5 << 8 << QStringLiteral("20:08  pm");
    QTest::newRow("other characters") << QStringLiteral("time1") << QStringLiteral("alien") << 0 << 9 << QStringLiteral("20:08  pm");

    // TODO add more tests for rejecting strings
}

void KeystrokeTest::testTimeKeystrokeCommit()
{
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(QString, result);

    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_AFMethodsTestsFields[fieldName]);
    fft->setText(text);
    m_AFMethodsTestsDocument->processKeystrokeCommitAction(fft->additionalAction(Okular::FormField::FieldModified), fft);

    QCOMPARE(fft->text(), result);
}

void KeystrokeTest::testTimeKeystrokeCommit_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("result");

    QTest::newRow("hh:mm") << QStringLiteral("time1") << QStringLiteral("20:08") << QStringLiteral("20:08");
    QTest::newRow("hh:mm:ss") << QStringLiteral("time1") << QStringLiteral("20:08:12") << QStringLiteral("20:08:12");
    QTest::newRow("hh:m:s") << QStringLiteral("time1") << QStringLiteral("20:0:1") << QStringLiteral("20:0:1");
    QTest::newRow("hh:mm:ss am") << QStringLiteral("time1") << QStringLiteral("20:08:12 am") << QStringLiteral("20:08:12 am");
}

void KeystrokeTest::testSpecialKeystrokeNoCommit()
{
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(int, selStart);
    QFETCH(int, selEnd);
    QFETCH(QString, result);

    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_AFMethodsTestsFields[fieldName]);
    m_AFMethodsTestsDocument->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, text, selStart, selEnd);

    QCOMPARE(fft->text(), result);
}

void KeystrokeTest::testSpecialKeystrokeNoCommit_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("selStart");
    QTest::addColumn<int>("selEnd");
    QTest::addColumn<QString>("result");

    // zip code
    QTest::newRow("zip accept") << QStringLiteral("CEP") << QStringLiteral("12345") << 0 << 0 << QStringLiteral("12345");
    QTest::newRow("zip reject/extra length") << QStringLiteral("CEP") << QStringLiteral("123456") << 5 << 5 << QStringLiteral("12345");
    QTest::newRow("zip reject/invalid char") << QStringLiteral("CEP") << QStringLiteral("abcd") << 0 << 5 << QStringLiteral("12345");

    // zip+4 code
    QTest::newRow("zip+4 accept/all nums") << QStringLiteral("8Digits") << QStringLiteral("123456789") << 0 << 0 << QStringLiteral("123456789");
    QTest::newRow("zip+4 accept/. as sep") << QStringLiteral("8Digits") << QStringLiteral("12345.67") << 0 << 9 << QStringLiteral("12345.67");
    QTest::newRow("zip+4 accept/- as separator") << QStringLiteral("8Digits") << QStringLiteral("12345-67") << 0 << 8 << QStringLiteral("12345-67");
    QTest::newRow("zip+4 accept/' ' as separator partial") << QStringLiteral("8Digits") << QStringLiteral("123 6789") << 0 << 8 << QStringLiteral("123 6789");
    QTest::newRow("zip+4 reject/more chars after separator") << QStringLiteral("8Digits") << QStringLiteral("123 67890") << 8 << 8 << QStringLiteral("123 6789");
    QTest::newRow("zip+4 reject/invalid char") << QStringLiteral("8Digits") << QStringLiteral("123 6789abcd") << 8 << 8 << QStringLiteral("123 6789");

    // phone
    QTest::newRow("phone accept/all nums") << QStringLiteral("telefone") << QStringLiteral("1234567890") << 0 << 0 << QStringLiteral("1234567890");
    QTest::newRow("phone accept/parenthesis") << QStringLiteral("telefone") << QStringLiteral("(123 45") << 0 << 10 << QStringLiteral("(123 45");
    QTest::newRow("phone accept/' ' and hyphen both") << QStringLiteral("telefone") << QStringLiteral("123-456 7890") << 0 << 7 << QStringLiteral("123-456 7890");
    QTest::newRow("phone accept/. as sep") << QStringLiteral("telefone") << QStringLiteral("123.456.7890") << 0 << 12 << QStringLiteral("123.456.7890");
    QTest::newRow("phone reject/many sep") << QStringLiteral("telefone") << QStringLiteral("1-23-45-67") << 0 << 12 << QStringLiteral("123.456.7890");
    QTest::newRow("phone reject/incorrect parenthesis") << QStringLiteral("telefone") << QStringLiteral("(1234)") << 0 << 12 << QStringLiteral("123.456.7890");
    QTest::newRow("phone reject/incorrect spaces") << QStringLiteral("telefone") << QStringLiteral("123   56") << 0 << 12 << QStringLiteral("123.456.7890");
    QTest::newRow("phone reject/invalid chars") << QStringLiteral("telefone") << QStringLiteral("abcd") << 0 << 12 << QStringLiteral("123.456.7890");
    QTest::newRow("phone reject/exceeding length") << QStringLiteral("telefone") << QStringLiteral("123.456.78901") << 12 << 12 << QStringLiteral("123.456.7890");

    // ssn
    QTest::newRow("ssn accept/all nums") << QStringLiteral("CPF") << QStringLiteral("123456789") << 0 << 0 << QStringLiteral("123456789");
    QTest::newRow("ssn accept/' ' and - as sep") << QStringLiteral("CPF") << QStringLiteral("123 45-6789") << 0 << 9 << QStringLiteral("123 45-6789");
    QTest::newRow("ssn accept/. as sep") << QStringLiteral("CPF") << QStringLiteral("123.45.6789") << 0 << 11 << QStringLiteral("123.45.6789");
    QTest::newRow("ssn reject/too many seps") << QStringLiteral("CPF") << QStringLiteral("123.45..6789") << 0 << 11 << QStringLiteral("123.45.6789");
    QTest::newRow("ssn reject/exceeding length") << QStringLiteral("CPF") << QStringLiteral("123.45.67890") << 11 << 11 << QStringLiteral("123.45.6789");
    QTest::newRow("ssn reject/invalid chars") << QStringLiteral("CPF") << QStringLiteral("abcd") << 0 << 11 << QStringLiteral("123.45.6789");
}

void KeystrokeTest::testSpecialKeystrokeCommit()
{
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(QString, result);

    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_AFMethodsTestsFields[fieldName]);
    fft->setText(text);
    m_AFMethodsTestsDocument->processKeystrokeCommitAction(fft->additionalAction(Okular::FormField::FieldModified), fft);

    QCOMPARE(fft->text(), result);
}

void KeystrokeTest::testSpecialKeystrokeCommit_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("result");

    // zip
    QTest::newRow("zip accept") << QStringLiteral("CEP") << QStringLiteral("12345") << QStringLiteral("12345");

    // zip+4
    QTest::newRow("zip+4 accept/all nums") << QStringLiteral("8Digits") << QStringLiteral("123456789") << QStringLiteral("123456789");
    QTest::newRow("zip+4 accept/hyphen as sep") << QStringLiteral("8Digits") << QStringLiteral("12345-6789") << QStringLiteral("12345-6789");
    QTest::newRow("zip+4 accept/. as sep") << QStringLiteral("8Digits") << QStringLiteral("12345.6789") << QStringLiteral("12345.6789");
    QTest::newRow("zip+4 accept/' ' as sep") << QStringLiteral("8Digits") << QStringLiteral("12345 6789") << QStringLiteral("12345 6789");

    // phone
    QTest::newRow("phone accept/all nums") << QStringLiteral("telefone") << QStringLiteral("1234567890") << QStringLiteral("1234567890");
    QTest::newRow("phone accept/with parenthesis") << QStringLiteral("telefone") << QStringLiteral("(123)4567890") << QStringLiteral("(123)4567890");
    QTest::newRow("phone accept/hyphen, spaces and parenthesis") << QStringLiteral("telefone") << QStringLiteral("(123) 456-7890") << QStringLiteral("(123) 456-7890");
    QTest::newRow("phone accept/only hyphens") << QStringLiteral("telefone") << QStringLiteral("123-456-7890") << QStringLiteral("123-456-7890");
    QTest::newRow("phone accept/only dots") << QStringLiteral("telefone") << QStringLiteral("123.456.7890") << QStringLiteral("123.456.7890");

    // ssn
    QTest::newRow("ssn accept/all nums") << QStringLiteral("CPF") << QStringLiteral("123456789") << QStringLiteral("123456789");
    QTest::newRow("ssn accept/hyphens") << QStringLiteral("CPF") << QStringLiteral("123-45-6789") << QStringLiteral("123-45-6789");
    QTest::newRow("ssn accept/hyphens and dots") << QStringLiteral("CPF") << QStringLiteral("123-45.6789") << QStringLiteral("123-45.6789");
    QTest::newRow("ssn accept/spaces") << QStringLiteral("CPF") << QStringLiteral("123 45 6789") << QStringLiteral("123 45 6789");

    // TODO: Add more tests for rejecting strings when feature to restore committed values is implemented.
}

void KeystrokeTest::testPercentKeystrokeNoCommit()
{
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(int, selStart);
    QFETCH(int, selEnd);
    QFETCH(QString, result);

    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_AFMethodsTestsFields[fieldName]);
    m_AFMethodsTestsDocument->processKeystrokeAction(fft->additionalAction(Okular::FormField::FieldModified), fft, text, selStart, selEnd);

    QCOMPARE(fft->text(), result);
}

void KeystrokeTest::testPercentKeystrokeNoCommit_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("selStart");
    QTest::addColumn<int>("selEnd");
    QTest::addColumn<QString>("result");

    QTest::newRow("no decimal separator/accept") << QStringLiteral("pct1") << QStringLiteral("123") << 0 << 0 << QStringLiteral("123");
    QTest::newRow(". used as decimal separator/accept") << QStringLiteral("pct1") << QStringLiteral("1.21") << 1 << 3 << QStringLiteral("1.21");
    QTest::newRow(", used as decimal separator/accept") << QStringLiteral("pct3") << QStringLiteral("1,21") << 0 << 0 << QStringLiteral("1,21");
    QTest::newRow("+ sign used as prefix/accept") << QStringLiteral("pct1") << QStringLiteral("+1.21") << 0 << 4 << QStringLiteral("+1.21");
    QTest::newRow("- sign used as prefix/accept") << QStringLiteral("pct1") << QStringLiteral("-1.2") << 0 << 5 << QStringLiteral("-1.2");
    QTest::newRow("use multiple decimal separator/reject") << QStringLiteral("pct1") << QStringLiteral("-1.2.1") << 4 << 4 << QStringLiteral("-1.2");
    QTest::newRow("use alphabets/reject") << QStringLiteral("pct1") << QStringLiteral("-1.2abc") << 4 << 4 << QStringLiteral("-1.2");
    QTest::newRow("use multiple - sign/reject") << QStringLiteral("pct1") << QStringLiteral("-1.2-1") << 4 << 4 << QStringLiteral("-1.2");
    QTest::newRow("use , in pct1/reject") << QStringLiteral("pct1") << QStringLiteral("-1,2") << 0 << 4 << QStringLiteral("-1.2");
    QTest::newRow("use . in pct3/reject") << QStringLiteral("pct3") << QStringLiteral("1.2") << 0 << 4 << QStringLiteral("1,21");
}

void KeystrokeTest::testPercentKeystrokeCommit()
{
    QFETCH(QString, fieldName);
    QFETCH(QString, text);
    QFETCH(QString, result);

    Okular::FormFieldText *fft = reinterpret_cast<Okular::FormFieldText *>(m_AFMethodsTestsFields[fieldName]);
    fft->setText(text);
    m_AFMethodsTestsDocument->processKeystrokeCommitAction(fft->additionalAction(Okular::FormField::FieldModified), fft);

    QCOMPARE(fft->text(), result);
}

void KeystrokeTest::testPercentKeystrokeCommit_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("result");

    QTest::newRow("no decimal separator/accept") << QStringLiteral("pct1") << QStringLiteral("123") << QStringLiteral("123");
    QTest::newRow(". used as decimal separator/accept") << QStringLiteral("pct1") << QStringLiteral("1.21") << QStringLiteral("1.21");
    QTest::newRow(", used as decimal separator/accept") << QStringLiteral("pct3") << QStringLiteral("1,21") << QStringLiteral("1,21");
    QTest::newRow("+ sign used as prefix/accept") << QStringLiteral("pct1") << QStringLiteral("+1.21") << QStringLiteral("+1.21");
    QTest::newRow("- sign used as prefix/accept") << QStringLiteral("pct1") << QStringLiteral("-1.2") << QStringLiteral("-1.2");

    // TODO add more tests for rejecting strings
}

void KeystrokeTest::cleanupTestCase()
{
    m_genericTestsDocument->closeDocument();
    delete m_genericTestsDocument;
    m_AFMethodsTestsDocument->closeDocument();
    delete m_AFMethodsTestsDocument;
}

QTEST_MAIN(KeystrokeTest)
#include "keystroketest.moc"
