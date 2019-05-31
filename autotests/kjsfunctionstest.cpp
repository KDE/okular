/***************************************************************************
 *   Copyright (C) 2018 by Intevation GmbH <intevation@intevation.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtTest>

#include <QAbstractItemModel>
#include <QMap>
#include <QMimeType>
#include <QMimeDatabase>
#include <QThread>
#include "../settings_core.h"
#include "core/document.h"
#include "core/scripter.h"
#include <core/script/event_p.h>
#include <core/page.h>
#include <core/form.h>

#include "../generators/poppler/config-okular-poppler.h"

class KJSFunctionsTest: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testNumFields();
    void testNthFieldName();
    void testDisplay();
    void testSetClearInterval();
    void testSetClearTimeOut();
    void testGetOCGs();
    void testOCGSetState();
    void cleanupTestCase();

private:
    Okular::Document *m_document;
    QMap<QString, Okular::FormField*> m_fields;
};

void KJSFunctionsTest::initTestCase()
{
    Okular::SettingsCore::instance( QStringLiteral("kjsfunctionstest") );
    m_document = new Okular::Document( nullptr );

    const QString testFile = QStringLiteral( KDESRCDIR "data/kjsfunctionstest.pdf" );
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile( testFile );
    QCOMPARE( m_document->openDocument( testFile, QUrl(), mime), Okular::Document::OpenSuccess );

    const Okular::Page* page = m_document->page( 0 );
    for ( Okular::FormField *ff: page->formFields() )
    {
        m_fields.insert( ff->name(),  ff );
    }
}

void KJSFunctionsTest::testNumFields()
{
    QString result = m_document->executeScript( "Doc.numFields" );
    QCOMPARE( "31", result );
}

void KJSFunctionsTest::testNthFieldName()
{
    for(int i = 0;i < 21;++i)
    {
        QString result = m_document->executeScript( QString( "Doc.getNthFieldName(%1)" ).arg( i ) );
        QCOMPARE( result, QString( "0.%1" ).arg(i) );
    }
}

void KJSFunctionsTest::testDisplay()
{
    m_document->executeScript( "field = Doc.getField(\"0.0\");field.display=display.hidden;\
        field = Doc.getField(\"0.10\");field.display=display.visible;" );
    QCOMPARE( false, m_fields["0.0"]->isVisible() );
    QCOMPARE( true, m_fields["0.10"]->isVisible() );
    
    m_document->executeScript( "field = Doc.getField(\"0.10\");field.display=display.hidden;\
        field = Doc.getField(\"0.15\");field.display=display.visible;" );
    QCOMPARE( false, m_fields["0.10"]->isVisible() );
    QCOMPARE( true, m_fields["0.15"]->isVisible() );

    m_document->executeScript( "field = Doc.getField(\"0.15\");field.display=display.hidden;\
        field = Doc.getField(\"0.20\");field.display=display.visible;" );
    QCOMPARE( false, m_fields["0.15"]->isVisible() );
    QCOMPARE( true, m_fields["0.20"]->isVisible() );

    m_document->executeScript( "field = Doc.getField(\"0.20\");field.display=display.hidden;\
        field = Doc.getField(\"0.0\");field.display=display.visible;" );
    QCOMPARE( false, m_fields["0.20"]->isVisible() );
    QCOMPARE( true, m_fields["0.0"]->isVisible() );
}

void delay()
{
    QTime dieTime= QTime::currentTime().addSecs( 2 );
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void KJSFunctionsTest::testSetClearInterval()
{
    QString result = m_document->executeScript( "obj = new Object();obj.idx=0;\
        obj.inc=function(){obj.idx = obj.idx + 1;};\
        intv = app.setInterval('obj.inc()', 450);obj.idx;" );
    QCOMPARE( "0", result );
    delay();

    result = m_document->executeScript( "app.clearInterval(intv);obj.idx;");
    QCOMPARE( "4", result );
}

void KJSFunctionsTest::testSetClearTimeOut()
{
    QString result = m_document->executeScript( "intv = app.setTimeOut('obj.inc()', 1);obj.idx;" );
    QCOMPARE( "4", result );
    delay();
    
    result = m_document->executeScript( "obj.idx;" );
    QCOMPARE( "5", result );
    
    result = m_document->executeScript( "intv = app.setTimeOut('obj.inc()', 2000);obj.idx;" );
    QCOMPARE( "5", result );
    
    result = m_document->executeScript( "app.clearTimeOut(intv);obj.idx;" );
    QCOMPARE( "5", result );
    delay();
    QCOMPARE( "5", result );
}

void KJSFunctionsTest::testGetOCGs()
{
    QAbstractItemModel *model = m_document->layersModel();

    QString result = m_document->executeScript( "var ocg = this.getOCGs(this.pageNum);\
        ocgName = ocg[0].name;" );
    QCOMPARE( model->data( model->index( 0, 0 ), Qt::DisplayRole ).toString() , result );
    
    result = m_document->executeScript( "ocgName = ocg[1].name;" );
    QCOMPARE( model->data( model->index( 1, 0 ), Qt::DisplayRole ).toString() , result );
    
    result = m_document->executeScript( "ocgName = ocg[2].name;" );
    QCOMPARE( model->data( model->index( 2, 0 ), Qt::DisplayRole ).toString() , result );
    
    result = m_document->executeScript( "ocgName = ocg[3].name;" );
    QCOMPARE( model->data( model->index( 3, 0 ), Qt::DisplayRole ).toString() , result );

    result = m_document->executeScript( "ocgName = ocg[0].initState;" );
    QCOMPARE( model->data( model->index( 0, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false"
        , result );
    
    result = m_document->executeScript( "ocgName = ocg[1].initState;" );
    QCOMPARE( model->data( model->index( 1, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false"
        , result );
    
    result = m_document->executeScript( "ocgName = ocg[2].initState;" );
    QCOMPARE( model->data( model->index( 2, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false"
        , result );
    
    result = m_document->executeScript( "ocgName = ocg[3].initState;" );
    QCOMPARE( model->data( model->index( 3, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false"
        , result );
}

void KJSFunctionsTest::testOCGSetState()
{
    QAbstractItemModel *model = m_document->layersModel();

    QString result = m_document->executeScript( "ocgName = ocg[0].state;" );
    QCOMPARE( model->data( model->index( 0, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false", result );
    
    result = m_document->executeScript( "ocg[0].state = false;ocgName = ocg[0].state;");
    QCOMPARE( model->data( model->index( 0, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false", result );

    result = m_document->executeScript( "ocgName = ocg[1].state;" );
    QCOMPARE( model->data( model->index( 1, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false", result );
    
    result = m_document->executeScript( "ocg[1].state = false;ocgName = ocg[1].state;");
    QCOMPARE( model->data( model->index( 1, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false", result );
    
    result = m_document->executeScript( "ocgName = ocg[2].state;" );
    QCOMPARE( model->data( model->index( 2, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false", result );
    
    result = m_document->executeScript( "ocg[2].state = true;ocgName = ocg[2].state;");
    QCOMPARE( model->data( model->index( 2, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false", result );

    result = m_document->executeScript( "ocgName = ocg[3].state;" );
    QCOMPARE( model->data( model->index( 3, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false", result );
    
    result = m_document->executeScript( "ocg[3].state = true;ocgName = ocg[3].state;");
    QCOMPARE( model->data( model->index( 3, 0 ), Qt::CheckStateRole ).toBool() ? "true" : "false", result );
}

void KJSFunctionsTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}


QTEST_MAIN( KJSFunctionsTest )
#include "kjsfunctionstest.moc"
