/*
    SPDX-FileCopyrightText: 2013 Jon Mease <jon.mease@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include "../settings_core.h"
#include "core/annotations.h"
#include "core/document.h"
#include <QMimeDatabase>
#include <QMimeType>

class MockEditor;

class EditAnnotationContentsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void testConsecutiveCharBackspacesMerged();
    void testConsecutiveNewlineBackspacesNotMerged();
    void testConsecutiveCharInsertionsMerged();
    void testConsecutiveNewlineInsertionsNotMerged();
    void testConsecutiveCharDeletesMerged();
    void testConsecutiveNewlineDeletesNotMerged();
    void testConsecutiveEditsNotMergedAcrossDifferentAnnotations();
    void testInsertWithSelection();
    void testCombinations();

private:
    Okular::Document *m_document;
    Okular::TextAnnotation *m_annot1;
    Okular::TextAnnotation *m_annot2;
    MockEditor *m_editor1;
    MockEditor *m_editor2;
};

/*
 * Simple class that receives the Document::annotationContentsChangedByUndoRedo
 * signal that would normally be directed to an annotation's
 * contents editor (For example AnnotWindow)
 */
class MockEditor : public QObject
{
    Q_OBJECT

public:
    MockEditor(Okular::Annotation *annot, Okular::Document *doc);
    QString contents()
    {
        return m_contents;
    }
    int cursorPos()
    {
        return m_cursorPos;
    }
    int anchorPos()
    {
        return m_anchorPos;
    }

private Q_SLOTS:
    void slotAnnotationContentsChangedByUndoRedo(Okular::Annotation *annotation, const QString &contents, int cursorPos, int anchorPos);

private:
    Okular::Document *m_document;
    Okular::Annotation *m_annot;

    QString m_contents;
    int m_cursorPos;
    int m_anchorPos;
};

MockEditor::MockEditor(Okular::Annotation *annot, Okular::Document *doc)
{
    m_annot = annot;
    m_document = doc;
    connect(m_document, &Okular::Document::annotationContentsChangedByUndoRedo, this, &MockEditor::slotAnnotationContentsChangedByUndoRedo);
    m_cursorPos = 0;
    m_anchorPos = 0;
    m_contents = annot->contents();
}

void MockEditor::slotAnnotationContentsChangedByUndoRedo(Okular::Annotation *annotation, const QString &contents, int cursorPos, int anchorPos)
{
    if (annotation == m_annot) {
        m_contents = contents;
        m_cursorPos = cursorPos;
        m_anchorPos = anchorPos;
    }
}

void EditAnnotationContentsTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("editannotationcontentstest"));
    m_document = new Okular::Document(nullptr);
}

void EditAnnotationContentsTest::cleanupTestCase()
{
    delete m_document;
}

void EditAnnotationContentsTest::init()
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
    m_document->addPageAnnotation(0, m_annot1);

    m_annot2 = new Okular::TextAnnotation();
    m_annot2->setBoundingRectangle(Okular::NormalizedRect(0.1, 0.1, 0.15, 0.15));
    m_annot2->setContents(QStringLiteral("Hello, World"));
    m_document->addPageAnnotation(0, m_annot2);

    // setup editors
    m_editor1 = new MockEditor(m_annot1, m_document);
    m_editor2 = new MockEditor(m_annot2, m_document);
}

void EditAnnotationContentsTest::cleanup()
{
    m_document->closeDocument();
    delete m_editor1;
    delete m_editor2;
    // m_annot1 and m_annot2 are deleted when document is closed
}

