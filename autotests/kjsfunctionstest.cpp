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
#include <QAbstractButton>
#include <QMessageBox>
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

class MessageBoxHelper : public QObject
{
    Q_OBJECT

public:
    MessageBoxHelper(QMessageBox::StandardButton b, QString message, QMessageBox::Icon icon, QString title, bool hasCheckBox ) 
                     : m_button(b), m_clicked(false), m_message(message), m_icon(icon), m_title(title), m_checkBox(hasCheckBox)
    {
        QTimer::singleShot(0, this, &MessageBoxHelper::closeMessageBox);
    }

    ~MessageBoxHelper()
    {
        QVERIFY(m_clicked);
    }

private slots:
    void closeMessageBox()
    {
        QWidgetList allToplevelWidgets = QApplication::topLevelWidgets();
        QMessageBox *mb = nullptr;
        foreach ( QWidget *w, allToplevelWidgets ) {
            if ( w->inherits( "QMessageBox" ) ) {
                mb = qobject_cast< QMessageBox * >( w );
                QCOMPARE( mb->text(), m_message );
                QCOMPARE( mb->windowTitle(), m_title );
                QCOMPARE( mb->icon(), m_icon );
                QCheckBox *box = mb->checkBox();
                QCOMPARE( box != nullptr, m_checkBox );
                mb->button( m_button )->click();
            }
        }
        if (!mb) {
            QTimer::singleShot(0, this, &MessageBoxHelper::closeMessageBox);
            return;
        }
        m_clicked = true;
    }

private:
    QMessageBox::StandardButton m_button;
    bool m_clicked;
    QString m_message;
    QMessageBox::Icon m_icon;
    QString m_title;
    bool m_checkBox;
};


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
    void testAlert();
    void testPrintD();
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
        QVERIFY( m_fields[QString( "0.%1" ).arg(i)]->isVisible() );
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
    QVERIFY( !m_fields["0.0"]->isVisible() );
    QVERIFY( !m_fields["0.0"]->isPrintable() );
    QVERIFY( m_fields["0.10"]->isVisible() );
    QVERIFY( m_fields["0.10"]->isPrintable() );
    delete action;
    
    action = new Okular::ScriptAction( Okular::JavaScript,
        QStringLiteral( "field = Doc.getField(\"0.10\");field.display=display.noView;\
        field = Doc.getField(\"0.15\");field.display=display.noPrint;" ) );
    m_document->processAction( action );
    QVERIFY( !m_fields["0.10"]->isVisible() );
    QVERIFY( m_fields["0.10"]->isPrintable() );
    QVERIFY( m_fields["0.15"]->isVisible() );
    QVERIFY( !m_fields["0.15"]->isPrintable() );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript,
        QStringLiteral( "field = Doc.getField(\"0.15\");field.display=display.hidden;\
        field = Doc.getField(\"0.20\");field.display=display.visible;" ) );
    m_document->processAction( action );
    QVERIFY( !m_fields["0.15"]->isVisible() );
    QVERIFY( !m_fields["0.15"]->isPrintable() );
    QVERIFY( m_fields["0.20"]->isVisible() );
    QVERIFY( m_fields["0.20"]->isPrintable() );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript,
        QStringLiteral( "field = Doc.getField(\"0.20\");field.display=display.hidden;\
        field = Doc.getField(\"0.0\");field.display=display.visible;" ) );
    m_document->processAction( action );
    QVERIFY( !m_fields["0.20"]->isVisible() );
    QVERIFY( !m_fields["0.20"]->isPrintable() );
    QVERIFY( m_fields["0.0"]->isVisible() );
    QVERIFY( m_fields["0.0"]->isPrintable() );
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
    QVERIFY( m_fields["0.0"]->isVisible() );
    QVERIFY( !m_fields["0.3"]->isVisible() );
    delete action;
    delay();

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "app.clearInterval(intv);obj.idx;" ) );
    m_document->processAction( action );
    QVERIFY( m_fields["0.3"]->isVisible() );
    delete action;
}

void KJSFunctionsTest::testSetClearTimeOut()
{
    Okular::ScriptAction *action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "intv = app.setTimeOut('obj.inc()', 1);obj.idx;" ) );
    m_document->processAction( action );
    QVERIFY( m_fields["0.3"]->isVisible() );
    QVERIFY( !m_fields["0.4"]->isVisible() );
    delay();
    delete action;
    
    QVERIFY( m_fields["0.4"]->isVisible() );
    
    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral(  "intv = app.setTimeOut('obj.inc()', 2000);obj.idx;" ) );
    m_document->processAction( action );
    QVERIFY( m_fields["0.4"]->isVisible() );
    delete action;
    
    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral(  "app.clearTimeOut(intv);obj.idx;" ) );
    m_document->processAction( action );
    QVERIFY( m_fields["0.4"]->isVisible() );
    delay();
    QVERIFY( m_fields["0.4"]->isVisible() );
    delete action;
}

