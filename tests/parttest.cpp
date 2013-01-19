/***************************************************************************
 *   Copyright (C) 2013 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtest_kde.h>

#include "../part.h"
#include "../ui/toc.h"

#include <QTreeView>

namespace Okular
{
class PartTest
    : public QObject
{
    Q_OBJECT

    private slots:
        void testTOCReload();
};

void PartTest::testTOCReload()
{
    QVariantList dummyArgs;
    Okular::Part part(NULL, NULL, dummyArgs, KGlobal::mainComponent());
    part.openDocument(KDESRCDIR "data/tocreload.pdf");
    QCOMPARE(part.m_toc->expandedNodes().count(), 0);
    part.m_toc->m_treeView->expandAll();
    QCOMPARE(part.m_toc->expandedNodes().count(), 3);
    part.reload();
    qApp->processEvents();
    QCOMPARE(part.m_toc->expandedNodes().count(), 3);
}
}

QTEST_KDEMAIN( Okular::PartTest, GUI )

#include "parttest.moc"