void EditAnnotationContentsTest::testConsecutiveCharBackspacesMerged()
{
    // Hello, World| -> Hello, Worl|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, Worl"), 11, 12, 12);
    QCOMPARE(QStringLiteral("Hello, Worl"), m_annot1->contents());

    // Hello, Worl| -> Hello, Wor|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, Wor"), 10, 11, 11);
    QCOMPARE(QStringLiteral("Hello, Wor"), m_annot1->contents());

    // undo and verify that consecutive backspace operations are merged together
    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor1->contents());
    QCOMPARE(12, m_editor1->cursorPos());
    QCOMPARE(12, m_editor1->anchorPos());

    m_document->redo();
    QCOMPARE(QStringLiteral("Hello, Wor"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, Wor"), m_editor1->contents());
    QCOMPARE(10, m_editor1->cursorPos());
    QCOMPARE(10, m_editor1->anchorPos());

    // Hello, Wor| -> Hello, Wo|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, Wo"), 9, 10, 10);
    QCOMPARE(QStringLiteral("Hello, Wo"), m_annot1->contents());

    // Hello, Wo| -> Hello, W|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, W"), 8, 9, 9);
    QCOMPARE(QStringLiteral("Hello, W"), m_annot1->contents());

    // Hello, W| -> Hello, |
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, "), 7, 8, 8);
    QCOMPARE(QStringLiteral("Hello, "), m_annot1->contents());

    // undo and verify that consecutive backspace operations are merged together
    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor1->contents());
    QCOMPARE(12, m_editor1->cursorPos());
    QCOMPARE(12, m_editor1->anchorPos());

    m_document->redo();
    QCOMPARE(QStringLiteral("Hello, "), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, "), m_editor1->contents());
    QCOMPARE(7, m_editor1->cursorPos());
    QCOMPARE(7, m_editor1->anchorPos());
}

void EditAnnotationContentsTest::testConsecutiveNewlineBackspacesNotMerged()
{
    // Set contents to Hello, \n\n|World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, \n\nWorld"), 0, 0, 0);
    QCOMPARE(QStringLiteral("Hello, \n\nWorld"), m_annot1->contents());

    // Hello, \n\n|World -> Hello, \n|World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, \nWorld"), 8, 9, 9);
    QCOMPARE(QStringLiteral("Hello, \nWorld"), m_annot1->contents());

    // Hello, \n|World -> Hello, |World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, World"), 7, 8, 8);
    QCOMPARE(QStringLiteral("Hello, World"), m_annot1->contents());

    // Hello, |World -> Hello,|World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello,World"), 6, 7, 7);
    QCOMPARE(QStringLiteral("Hello,World"), m_annot1->contents());

    // Hello,|World -> Hello|World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("HelloWorld"), 5, 6, 6);
    QCOMPARE(QStringLiteral("HelloWorld"), m_annot1->contents());

    // Backspace operations of non-newline characters should be merged
    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor1->contents());
    QCOMPARE(7, m_editor1->cursorPos());
    QCOMPARE(7, m_editor1->anchorPos());

    // Backspace operations on newline characters should not be merged
    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, \nWorld"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, \nWorld"), m_editor1->contents());
    QCOMPARE(8, m_editor1->cursorPos());
    QCOMPARE(8, m_editor1->anchorPos());

    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, \n\nWorld"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, \n\nWorld"), m_editor1->contents());
    QCOMPARE(9, m_editor1->cursorPos());
    QCOMPARE(9, m_editor1->anchorPos());
}

void EditAnnotationContentsTest::testConsecutiveCharInsertionsMerged()
{
    // Hello, |World -> Hello, B|World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, BWorld"), 8, 7, 7);
    QCOMPARE(QStringLiteral("Hello, BWorld"), m_annot1->contents());

    // Hello, l| -> Hello, li|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, BiWorld"), 9, 8, 8);
    QCOMPARE(QStringLiteral("Hello, BiWorld"), m_annot1->contents());

    // Hello, li| -> Hello, lin|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, BigWorld"), 10, 9, 9);
    QCOMPARE(QStringLiteral("Hello, BigWorld"), m_annot1->contents());

    // Hello, lin| -> Hello, line|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, Big World"), 11, 10, 10);
    QCOMPARE(QStringLiteral("Hello, Big World"), m_annot1->contents());

    // Verify undo/redo operations merged
    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor1->contents());
    QCOMPARE(7, m_editor1->cursorPos());
    QCOMPARE(7, m_editor1->anchorPos());

    m_document->redo();
    QCOMPARE(QStringLiteral("Hello, Big World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, Big World"), m_editor1->contents());
    QCOMPARE(11, m_editor1->cursorPos());
    QCOMPARE(11, m_editor1->anchorPos());
}

