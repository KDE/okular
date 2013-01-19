/***************************************************************************
 *   Copyright (C) 2013 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtest_kde.h>

#include "../core/document.h"
#include "../settings_core.h"

Q_DECLARE_METATYPE(Okular::Document::SearchStatus)

class SearchFinishedReceiver : public QObject
{
    Q_OBJECT

    private slots:
        void searchFinished(int id, Okular::Document::SearchStatus status)
        {
            m_id = id;
            m_status = status;
        }
    
    public:
        int m_id;
        Okular::Document::SearchStatus m_status;
};
    
class SearchTest : public QObject
{
    Q_OBJECT

    private slots:
        void initTestCase();
        void test311232();
};

void SearchTest::initTestCase()
{
    qRegisterMetaType<Okular::Document::SearchStatus>();
}

void SearchTest::test311232()
{
    Okular::SettingsCore::instance( "searchtest" );
    Okular::Document d(0);
    SearchFinishedReceiver receiver;
    QSignalSpy spy(&d, SIGNAL(searchFinished(int,Okular::Document::SearchStatus)));

    QObject::connect(&d, SIGNAL(searchFinished(int,Okular::Document::SearchStatus)), &receiver, SLOT(searchFinished(int,Okular::Document::SearchStatus)));
    
    const QString testFile = KDESRCDIR "data/file1.pdf";
    const KMimeType::Ptr mime = KMimeType::findByPath( testFile );
    d.openDocument(testFile, KUrl(), mime);
    
    const int searchId = 0;
    d.searchText(searchId, " i ", true, Qt::CaseSensitive, Okular::Document::NextMatch, false, QColor(), true);
    QTime t;
    t.start();
    while (spy.count() != 1 && t.elapsed() < 500)
        qApp->processEvents();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(receiver.m_id, searchId);
    QCOMPARE(receiver.m_status, Okular::Document::MatchFound);
    
    
    d.continueSearch( searchId, Okular::Document::PreviousMatch );
    t.start();
    while (spy.count() != 2 && t.elapsed() < 500)
        qApp->processEvents();
    QCOMPARE(spy.count(), 2);
    QCOMPARE(receiver.m_id, searchId);
    QCOMPARE(receiver.m_status, Okular::Document::NoMatchFound);
}

QTEST_KDEMAIN( SearchTest, GUI )

#include "searchtest.moc"
