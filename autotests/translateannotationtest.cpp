/*
    SPDX-FileCopyrightText: 2013 Jon Mease <jon.mease@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include "../settings_core.h"
#include "core/annotations.h"
#include "core/document.h"
#include "testingutils.h"
#include <QMimeDatabase>
#include <QMimeType>

Okular::LineAnnotation *getNewLineAnnotation(double startX, double startY, double endX, double endY)
{
    Okular::LineAnnotation *line = new Okular::LineAnnotation;
    line->setLinePoints({Okular::NormalizedPoint(startX, startY), Okular::NormalizedPoint(endX, endY)});

    double left = qMin(startX, endX);
    double top = qMin(startY, endY);
    double right = qMax(startX, endX);
    double bottom = qMax(startY, endY);

    line->setBoundingRectangle(Okular::NormalizedRect(left, top, right, bottom));
    return line;
}

class TranslateAnnotationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void testTranslateAnnotation();
    void testSequentialTranslationsMergedIfBeingMovedIsSet();
    void testSequentialTranslationsNotMergedIfBeingMovedIsNotSet();
    void testAlternateTranslationsNotMerged();

private:
    Okular::Document *m_document;
    Okular::LineAnnotation *m_annot1;
    Okular::LineAnnotation *m_annot2;

    Okular::NormalizedPoint m_deltaA;
    Okular::NormalizedPoint m_deltaB;

    QList<Okular::NormalizedPoint> m_origPoints1;
    QList<Okular::NormalizedPoint> m_origPoints2;

    QList<Okular::NormalizedPoint> m_points1DeltaA;
    QList<Okular::NormalizedPoint> m_points1DeltaAB;
    QList<Okular::NormalizedPoint> m_points2DeltaA;
    QList<Okular::NormalizedPoint> m_points2DeltaAB;
};

void TranslateAnnotationTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("editannotationcontentstest"));
    m_document = new Okular::Document(nullptr);

    // translate m_annot1
    m_deltaA = Okular::NormalizedPoint(0.05, 0.1);
    m_deltaB = Okular::NormalizedPoint(0.1, 0.2);

    // Build lists of expected points for various states
    m_origPoints1 = {Okular::NormalizedPoint(0.1, 0.1), Okular::NormalizedPoint(0.2, 0.3)};

    m_points1DeltaA = {Okular::NormalizedPoint(0.15, 0.2), Okular::NormalizedPoint(0.25, 0.4)};

    m_points1DeltaAB = {Okular::NormalizedPoint(0.25, 0.4), Okular::NormalizedPoint(0.35, 0.6)};

    m_origPoints2 = {Okular::NormalizedPoint(0.1, 0.1), Okular::NormalizedPoint(0.3, 0.4)};

    m_points2DeltaA = {Okular::NormalizedPoint(0.15, 0.2), Okular::NormalizedPoint(0.35, 0.5)};

    m_points2DeltaAB = {Okular::NormalizedPoint(0.25, 0.4), Okular::NormalizedPoint(0.45, 0.7)};
}

void TranslateAnnotationTest::cleanupTestCase()
{
    delete m_document;
}

void TranslateAnnotationTest::init()
{
    const QString testFile = QStringLiteral(KDESRCDIR "data/file1.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);
    QCOMPARE(m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess);

    // Undo and Redo should be unavailable when docuemnt is first opened.
    QVERIFY(!m_document->canUndo());
    QVERIFY(!m_document->canRedo());

    // Create two distinct line annotations and add them to the document
    m_annot1 = getNewLineAnnotation(m_origPoints1.first().x, m_origPoints1.first().y, m_origPoints1.last().x, m_origPoints1.last().y);
    m_document->addPageAnnotation(0, m_annot1);

    m_annot2 = getNewLineAnnotation(m_origPoints2.first().x, m_origPoints2.first().y, m_origPoints2.last().x, m_origPoints2.last().y);
    m_document->addPageAnnotation(0, m_annot2);
}

void TranslateAnnotationTest::cleanup()
{
    m_document->closeDocument();
    // m_annot1 and m_annot2 are deleted when document is closed
}

void TranslateAnnotationTest::testTranslateAnnotation()
{
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_origPoints1));
    m_document->translatePageAnnotation(0, m_annot1, m_deltaA);
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaA));

    // undo and ensure m_annot1 is back to where it started
    m_document->undo();
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_origPoints1));

    // redo then translate m_annot1 by m_deltaB
    m_document->redo();
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaA));
}

void TranslateAnnotationTest::testSequentialTranslationsMergedIfBeingMovedIsSet()
{
    // mark m_annot1 as BeingMoved but not m_annot2
    m_annot1->setFlags(m_annot1->flags() | Okular::Annotation::BeingMoved);

    // Verify initial positions
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_origPoints1));
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot2->linePoints(), m_origPoints2));

    // Translate m_annot1 by m_deltaA then m_deltaB
    m_document->translatePageAnnotation(0, m_annot1, m_deltaA);
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaA));
    m_document->translatePageAnnotation(0, m_annot1, m_deltaB);
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaAB));

    // Now undo and verify that these two translations were merged into one undo command
    m_document->undo();
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_origPoints1));
}

void TranslateAnnotationTest::testSequentialTranslationsNotMergedIfBeingMovedIsNotSet()
{
    // mark m_annot1 as not BeingMoved
    m_annot1->setFlags(m_annot1->flags() & ~Okular::Annotation::BeingMoved);

    // Verify initial positions
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_origPoints1));
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot2->linePoints(), m_origPoints2));

    // Translate m_annot1 by m_deltaA then m_deltaB
    m_document->translatePageAnnotation(0, m_annot1, m_deltaA);
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaA));
    m_document->translatePageAnnotation(0, m_annot1, m_deltaB);
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaAB));

    // Now undo and verify that these two translations were NOT merged into one undo command
    m_document->undo();
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaA));

    m_document->undo();
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_origPoints1));
}

void TranslateAnnotationTest::testAlternateTranslationsNotMerged()
{
    // Set both m_annot1 and m_annot2 to BeingMoved
    m_annot1->setFlags(m_annot1->flags() | Okular::Annotation::BeingMoved);
    m_annot2->setFlags(m_annot2->flags() | Okular::Annotation::BeingMoved);

    m_document->translatePageAnnotation(0, m_annot1, m_deltaA);
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaA));
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot2->linePoints(), m_origPoints2));
    m_document->translatePageAnnotation(0, m_annot2, m_deltaA);
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaA));
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot2->linePoints(), m_points2DeltaA));
    m_document->translatePageAnnotation(0, m_annot1, m_deltaB);
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaAB));
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot2->linePoints(), m_points2DeltaA));
    m_document->translatePageAnnotation(0, m_annot2, m_deltaB);
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaAB));
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot2->linePoints(), m_points2DeltaAB));

    // First undo should move only m_annot2 back by m_deltaB
    m_document->undo();
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaAB));
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot2->linePoints(), m_points2DeltaA));

    // Next undo should move only m_annot1 back by m_deltaB
    m_document->undo();
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaA));
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot2->linePoints(), m_points2DeltaA));

    // Next Undo should move only m_annot2 back to its original location
    m_document->undo();
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_points1DeltaA));
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot2->linePoints(), m_origPoints2));

    // Next undo should move m_annot1 back to its original location
    m_document->undo();
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot1->linePoints(), m_origPoints1));
    QVERIFY(TestingUtils::pointListsAlmostEqual(m_annot2->linePoints(), m_origPoints2));
}

QTEST_MAIN(TranslateAnnotationTest)
#include "translateannotationtest.moc"
