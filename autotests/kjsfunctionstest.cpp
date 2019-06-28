/***************************************************************************
 *   Copyright (C) 2019 by João Netto <joaonetto901@gmail.com>             *
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
#include "core/action.h"
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
#ifdef HAVE_POPPLER_0_79
    void initTestCase();
    void testNthFieldName();
    void testDisplay();
    void testSetClearInterval();
    void testSetClearTimeOut();
    void testGetOCGs();
    void cleanupTestCase();
#endif
private:
    Okular::Document *m_document;
    QMap<QString, Okular::FormField*> m_fields;
};

#ifdef HAVE_POPPLER_0_79

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

void KJSFunctionsTest::testNthFieldName()
{
    for(int i = 0;i < 21;++i)
    {
        Okular::ScriptAction *action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "var field = Doc.getField( Doc.getNthFieldName(%1) );\
                                                                              field.display = display.visible;" ).arg( i ) );
        m_document->processAction( action );
        QCOMPARE( true, m_fields[QString( "0.%1" ).arg(i)]->isVisible() );
        m_fields[QString( "0.%1" ).arg(i)]->setVisible( false ); 
        delete action;
    }
}

void KJSFunctionsTest::testDisplay()
{
    Okular::ScriptAction *action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "field = Doc.getField(\"0.0\");field.display=display.hidden;\
        field = Doc.getField(\"0.10\");field.display=display.visible;" ) );
    m_document->processAction( action );
    QCOMPARE( false, m_fields["0.0"]->isVisible() );
    QCOMPARE( false, m_fields["0.0"]->isPrintable() );
    QCOMPARE( true, m_fields["0.10"]->isVisible() );
    QCOMPARE( true, m_fields["0.10"]->isPrintable() );
    delete action;
    
    action = new Okular::ScriptAction( Okular::JavaScript,
        QStringLiteral( "field = Doc.getField(\"0.10\");field.display=display.noView;\
        field = Doc.getField(\"0.15\");field.display=display.noPrint;" ) );
    m_document->processAction( action );
    QCOMPARE( false, m_fields["0.10"]->isVisible() );
    QCOMPARE( true, m_fields["0.10"]->isPrintable() );
    QCOMPARE( true, m_fields["0.15"]->isVisible() );
    QCOMPARE( false, m_fields["0.15"]->isPrintable() );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript,
        QStringLiteral( "field = Doc.getField(\"0.15\");field.display=display.hidden;\
        field = Doc.getField(\"0.20\");field.display=display.visible;" ) );
    m_document->processAction( action );
    QCOMPARE( false, m_fields["0.15"]->isVisible() );
    QCOMPARE( false, m_fields["0.15"]->isPrintable() );
    QCOMPARE( true, m_fields["0.20"]->isVisible() );
    QCOMPARE( true, m_fields["0.20"]->isPrintable() );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript,
        QStringLiteral( "field = Doc.getField(\"0.20\");field.display=display.hidden;\
        field = Doc.getField(\"0.0\");field.display=display.visible;" ) );
    m_document->processAction( action );
    QCOMPARE( false, m_fields["0.20"]->isVisible() );
    QCOMPARE( false, m_fields["0.20"]->isPrintable() );
    QCOMPARE( true, m_fields["0.0"]->isVisible() );
    QCOMPARE( true, m_fields["0.0"]->isPrintable() );
    delete action;
}

void delay()
{
    QTime dieTime= QTime::currentTime().addSecs( 2 );
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void KJSFunctionsTest::testSetClearInterval()
{
    Okular::ScriptAction *action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "obj = new Object();obj.idx=0;\
        obj.inc=function(){field = Doc.getField(Doc.getNthFieldName(obj.idx));\
        field.display = display.visible;\
        obj.idx = obj.idx + 1;};\
        intv = app.setInterval('obj.inc()', 450);obj.idx;" ) );
    m_document->processAction( action );
    QCOMPARE( m_fields["0.0"]->isVisible() , true  );
    QCOMPARE( m_fields["0.3"]->isVisible() , false );
    delete action;
    delay();

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "app.clearInterval(intv);obj.idx;" ) );
    m_document->processAction( action );
    QCOMPARE( m_fields["0.3"]->isVisible() , true );
    delete action;
}

void KJSFunctionsTest::testSetClearTimeOut()
{
    Okular::ScriptAction *action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "intv = app.setTimeOut('obj.inc()', 1);obj.idx;" ) );
    m_document->processAction( action );
    QCOMPARE( m_fields["0.3"]->isVisible() , true);
    QCOMPARE( m_fields["0.4"]->isVisible() , false );
    delay();
    delete action;
    
    QCOMPARE( m_fields["0.4"]->isVisible() , true );
    
    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral(  "intv = app.setTimeOut('obj.inc()', 2000);obj.idx;" ) );
    m_document->processAction( action );
    QCOMPARE( m_fields["0.4"]->isVisible() , true );
    delete action;
    
    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral(  "app.clearTimeOut(intv);obj.idx;" ) );
    m_document->processAction( action );
    QCOMPARE( m_fields["0.4"]->isVisible() , true );
    delay();
    QCOMPARE( m_fields["0.4"]->isVisible() , true );
    delete action;
}

void KJSFunctionsTest::testGetOCGs()
{
    QAbstractItemModel *model = m_document->layersModel();

    Okular::ScriptAction *action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "var ocg = this.getOCGs(this.pageNum);\
        ocg[0].state = false;" ) );
    m_document->processAction( action );
    QCOMPARE( model->data( model->index( 0, 0 ), Qt::CheckStateRole ).toBool() , false );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[0].state = true;" ) );
    m_document->processAction( action );
    QCOMPARE( model->data( model->index( 0, 0 ), Qt::CheckStateRole ).toBool() , true );
    delete action;
    
    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[1].state = false;" ) );
    m_document->processAction( action );
    QCOMPARE( model->data( model->index( 1, 0 ), Qt::CheckStateRole ).toBool() , false );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[1].state = true;" ) );
    m_document->processAction( action );
    QCOMPARE( model->data( model->index( 1, 0 ), Qt::CheckStateRole ).toBool() , true );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[2].state = false;" ) );
    m_document->processAction( action );
    QCOMPARE( model->data( model->index( 2, 0 ), Qt::CheckStateRole ).toBool() , false );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[2].state = true;" ) );
    m_document->processAction( action );
    QCOMPARE( model->data( model->index( 2, 0 ), Qt::CheckStateRole ).toBool() , true );
    delete action;
    
    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[3].state = false;" ) );
    m_document->processAction( action );
    QCOMPARE( model->data( model->index( 3, 0 ), Qt::CheckStateRole ).toBool() , false );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[3].state = true;" ) );
    m_document->processAction( action );
    QCOMPARE( model->data( model->index( 3, 0 ), Qt::CheckStateRole ).toBool() , true );
    delete action;
}


void KJSFunctionsTest::cleanupTestCase()
{
    m_document->closeDocument();
    delete m_document;
}

#endif

QTEST_MAIN( KJSFunctionsTest )
#include "kjsfunctionstest.moc"