void KJSFunctionsTest::testGetOCGs()
{
    QAbstractItemModel *model = m_document->layersModel();

    Okular::ScriptAction *action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "var ocg = this.getOCGs(this.pageNum);\
        ocg[0].state = false;" ) );
    m_document->processAction( action );
    QVERIFY( !model->data( model->index( 0, 0 ), Qt::CheckStateRole ).toBool() );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[0].state = true;" ) );
    m_document->processAction( action );
    QVERIFY( model->data( model->index( 0, 0 ), Qt::CheckStateRole ).toBool() );
    delete action;
    
    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[1].state = false;" ) );
    m_document->processAction( action );
    QVERIFY( !model->data( model->index( 1, 0 ), Qt::CheckStateRole ).toBool() );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[1].state = true;" ) );
    m_document->processAction( action );
    QVERIFY( model->data( model->index( 1, 0 ), Qt::CheckStateRole ).toBool() );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[2].state = false;" ) );
    m_document->processAction( action );
    QVERIFY( !model->data( model->index( 2, 0 ), Qt::CheckStateRole ).toBool() );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[2].state = true;" ) );
    m_document->processAction( action );
    QVERIFY( model->data( model->index( 2, 0 ), Qt::CheckStateRole ).toBool() );
    delete action;
    
    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[3].state = false;" ) );
    m_document->processAction( action );
    QVERIFY( !model->data( model->index( 3, 0 ), Qt::CheckStateRole ).toBool() );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, 
        QStringLiteral( "ocg[3].state = true;" ) );
    m_document->processAction( action );
    QVERIFY( model->data( model->index( 3, 0 ), Qt::CheckStateRole ).toBool() );
    delete action;
}

void KJSFunctionsTest::testAlert()
{
    Okular::ScriptAction *action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( \"Random Message\" );" ) );
    QScopedPointer< MessageBoxHelper > messageBoxHelper;
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Ok, QStringLiteral( "Random Message" ), QMessageBox::Critical, QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( \"Empty Message\", 1 );" ) );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Ok, QStringLiteral( "Empty Message" ), QMessageBox::Warning, QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( \"No Message\", 2, 2 );" ) );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Yes, QStringLiteral( "No Message" ), QMessageBox::Question, QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( \"No\", 3, 2, \"Test Dialog\" );" ) );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::No, QStringLiteral( "No" ), QMessageBox::Information, QStringLiteral( "Test Dialog" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "var oCheckBox = new Object();\
                                                                            ret = app.alert( \"Cancel\", 3, 3, \"Test Dialog\", 0, oCheckBox );" ) );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Cancel, QStringLiteral( "Cancel" ), QMessageBox::Information, QStringLiteral( "Test Dialog" ), true ) );
    m_document->processAction( action );
    delete action;
}

void KJSFunctionsTest::testPrintD()
{
    Okular::ScriptAction *action = new Okular::ScriptAction( Okular::JavaScript, 
                                                             QStringLiteral( "var date = new Date( 2010, 0, 5, 11, 10, 32, 1 );\
                                                                              ret = app.alert( util.printd( \"mm\\\\yyyy\", date ) );" ) );
    QScopedPointer< MessageBoxHelper > messageBoxHelper;
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Ok, QStringLiteral( "01\\2010" ), QMessageBox::Critical, QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( util.printd( \"m\\\\yy\", date ) );" ) );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Ok, QStringLiteral( "1\\10" ), QMessageBox::Critical, QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( util.printd( \"dd\\\\mm HH:MM\", date ) );" ) );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Ok, QStringLiteral( "05\\01 11:10" ), QMessageBox::Critical, QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( util.printd( \"dd\\\\mm HH:MM:ss\", date ) );" ) );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Ok, QStringLiteral( "05\\01 11:10:32" ), QMessageBox::Critical, QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( util.printd( \"yyyy\\\\mm HH:MM:ss\", date ) );" ) );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Ok, QStringLiteral( "2010\\01 11:10:32" ), QMessageBox::Critical, QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( util.printd( 0, date ) );" ) );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Ok, QStringLiteral( "D:20100105111032" ), QMessageBox::Critical, QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( util.printd( 1, date ) );" ) );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Ok, QStringLiteral( "2010.01.05 11:10:32" ), QMessageBox::Critical, QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
    delete action;

    action = new Okular::ScriptAction( Okular::JavaScript, QStringLiteral( "ret = app.alert( util.printd( 2, date ) );" ) );
    QLocale locale = QLocale::system();
    QDate date( 2010, 1, 5 );
    messageBoxHelper.reset( new MessageBoxHelper( QMessageBox::Ok, date.toString( locale.dateFormat( QLocale::ShortFormat ) ) + QStringLiteral( " 11:10:32" ), QMessageBox::Critical,
                                                  QStringLiteral( "Okular" ), false ) );
    m_document->processAction( action );
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
