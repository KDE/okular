/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

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
#ifndef HAVE_POPPLER_0_73
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
#endif
}

QTEST_MAIN(SignatureFormTest)
#include "signatureformtest.moc"
