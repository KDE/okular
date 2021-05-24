/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include <QMimeDatabase>
#include <QMimeType>

#include "../settings_core.h"
#include <core/document.h>
#include <core/form.h>
#include <core/page.h>

class SignatureFormTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSignatureForm();

private:
    Okular::Document *m_document;
};

void SignatureFormTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("signatureformtest"));
    m_document = new Okular::Document(nullptr);
}

void SignatureFormTest::cleanupTestCase()
{
    delete m_document;
}

void SignatureFormTest::testSignatureForm()
{
    const QString testFile = QStringLiteral(KDESRCDIR "data/pdf_with_signature.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);
    QCOMPARE(m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess);

    const Okular::Page *page = m_document->page(0);
    QLinkedList<Okular::FormField *> pageFields = page->formFields();
    QCOMPARE(pageFields.size(), 1);
    QCOMPARE(pageFields.first()->type(), Okular::FormField::FormSignature);

    Okular::FormFieldSignature *sf = static_cast<Okular::FormFieldSignature *>(pageFields.first());
    QCOMPARE(sf->signatureType(), Okular::FormFieldSignature::AdbePkcs7detached);
}

QTEST_MAIN(SignatureFormTest)
#include "signatureformtest.moc"
