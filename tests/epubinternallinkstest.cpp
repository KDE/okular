/***************************************************************************
 *   Copyright (C) 2013 Jaydeep Solanki <jaydp17@gmail.com>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtest_kde.h>

#include "../part.h"
#include "../core/page.h"

namespace Okular
{
class EpubInternalLinksTest : public QObject
{
    Q_OBJECT
private slots:
    void testInternalLinks_data();
    void testInternalLinks();
};

void EpubInternalLinksTest::testInternalLinks()
{
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs, KGlobal::mainComponent());
    part.openDocument(KDESRCDIR "data/contents.epub");
    Okular::Document *doc = part.m_document;
    doc->setViewportPage(0);
    QVERIFY(doc);

    int width = doc->page( 0 )->width();  // 0 because current page is zero
    int height = doc->page( 0 )->height();
    QFETCH( double, yCords );
    QFETCH( int , pagesNos );
    const ObjectRect *rect = doc->page( 0 )->objectRect( ObjectRect::Action, 0.0453564, yCords, width, height);
    QVERIFY( rect );
    const Okular::Action * action = static_cast< const Okular::Action * >( rect->object() );
    doc->processAction( action );
    QCOMPARE( doc->currentPage(), ( uint ) pagesNos );
    part.slotGotoFirst();
}


void EpubInternalLinksTest::testInternalLinks_data()
{
    QTest::addColumn< double >( "yCords" );
    QTest::addColumn< int >( "pagesNos" );
    QTest::newRow( "0.025" ) << 0.025 << 0;
    QTest::newRow( "0.06" ) << 0.06 << 1;
    QTest::newRow( "0.095" ) << 0.095 << 0;
    QTest::newRow( "0.13" ) << 0.13 << 1;
    QTest::newRow( "0.165" ) << 0.165 << 1;
    QTest::newRow( "0.2" ) << 0.2 << 1;
    QTest::newRow( "0.235" ) << 0.235 << 2;
    QTest::newRow( "0.27" ) << 0.27 << 2;
    QTest::newRow( "0.305" ) << 0.305 << 2;
    QTest::newRow( "0.34" ) << 0.34 << 2;
}

}

QTEST_KDEMAIN( Okular::EpubInternalLinksTest, GUI )

#include "epubinternallinkstest.moc"