void EditAnnotationContentsTest::testConsecutiveNewlineInsertionsNotMerged()
{
    // Hello, |World -> Hello, \n|World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, \nWorld"), 8, 7, 7);
    QCOMPARE(QStringLiteral("Hello, \nWorld"), m_annot1->contents());

    // Hello, |World -> Hello, \n|World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, \n\nWorld"), 9, 8, 8);
    QCOMPARE(QStringLiteral("Hello, \n\nWorld"), m_annot1->contents());

    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, \nWorld"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, \nWorld"), m_editor1->contents());
    QCOMPARE(8, m_editor1->cursorPos());
    QCOMPARE(8, m_editor1->anchorPos());

    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor1->contents());
    QCOMPARE(7, m_editor1->cursorPos());
    QCOMPARE(7, m_editor1->anchorPos());

    m_document->redo();
    QCOMPARE(QStringLiteral("Hello, \nWorld"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, \nWorld"), m_editor1->contents());
    QCOMPARE(8, m_editor1->cursorPos());
    QCOMPARE(8, m_editor1->anchorPos());

    m_document->redo();
    QCOMPARE(QStringLiteral("Hello, \n\nWorld"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, \n\nWorld"), m_editor1->contents());
    QCOMPARE(9, m_editor1->cursorPos());
    QCOMPARE(9, m_editor1->anchorPos());
}

void EditAnnotationContentsTest::testConsecutiveCharDeletesMerged()
{
    // Hello, |World -> Hello, |orld
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, orld"), 7, 7, 7);
    QCOMPARE(QStringLiteral("Hello, orld"), m_annot1->contents());

    // Hello, |orld -> Hello, |rld
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, rld"), 7, 7, 7);
    QCOMPARE(QStringLiteral("Hello, rld"), m_annot1->contents());

    // Hello, |rld -> Hello, |ld
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, ld"), 7, 7, 7);
    QCOMPARE(QStringLiteral("Hello, ld"), m_annot1->contents());

    // Hello, |ld -> Hello, |d
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, d"), 7, 7, 7);
    QCOMPARE(QStringLiteral("Hello, d"), m_annot1->contents());

    // Hello, | -> Hello, |
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, "), 7, 7, 7);
    QCOMPARE(QStringLiteral("Hello, "), m_annot1->contents());

    // Verify undo/redo operations merged
    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor1->contents());
    QCOMPARE(7, m_editor1->cursorPos());
    QCOMPARE(7, m_editor1->anchorPos());

    m_document->redo();
    QCOMPARE(QStringLiteral("Hello, "), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, "), m_editor1->contents());
    QCOMPARE(7, m_editor1->cursorPos());
    QCOMPARE(7, m_editor1->anchorPos());
}

void EditAnnotationContentsTest::testConsecutiveNewlineDeletesNotMerged()
{
    // Set contents to Hello, \n\n|World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, \n\nWorld"), 0, 0, 0);
    QCOMPARE(QStringLiteral("Hello, \n\nWorld"), m_annot1->contents());

    // He|llo, \n\nWorld ->  He|lo, \n\nWorld
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Helo, \n\nWorld"), 2, 2, 2);
    QCOMPARE(QStringLiteral("Helo, \n\nWorld"), m_annot1->contents());

    // He|lo, \n\nWorld ->  He|o, \n\nWorld
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Heo, \n\nWorld"), 2, 2, 2);
    QCOMPARE(QStringLiteral("Heo, \n\nWorld"), m_annot1->contents());

    // He|o, \n\nWorld ->  He|, \n\nWorld
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("He, \n\nWorld"), 2, 2, 2);
    QCOMPARE(QStringLiteral("He, \n\nWorld"), m_annot1->contents());

    // He|, \n\nWorld ->  He| \n\nWorld
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("He \n\nWorld"), 2, 2, 2);
    QCOMPARE(QStringLiteral("He \n\nWorld"), m_annot1->contents());

    // He| \n\nWorld ->  He|\n\nWorld
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("He\n\nWorld"), 2, 2, 2);
    QCOMPARE(QStringLiteral("He\n\nWorld"), m_annot1->contents());

    // He|\n\nWorld ->  He|\nWorld
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("He\nWorld"), 2, 2, 2);
    QCOMPARE(QStringLiteral("He\nWorld"), m_annot1->contents());

    // He|\nWorld ->  He|World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("HeWorld"), 2, 2, 2);
    QCOMPARE(QStringLiteral("HeWorld"), m_annot1->contents());

    // Verify that deletions of newlines are not merged, but deletions of other characters are
    m_document->undo();
    QCOMPARE(QStringLiteral("He\nWorld"), m_annot1->contents());
    QCOMPARE(QStringLiteral("He\nWorld"), m_editor1->contents());
    QCOMPARE(2, m_editor1->cursorPos());
    QCOMPARE(2, m_editor1->anchorPos());

    m_document->undo();
    QCOMPARE(QStringLiteral("He\n\nWorld"), m_annot1->contents());
    QCOMPARE(QStringLiteral("He\n\nWorld"), m_editor1->contents());
    QCOMPARE(2, m_editor1->cursorPos());
    QCOMPARE(2, m_editor1->anchorPos());

    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, \n\nWorld"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, \n\nWorld"), m_editor1->contents());
    QCOMPARE(2, m_editor1->cursorPos());
    QCOMPARE(2, m_editor1->anchorPos());
}

