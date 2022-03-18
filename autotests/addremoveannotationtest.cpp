/*
    SPDX-FileCopyrightText: 2013 Jon Mease <jon.mease@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QMimeDatabase>
#include <QTest>

#include "../core/annotations.h"
#include "../core/document.h"
#include "../core/page.h"
#include "../settings_core.h"
#include "testingutils.h"

class AddRemoveAnnotationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void testAddAnnotations();
    void testAddAnnotationUndoWithRotate_Bug318091();
    void testRemoveAnnotations();

private:
    Okular::Document *m_document;
};

void AddRemoveAnnotationTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("addannotationtest"));
    m_document = new Okular::Document(nullptr);
}

void AddRemoveAnnotationTest::init()
{
    const QString testFile = QStringLiteral(KDESRCDIR "data/file1.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);
    QCOMPARE(m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess);
}

void AddRemoveAnnotationTest::cleanup()
{
    m_document->closeDocument();
}

void AddRemoveAnnotationTest::testAddAnnotations()
{
    // Undo and Redo should be unavailable when docuemnt is first opened.
    QVERIFY(!m_document->canUndo());
    QVERIFY(!m_document->canRedo());

    // Create two distinct text annotations
    Okular::Annotation *annot1 = new Okular::TextAnnotation();
    annot1->setBoundingRectangle(Okular::NormalizedRect(0.1, 0.1, 0.15, 0.15));
    annot1->setContents(QStringLiteral("annot contents"));

    Okular::Annotation *annot2 = new Okular::TextAnnotation();
    annot2->setBoundingRectangle(Okular::NormalizedRect(0.2, 0.2, 0.3, 0.4));
    annot2->setContents(QStringLiteral("annot contents"));

    // The two annotations shold have different properties XML strings
    QVERIFY(TestingUtils::getAnnotationXml(annot1) != TestingUtils::getAnnotationXml(annot2));

    // We start with no annotations in the docuemnt
    QVERIFY(m_document->page(0)->annotations().size() == 0);

    // After adding annot1 we should have one annotation in the page and it should be annot1.
    m_document->addPageAnnotation(0, annot1);
    QVERIFY(m_document->page(0)->annotations().size() == 1);
    QCOMPARE(annot1, m_document->page(0)->annotations().first());

    // Record the properties and name of annot1 just after insertion for later comparisons
    QString origLine1Xml = TestingUtils::getAnnotationXml(annot1);
    QString annot1Name = annot1->uniqueName();
    QVERIFY(!annot1Name.isEmpty());

    // Now undo the addition of annot1 and verify that annot1's properties haven't changed
    m_document->undo();
    QVERIFY(m_document->page(0)->annotations().empty());
    QVERIFY(!m_document->canUndo());
    QVERIFY(m_document->canRedo());
    QCOMPARE(TestingUtils::getAnnotationXml(annot1), origLine1Xml);

    // redo addition of annot1
    m_document->redo();
    QVERIFY(m_document->page(0)->annotations().size() == 1);
    QVERIFY(annot1 == m_document->page(0)->annotations().first());
    QCOMPARE(TestingUtils::getAnnotationXml(annot1), origLine1Xml);

    // undo once more
    m_document->undo();
    QVERIFY(m_document->page(0)->annotations().empty());
    QVERIFY(!m_document->canUndo());
    QVERIFY(m_document->canRedo());
    QCOMPARE(TestingUtils::getAnnotationXml(annot1), origLine1Xml);

    // Set AnnotationDisposeWatcher dispose function on annot1 so we can detect
    // when it is deleted
    annot1->setDisposeDataFunction(TestingUtils::AnnotationDisposeWatcher::disposeAnnotation);
    TestingUtils::AnnotationDisposeWatcher::resetDisposedAnnotationName();
    QCOMPARE(TestingUtils::AnnotationDisposeWatcher::disposedAnnotationName(), QString());

    // now add annot2
    m_document->addPageAnnotation(0, annot2);
    QString annot2Name = annot2->uniqueName();
    QVERIFY(!annot2Name.isEmpty());
    QVERIFY(annot1Name != annot2Name);
    QVERIFY(m_document->page(0)->annotations().size() == 1);
    QCOMPARE(annot2, m_document->page(0)->annotations().first());

    // Check that adding annot2 while annot1 was in the unadded state triggered the deletion of annot1
    QVERIFY(TestingUtils::AnnotationDisposeWatcher::disposedAnnotationName() == annot1Name);
}

void AddRemoveAnnotationTest::testAddAnnotationUndoWithRotate_Bug318091()
{
    Okular::Annotation *annot = new Okular::TextAnnotation();
    annot->setBoundingRectangle(Okular::NormalizedRect(0.1, 0.1, 0.15, 0.15));
    annot->setContents(QStringLiteral("annot contents"));

    m_document->addPageAnnotation(0, annot);
    QString origAnnotXml = TestingUtils::getAnnotationXml(annot);

    // Now undo annotation addition, rotate the page, and redo to annotation addition
    m_document->undo();
    m_document->setRotation(1);
    m_document->redo();

    // Verify that annotation's properties remain unchanged
    // In Bug318091 the bounding rectangle was being rotated upon each redo
    QString newAnnotXml = TestingUtils::getAnnotationXml(annot);
    QCOMPARE(origAnnotXml, newAnnotXml);
}

void AddRemoveAnnotationTest::testRemoveAnnotations()
{
    // Undo and Redo should be unavailable when docuemnt is first opened.
    QVERIFY(!m_document->canUndo());
    QVERIFY(!m_document->canRedo());

    // Create two distinct text annotations
    Okular::Annotation *annot1 = new Okular::TextAnnotation();
    annot1->setBoundingRectangle(Okular::NormalizedRect(0.1, 0.1, 0.15, 0.15));
    annot1->setContents(QStringLiteral("annot contents"));

    Okular::Annotation *annot2 = new Okular::TextAnnotation();
    annot2->setBoundingRectangle(Okular::NormalizedRect(0.2, 0.2, 0.3, 0.4));
    annot2->setContents(QStringLiteral("annot contents"));

    // Add annot1 and annot2 to document
    m_document->addPageAnnotation(0, annot1);
    m_document->addPageAnnotation(0, annot2);
    QVERIFY(m_document->page(0)->annotations().size() == 2);
    QVERIFY(m_document->page(0)->annotations().contains(annot1));
    QVERIFY(m_document->page(0)->annotations().contains(annot2));

    // Now remove annot1
    m_document->removePageAnnotation(0, annot1);
    QVERIFY(m_document->page(0)->annotations().size() == 1);
    QVERIFY(m_document->page(0)->annotations().contains(annot2));

    // Undo removal of annot1
    m_document->undo();
    QVERIFY(m_document->page(0)->annotations().size() == 2);
    QVERIFY(m_document->page(0)->annotations().contains(annot1));
    QVERIFY(m_document->page(0)->annotations().contains(annot2));

    // Redo removal
    m_document->redo();
    QVERIFY(m_document->page(0)->annotations().size() == 1);
    QVERIFY(m_document->page(0)->annotations().contains(annot2));

    // Verify that annot1 is disposed of if document is closed with annot1 in removed state
    QString annot1Name = annot1->uniqueName();
    annot1->setDisposeDataFunction(TestingUtils::AnnotationDisposeWatcher::disposeAnnotation);
    TestingUtils::AnnotationDisposeWatcher::resetDisposedAnnotationName();
    QVERIFY(TestingUtils::AnnotationDisposeWatcher::disposedAnnotationName().isEmpty());
    m_document->closeDocument();
    QVERIFY(TestingUtils::AnnotationDisposeWatcher::disposedAnnotationName() == annot1Name);
}

QTEST_MAIN(AddRemoveAnnotationTest)
#include "addremoveannotationtest.moc"
