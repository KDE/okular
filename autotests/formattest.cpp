/***************************************************************************
 *   Copyright (C) 2019 by João Netto <joaonetto901@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

#include <QMap>
#include <QMimeType>
#include <QMimeDatabase>
#include "../settings_core.h"
#include <core/document.h>
#include <core/page.h>
#include <core/form.h>
#include <QLocale>

#include "../generators/poppler/config-okular-poppler.h"

class FormatTest: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testTimeFormat();
    void testSpecialFormat();
private:

    Okular::Document *m_document;
    QMap<QString, Okular::FormField*> m_fields;
    QString m_formattedText;
};

void FormatTest::initTestCase()
{
    Okular::SettingsCore::instance( QStringLiteral( "formattest" ) );
    m_document = new Okular::Document( nullptr );

    const QString testFile = QStringLiteral( KDESRCDIR "data/formattest.pdf" );
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile( testFile );
    QCOMPARE( m_document->openDocument( testFile, QUrl(), mime), Okular::Document::OpenSuccess );

    connect( m_document, &Okular::Document::refreshFormWidget, [=]( Okular::FormField * form )
            {
                Okular::FormFieldText *fft = reinterpret_cast< Okular::FormFieldText * >( form );
                if( fft )
                    m_formattedText = fft->text();
            });

    const Okular::Page* page = m_document->page( 0 );
    for ( Okular::FormField *ff: page->formFields() )
    {
        m_fields.insert( ff->name(),  ff );
    }
}

void FormatTest::testTimeFormat()
{
    Okular::FormFieldText *fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ QStringLiteral( "time1" ) ] );
   
    fft->setText( QStringLiteral( "1:20" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "01:20" ), m_formattedText );

    fft->setText( QStringLiteral( "1:20 pm" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "13:20" ), m_formattedText );

    fft->setText( QStringLiteral( "1" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "" ), m_formattedText );

    fft->setText( QStringLiteral( "25:12" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "" ), m_formattedText );

    fft->setText( QStringLiteral( "abcd" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "" ), m_formattedText );

    fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ QStringLiteral( "time2" ) ] );
   
    fft->setText( QStringLiteral( "1:20" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "1:20 am" ), m_formattedText );

    fft->setText( QStringLiteral( "01:20 pm" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "1:20 pm" ), m_formattedText );

    fft->setText( QStringLiteral( "13:20" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "1:20 pm" ), m_formattedText );

    fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ QStringLiteral( "time3" ) ] );
   
    fft->setText( QStringLiteral( "1:20" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "01:20:00" ), m_formattedText );

    fft->setText( QStringLiteral( "1:20:00 pm" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "13:20:00" ), m_formattedText );

    fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ QStringLiteral( "time4" ) ] );
   
    fft->setText( QStringLiteral( "1:20:00" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "1:20:00 am" ), m_formattedText );

    fft->setText( QStringLiteral( "01:20:00 pm" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "1:20:00 pm" ), m_formattedText );

    fft->setText( QStringLiteral( "13:20:00" ) );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( QStringLiteral( "1:20:00 pm" ), m_formattedText );
}

void FormatTest::testSpecialFormat()
{
    Okular::FormFieldText *fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ QStringLiteral( "CEP" ) ] );

    // This field will just become itself, so we test only if keystroke worked.
    fft->setText( QStringLiteral( "12345" ) );
    bool ok = false;
    m_document->processKeystrokeAction( fft->additionalAction( Okular::FormField::FieldModified ), fft, ok );

    QCOMPARE( true, ok );

    fft->setText( QStringLiteral( "123456" ) );
    ok = false;
    m_document->processKeystrokeAction( fft->additionalAction( Okular::FormField::FieldModified ), fft, ok );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( false, ok );

    fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ QStringLiteral( "8Digits" ) ] );

    fft->setText( QStringLiteral( "123456789" ) );
    ok = false;
    m_document->processKeystrokeAction( fft->additionalAction( Okular::FormField::FieldModified ), fft, ok );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( true, ok );
    QCOMPARE( m_formattedText, QStringLiteral( "12345-6789" ) );

    fft->setText( QStringLiteral( "1234567890" ) );
    ok = false;
    m_document->processKeystrokeAction( fft->additionalAction( Okular::FormField::FieldModified ), fft, ok );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( false, ok );
    QCOMPARE( m_formattedText, QStringLiteral( "12345-6789" ) );

    fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ QStringLiteral( "telefone" ) ] );

    fft->setText( QStringLiteral( "1234567890" ) );
    ok = false;
    m_document->processKeystrokeAction( fft->additionalAction( Okular::FormField::FieldModified ), fft, ok );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( true, ok );
    QCOMPARE( m_formattedText, QStringLiteral("(123) 456-7890" ) );

    fft->setText( QStringLiteral( "12345678900" ) );
    ok = false;
    m_document->processKeystrokeAction( fft->additionalAction( Okular::FormField::FieldModified ), fft, ok );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( false, ok );
    QCOMPARE( m_formattedText, QStringLiteral("(123) 456-7890" ) );

    fft = reinterpret_cast< Okular::FormFieldText * >(  m_fields[ QStringLiteral( "CPF" ) ] );

    fft->setText( QStringLiteral( "123456789" ) );
    ok = false;
    m_document->processKeystrokeAction( fft->additionalAction( Okular::FormField::FieldModified ), fft, ok );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( true, ok );
    QCOMPARE( m_formattedText, QStringLiteral( "123-45-6789" ) );

    fft->setText( QStringLiteral( "1234567890" ) );
    ok = false;
    m_document->processKeystrokeAction( fft->additionalAction( Okular::FormField::FieldModified ), fft, ok );
    m_document->processFormatAction( fft->additionalAction( Okular::FormField::FormatField ), fft );

    QCOMPARE( false, ok );
    QCOMPARE( m_formattedText, QStringLiteral( "123-45-6789" ) );
}

void FormatTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}


QTEST_MAIN( FormatTest )
#include "formattest.moc"