void EditAnnotationContentsTest::testConsecutiveEditsNotMergedAcrossDifferentAnnotations()
{
    // Annot1: Hello, World| -> Hello, Worl|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, Worl"), 11, 12, 12);
    QCOMPARE(QStringLiteral("Hello, Worl"), m_annot1->contents());
    // Annot1: Hello, Worl| -> Hello, Wor|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, Wor"), 10, 11, 11);
    QCOMPARE(QStringLiteral("Hello, Wor"), m_annot1->contents());

    // Annot2: Hello, World| -> Hello, Worl|
    m_document->editPageAnnotationContents(0, m_annot2, QStringLiteral("Hello, Worl"), 11, 12, 12);
    QCOMPARE(QStringLiteral("Hello, Worl"), m_annot2->contents());
    // Annot2: Hello, Worl| -> Hello, Wor|
    m_document->editPageAnnotationContents(0, m_annot2, QStringLiteral("Hello, Wor"), 10, 11, 11);
    QCOMPARE(QStringLiteral("Hello, Wor"), m_annot2->contents());

    // Annot1: Hello, Wor| -> Hello, Wo|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, Wo"), 9, 10, 10);
    QCOMPARE(QStringLiteral("Hello, Wo"), m_annot1->contents());
    // Annot1: Hello, Wo| -> Hello, W|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, W"), 8, 9, 9);
    QCOMPARE(QStringLiteral("Hello, W"), m_annot1->contents());
    // Annot1: Hello, W| -> Hello, |
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, "), 7, 8, 8);
    QCOMPARE(QStringLiteral("Hello, "), m_annot1->contents());

    // Annot2: Hello, Wor| -> Hello, Wo|
    m_document->editPageAnnotationContents(0, m_annot2, QStringLiteral("Hello, Wo"), 9, 10, 10);
    QCOMPARE(QStringLiteral("Hello, Wo"), m_annot2->contents());
    // Annot2: Hello, Wo| -> Hello, W|
    m_document->editPageAnnotationContents(0, m_annot2, QStringLiteral("Hello, W"), 8, 9, 9);
    QCOMPARE(QStringLiteral("Hello, W"), m_annot2->contents());
    // Annot2: Hello, W| -> Hello, |
    m_document->editPageAnnotationContents(0, m_annot2, QStringLiteral("Hello, "), 7, 8, 8);
    QCOMPARE(QStringLiteral("Hello, "), m_annot2->contents());

    // undo and verify that consecutive backspace operations are merged together
    // m_annot2 -> "Hello, Wor|"
    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, Wor"), m_annot2->contents());
    QCOMPARE(QStringLiteral("Hello, "), m_editor1->contents());
    QCOMPARE(QStringLiteral("Hello, Wor"), m_editor2->contents());
    QCOMPARE(10, m_editor2->cursorPos());
    QCOMPARE(10, m_editor2->anchorPos());

    // m_annot1 -> "Hello, Wor|"
    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, Wor"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, Wor"), m_editor1->contents());
    QCOMPARE(QStringLiteral("Hello, Wor"), m_editor2->contents());
    QCOMPARE(10, m_editor1->cursorPos());
    QCOMPARE(10, m_editor1->anchorPos());

    // m_annot2 -> "Hello, World|"
    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, World"), m_annot2->contents());
    QCOMPARE(QStringLiteral("Hello, Wor"), m_editor1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor2->contents());
    QCOMPARE(12, m_editor2->cursorPos());
    QCOMPARE(12, m_editor2->anchorPos());

    // m_annot1 -> "Hello, World|"
    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor2->contents());
    QCOMPARE(12, m_editor1->cursorPos());
    QCOMPARE(12, m_editor1->anchorPos());
}

