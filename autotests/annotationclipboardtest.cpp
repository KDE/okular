/*
    SPDX-FileCopyrightText: 2013 Peter Grasch <me@bedahr.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QClipboard>
#include <QDomDocument>
#include <QMimeData>
#include <QMimeDatabase>
#include <QTest>

#include "../core/annotations.h"
#include "../core/document.h"
#include "../core/page.h"
#include "../part/annotationpopup.h"
#include "../settings_core.h"

class AnnotationClipboardTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testCopy();
    void testCopyPaste();
    void testCopyPasteWrongVersion();
    void testClipboardMimetype();
    void cleanup();
    void cleanupTestCase();

private:
    Okular::Document *m_document;
};

void AnnotationClipboardTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("annotationclipboardtest"));
    m_document = new Okular::Document(nullptr);
    const QString testFile = QStringLiteral(KDESRCDIR "data/file1.pdf");
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(testFile);
    QCOMPARE(m_document->openDocument(testFile, QUrl(), mime), Okular::Document::OpenSuccess);
}

void AnnotationClipboardTest::cleanup()
{
    const QList<Okular::Annotation *> annotations = m_document->page(0)->annotations();
    for (Okular::Annotation *annotation : annotations) {
        m_document->removePageAnnotation(0, annotation);
    }
}

void AnnotationClipboardTest::cleanupTestCase()
{
    delete m_document;
}

void AnnotationClipboardTest::testCopy()
{
    Okular::TextAnnotation *ta = new Okular::TextAnnotation();
    ta->setFlags(ta->flags() | Okular::Annotation::FixedRotation);
    ta->setTextType(Okular::TextAnnotation::InPlace);
    ta->setInplaceIntent(Okular::TextAnnotation::TypeWriter);
    ta->style().setWidth(0.0);
    ta->style().setColor(QColor(255, 255, 255, 0));
    ta->setBoundingRectangle(Okular::NormalizedRect(0.1, 0.1, 0.5, 0.5));
    ta->setContents(QStringLiteral("annot contents"));

    AnnotationPopup popup(m_document, AnnotationPopup::SingleAnnotationMode);
    popup.addAnnotation(ta, 0);
    popup.doCopyAnnotation({ta, 0});

    const QMimeData *clipData = QApplication::clipboard()->mimeData();
    QVERIFY(clipData && clipData->hasFormat(QLatin1String(AnnotationPopup::annotationClipboardMimeType)));

    QDomDocument document;
    document.setContent(clipData->data(QLatin1String(AnnotationPopup::annotationClipboardMimeType)));
    const QDomElement root = document.documentElement();

    QCOMPARE(root.tagName(), QStringLiteral("annotations"));
    QVERIFY(root.hasAttribute(QStringLiteral("version")));
    QCOMPARE(root.attribute(QStringLiteral("version")).toInt(), AnnotationPopup::annotationClipboardFormatVersion);

    const QDomElement annEl = root.firstChildElement(QStringLiteral("annotation"));
    QVERIFY(!annEl.isNull());
    QCOMPARE(annEl.attribute(QStringLiteral("type")).toInt(), (int)Okular::Annotation::AText);

    delete ta;
}

void AnnotationClipboardTest::testCopyPaste()
{
    // Copy a known annotation to the clipboard
    Okular::TextAnnotation *original = new Okular::TextAnnotation();
    original->setFlags(original->flags() | Okular::Annotation::FixedRotation);
    original->setTextType(Okular::TextAnnotation::InPlace);
    original->setInplaceIntent(Okular::TextAnnotation::TypeWriter);
    original->setBoundingRectangle(Okular::NormalizedRect(0.1, 0.1, 0.5, 0.5));
    original->setContents(QStringLiteral("test contents"));

    AnnotationPopup popup(m_document, AnnotationPopup::SingleAnnotationMode);
    popup.addAnnotation(original, 0);
    popup.doCopyAnnotation({original, 0});
    QVERIFY(AnnotationPopup::clipboardHasAnnotations());

    // Paste onto page 0 and verify the annotation was deserialized correctly
    popup.pasteAnnotationToPage(0);

    const QList<Okular::Annotation *> annotations = m_document->page(0)->annotations();
    QCOMPARE(annotations.size(), 1);

    const auto *loaded = dynamic_cast<const Okular::TextAnnotation *>(annotations.first());
    QVERIFY(loaded != nullptr);
    QCOMPARE(loaded->contents(), QStringLiteral("test contents"));
    QCOMPARE(loaded->textType(), Okular::TextAnnotation::InPlace);
    QCOMPARE(loaded->inplaceIntent(), Okular::TextAnnotation::TypeWriter);

    delete original;
}

void AnnotationClipboardTest::testCopyPasteWrongVersion()
{
    // Build clipboard data with an unsupported version
    Okular::TextAnnotation ta;
    ta.setBoundingRectangle(Okular::NormalizedRect(0.1, 0.1, 0.5, 0.5));
    ta.setContents(QStringLiteral("should not be pasted"));

    QDomDocument document(QStringLiteral("okular-annotations"));
    QDomElement root = document.createElement(QStringLiteral("annotations"));
    root.setAttribute(QStringLiteral("version"), AnnotationPopup::annotationClipboardFormatVersion + 1);
    document.appendChild(root);
    QDomElement annotationElement = document.createElement(QStringLiteral("annotation"));
    Okular::AnnotationUtils::storeAnnotation(&ta, annotationElement, document);
    root.appendChild(annotationElement);

    auto *mimeData = new QMimeData();
    mimeData->setData(QLatin1String(AnnotationPopup::annotationClipboardMimeType), document.toByteArray());
    QApplication::clipboard()->setMimeData(mimeData);

    AnnotationPopup popup(m_document, AnnotationPopup::SingleAnnotationMode);
    popup.pasteAnnotationToPage(0);

    // Version mismatch → nothing should have been pasted
    QCOMPARE(m_document->page(0)->annotations().size(), 0);
}

void AnnotationClipboardTest::testClipboardMimetype()
{
    Okular::TextAnnotation *ta = new Okular::TextAnnotation();
    ta->setTextType(Okular::TextAnnotation::InPlace);
    ta->setBoundingRectangle(Okular::NormalizedRect(0.1, 0.1, 0.5, 0.5));
    ta->setContents(QStringLiteral("annot contents"));

    AnnotationPopup popup(m_document, AnnotationPopup::SingleAnnotationMode);
    popup.addAnnotation(ta, 0);
    popup.doCopyAnnotation({ta, 0});

    const QMimeData *clipData = QApplication::clipboard()->mimeData();
    QVERIFY(clipData && clipData->hasFormat(QLatin1String(AnnotationPopup::annotationClipboardMimeType)));

    delete ta;
}

QTEST_MAIN(AnnotationClipboardTest)
#include "annotationclipboardtest.moc"
