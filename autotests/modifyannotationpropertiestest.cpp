/*
    SPDX-FileCopyrightText: 2013 Jon Mease <jon.mease@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include "../settings_core.h"
#include "core/document.h"
#include "testingutils.h"
#include <QMimeDatabase>
#include <QMimeType>
#include <core/annotations.h>
#include <core/area.h>

class ModifyAnnotationPropertiesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void testModifyAnnotationProperties();
    void testModifyDefaultAnnotationProperties();
    void testModifyAnnotationPropertiesWithRotation_Bug318828();

private:
    Okular::Document *m_document;
    Okular::TextAnnotation *m_annot1;
};

void ModifyAnnotationPropertiesTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("editannotationcontentstest"));
    m_document = new Okular::Document(nullptr);
}

void ModifyAnnotationPropertiesTest::cleanupTestCase()
{
    delete m_document;
}

void ModifyAnnotationPropertiesTest::init()
{
    const QString testFile = QStringLiteral(KDESRCDIR "data/file1.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);
    QCOMPARE(m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess);

    // Undo and Redo should be unavailable when docuemnt is first opened.
    QVERIFY(!m_document->canUndo());
    QVERIFY(!m_document->canRedo());

    // Create two distinct text annotations
    m_annot1 = new Okular::TextAnnotation();
    m_annot1->setBoundingRectangle(Okular::NormalizedRect(0.1, 0.1, 0.15, 0.15));
    m_annot1->setContents(QStringLiteral("Hello, World"));
    m_annot1->setAuthor(QStringLiteral("Jon Mease"));
    m_annot1->style().setColor(Qt::red);
    m_annot1->style().setWidth(4.0);
    m_document->addPageAnnotation(0, m_annot1);
}

void ModifyAnnotationPropertiesTest::cleanup()
{
    m_document->closeDocument();
    // m_annot1 and m_annot2 are deleted when document is closed
}

void ModifyAnnotationPropertiesTest::testModifyAnnotationProperties()
{
    // Add m_annot1 to document and record its properties XML string
    QString origLine1Xml = TestingUtils::getAnnotationXml(m_annot1);

    // Tell document we're going to modify m_annot1's properties
    m_document->prepareToModifyAnnotationProperties(m_annot1);

    // Now modify m_annot1's properties and record properties XML string
    m_annot1->style().setWidth(8.0);
    m_annot1->style().setColor(Qt::green);
    m_document->modifyPageAnnotationProperties(0, m_annot1);
    QString m_annot1XmlA = TestingUtils::getAnnotationXml(m_annot1);
    QCOMPARE(8.0, m_annot1->style().width());
    QCOMPARE(QColor(Qt::green), m_annot1->style().color());

    // undo modification and check that original properties have been restored
    m_document->undo();
    QCOMPARE(4.0, m_annot1->style().width());
    QCOMPARE(QColor(Qt::red), m_annot1->style().color());
    QCOMPARE(origLine1Xml, TestingUtils::getAnnotationXml(m_annot1));

    // redo modification and verify that new properties have been restored
    m_document->redo();
    QCOMPARE(8.0, m_annot1->style().width());
    QCOMPARE(QColor(Qt::green), m_annot1->style().color());
    QCOMPARE(m_annot1XmlA, TestingUtils::getAnnotationXml(m_annot1));

    // Verify that default values are properly restored.  (We haven't explicitly set opacity yet)
    QCOMPARE(1.0, m_annot1->style().opacity());
    m_document->prepareToModifyAnnotationProperties(m_annot1);
    m_annot1->style().setOpacity(0.5);
    m_document->modifyPageAnnotationProperties(0, m_annot1);
    QCOMPARE(0.5, m_annot1->style().opacity());

    m_document->undo();
    QCOMPARE(1.0, m_annot1->style().opacity());
    QCOMPARE(m_annot1XmlA, TestingUtils::getAnnotationXml(m_annot1));

    // And finally undo back to original properties
    m_document->undo();
    QCOMPARE(4.0, m_annot1->style().width());
    QCOMPARE(QColor(Qt::red), m_annot1->style().color());
    QCOMPARE(origLine1Xml, TestingUtils::getAnnotationXml(m_annot1));
}

void ModifyAnnotationPropertiesTest::testModifyDefaultAnnotationProperties()
{
    QString origLine1Xml = TestingUtils::getAnnotationXml(m_annot1);

    // Verify that default values are properly restored.  (We haven't explicitly set opacity yet)
    QCOMPARE(1.0, m_annot1->style().opacity());
    m_document->prepareToModifyAnnotationProperties(m_annot1);
    m_annot1->style().setOpacity(0.5);
    m_document->modifyPageAnnotationProperties(0, m_annot1);
    QCOMPARE(0.5, m_annot1->style().opacity());

    m_document->undo();
    QCOMPARE(1.0, m_annot1->style().opacity());
    QCOMPARE(origLine1Xml, TestingUtils::getAnnotationXml(m_annot1));
}

void ModifyAnnotationPropertiesTest::testModifyAnnotationPropertiesWithRotation_Bug318828()
{
    Okular::NormalizedRect boundingRect = Okular::NormalizedRect(0.1, 0.1, 0.15, 0.15);
    Okular::NormalizedRect transformedBoundingRect;
    m_annot1->setBoundingRectangle(boundingRect);
    m_document->addPageAnnotation(0, m_annot1);

    transformedBoundingRect = m_annot1->transformedBoundingRectangle();

    // Before page rotation boundingRect and transformedBoundingRect should be equal
    QCOMPARE(boundingRect, transformedBoundingRect);
    m_document->setRotation(1);

    // After rotation boundingRect should remain unchanged but
    // transformedBoundingRect should no longer equal boundingRect
    QCOMPARE(boundingRect, m_annot1->boundingRectangle());
    transformedBoundingRect = m_annot1->transformedBoundingRectangle();
    QVERIFY(!(boundingRect == transformedBoundingRect));

    // Modifying the properties of m_annot1 while page is rotated shouldn't
    // alter either boundingRect or transformedBoundingRect
    m_document->prepareToModifyAnnotationProperties(m_annot1);
    m_annot1->style().setOpacity(0.5);
    m_document->modifyPageAnnotationProperties(0, m_annot1);

    QCOMPARE(boundingRect, m_annot1->boundingRectangle());
    QCOMPARE(transformedBoundingRect, m_annot1->transformedBoundingRectangle());
}

QTEST_MAIN(ModifyAnnotationPropertiesTest)
#include "modifyannotationpropertiestest.moc"