void EditAnnotationContentsTest::testInsertWithSelection()
{
    // Annot1: |Hello|, World -> H|, World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("H, World"), 1, 0, 5);
    QCOMPARE(QStringLiteral("H, World"), m_annot1->contents());

    // Annot1: H|, World -> Hi|, World
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hi, World"), 2, 1, 1);
    QCOMPARE(QStringLiteral("Hi, World"), m_annot1->contents());

    m_document->undo();
    QCOMPARE(QStringLiteral("H, World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("H, World"), m_editor1->contents());
    QCOMPARE(1, m_editor1->cursorPos());
    QCOMPARE(1, m_editor1->anchorPos());

    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor1->contents());
    QCOMPARE(0, m_editor1->cursorPos());
    QCOMPARE(5, m_editor1->anchorPos());
}

void EditAnnotationContentsTest::testCombinations()
{
    // Annot1: Hello, World| -> Hello, Worl|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, Worl"), 11, 12, 12);
    QCOMPARE(QStringLiteral("Hello, Worl"), m_annot1->contents());

    // Annot1: Hello, Worl| -> Hello, Wor|
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("Hello, Wor"), 10, 11, 11);
    QCOMPARE(QStringLiteral("Hello, Wor"), m_annot1->contents());

    // Annot1: |He|llo, Wor -> |llo, Wor
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("llo, Wor"), 0, 2, 0);
    QCOMPARE(QStringLiteral("llo, Wor"), m_annot1->contents());

    // Annot1: |llo, Wor -> |lo, Wor
    m_document->editPageAnnotationContents(0, m_annot1, QStringLiteral("lo, Wor"), 0, 0, 0);
    QCOMPARE(QStringLiteral("lo, Wor"), m_annot1->contents());

    m_document->undo();
    QCOMPARE(QStringLiteral("llo, Wor"), m_annot1->contents());
    QCOMPARE(QStringLiteral("llo, Wor"), m_editor1->contents());
    QCOMPARE(0, m_editor1->cursorPos());
    QCOMPARE(0, m_editor1->anchorPos());

    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, Wor"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, Wor"), m_editor1->contents());
    QCOMPARE(2, m_editor1->cursorPos());
    QCOMPARE(0, m_editor1->anchorPos());

    m_document->undo();
    QCOMPARE(QStringLiteral("Hello, World"), m_annot1->contents());
    QCOMPARE(QStringLiteral("Hello, World"), m_editor1->contents());
    QCOMPARE(12, m_editor1->cursorPos());
    QCOMPARE(12, m_editor1->anchorPos());
}

QTEST_MAIN(EditAnnotationContentsTest)
#include "editannotationcontentstest.moc"
