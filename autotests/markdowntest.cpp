/*
    SPDX-FileCopyrightText: 2020 Markus Brenneis <support.gulp21+kde@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include "../settings_core.h"
#include "core/document.h"
#include "generators/markdown/converter.h"
#include <QMimeDatabase>
#include <QMimeType>
#include <QTextDocument>
#include <memory>

class MarkdownTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testFancyPantsEnabled();
    void testFancyPantsDisabled();
    void testImageSizes();
    void testSpecialCharsInImageFileName();
    void testStrikeThrough();
    void testHtmlTagFixup();

private:
    void findImages(QTextFrame *parent, QVector<QTextImageFormat> &images);
    void findImages(const QTextBlock &parent, QVector<QTextImageFormat> &images);
};

void MarkdownTest::initTestCase()
{
    Okular::SettingsCore::instance(QStringLiteral("markdowntest"));
}

void MarkdownTest::testFancyPantsEnabled()
{
    Markdown::Converter converter;
    converter.setFancyPantsEnabled(true);
    std::unique_ptr<QTextDocument> document(converter.convert(QStringLiteral(KDESRCDIR "data/imageSizes.md")));

    QTextFrame::iterator secondFrame = ++(document->rootFrame()->begin());
    QVERIFY(secondFrame.currentBlock().text().startsWith(QStringLiteral("©")));
}

void MarkdownTest::testFancyPantsDisabled()
{
    Markdown::Converter converter;
    converter.setFancyPantsEnabled(false);
    std::unique_ptr<QTextDocument> document(converter.convert(QStringLiteral(KDESRCDIR "data/imageSizes.md")));

    QTextFrame::iterator secondFrame = ++(document->rootFrame()->begin());
    QVERIFY(secondFrame.currentBlock().text().startsWith(QStringLiteral("(c)")));
}

void MarkdownTest::testImageSizes()
{
    Markdown::Converter converter;
    std::unique_ptr<QTextDocument> document(converter.convert(QStringLiteral(KDESRCDIR "data/imageSizes.md")));

    QTextFrame *parent = document->rootFrame();

    QVector<QTextImageFormat> images;
    findImages(parent, images);

    QCOMPARE(images.size(), 17);

    qreal expectedSizes[][2] = {// width, height
                                // small image
                                {412, 349},
                                {100, 84.70873786407767},
                                {118.0515759312321, 100},
                                {100, 100},
                                {890, 753.9077669902913},
                                {890, 890},
                                // wide image
                                {890, 178},
                                {100, 20},
                                {500, 100},
                                {100, 100},
                                {890, 178},
                                {890, 890},
                                // tall image
                                {300, 1500},
                                {100, 500},
                                {20, 100},
                                {100, 100},
                                {890, 890}};

    for (int i = 0; i < images.size(); i++) {
        QCOMPARE(images[i].width(), expectedSizes[i][0]);
        QCOMPARE(images[i].height(), expectedSizes[i][1]);
    }
}

void MarkdownTest::findImages(QTextFrame *parent, QVector<QTextImageFormat> &images)
{
    for (QTextFrame::iterator it = parent->begin(); !it.atEnd(); ++it) {
        QTextFrame *textFrame = it.currentFrame();
        const QTextBlock textBlock = it.currentBlock();

        if (textFrame) {
            findImages(textFrame, images);
        } else if (textBlock.isValid()) {
            findImages(textBlock, images);
        }
    }
}

void MarkdownTest::findImages(const QTextBlock &parent, QVector<QTextImageFormat> &images)
{
    for (QTextBlock::iterator it = parent.begin(); !it.atEnd(); ++it) {
        const QTextFragment textFragment = it.fragment();
        if (textFragment.isValid()) {
            const QTextCharFormat textCharFormat = textFragment.charFormat();
            if (textCharFormat.isImageFormat()) {
                images.append(textCharFormat.toImageFormat());
            }
        }
    }
}

void MarkdownTest::testSpecialCharsInImageFileName()
{
    Markdown::Converter converter;
    std::unique_ptr<QTextDocument> document(converter.convert(QStringLiteral(KDESRCDIR "data/imageUrlsWithSpecialChars.md")));

    QTextFrame *parent = document->rootFrame();

    QVector<QTextImageFormat> images;
    findImages(parent, images);

    QCOMPARE(images.size(), 1);
    QVERIFY(images[0].name().endsWith(QStringLiteral("kartöffelchen.jpg")));
    QVERIFY(!images[0].name().contains(QStringLiteral("kart%C3%B6ffelchen.jpg")));
}

void MarkdownTest::testStrikeThrough()
{
    Markdown::Converter converter;
    converter.setFancyPantsEnabled(true);
    std::unique_ptr<QTextDocument> document(converter.convert(QStringLiteral(KDESRCDIR "data/strikethrough.md")));

    const QTextFrame *rootFrame = document->rootFrame();
    auto frameIter = rootFrame->begin();

    // Header line.
    QCOMPARE_NE(frameIter, rootFrame->end());
    QCOMPARE(frameIter.currentBlock().text(), QStringLiteral("Test for strikethrough tag workaround"));

    // Ordinary line.
    {
        ++frameIter;
        QCOMPARE_NE(frameIter, rootFrame->end());
        auto block = frameIter.currentBlock();
        QCOMPARE(block.text(), QStringLiteral("Line without strikethrough"));
        // Single format for the entire line.
        auto formats = block.textFormats();
        QCOMPARE(formats.size(), 1);
        QVERIFY(!formats[0].format.fontStrikeOut());
    }

    // Part of the line has a strikethrough.
    {
        ++frameIter;
        QCOMPARE_NE(frameIter, rootFrame->end());
        auto block = frameIter.currentBlock();
        QCOMPARE(block.text(), QStringLiteral("Line with strikethrough"));
        // The "with" should be the only thing striked out.
        auto formats = block.textFormats();
        QCOMPARE(formats.size(), 3);
        QCOMPARE(block.text().sliced(formats[0].start, formats[0].length), QStringLiteral("Line "));
        QVERIFY(!formats[0].format.fontStrikeOut());
        QCOMPARE(block.text().sliced(formats[1].start, formats[1].length), QStringLiteral("with"));
        QVERIFY(formats[1].format.fontStrikeOut());
        QCOMPARE(block.text().sliced(formats[2].start, formats[2].length), QStringLiteral(" strikethrough"));
        QVERIFY(!formats[2].format.fontStrikeOut());
    }

    // Code block shouldn't have leading spaces, or be modified by our fixup.
    {
        ++frameIter;
        QCOMPARE_NE(frameIter, rootFrame->end());
        auto block = frameIter.currentBlock();
        QCOMPARE(block.text(), QStringLiteral("~~Strikethrough~~ should be <del>ignored</del> in a <s>code block</s>"));
    }
}

void MarkdownTest::testHtmlTagFixup()
{
    const QString wrapperTag = QStringLiteral("ignored_by_qt");
    const QString wrapperTagBegin = QStringLiteral("<ignored_by_qt>");
    const QString wrapperTagEnd = QStringLiteral("</ignored_by_qt>");

    // These should passthrough unchanged.
    const QString testCases[] = {
        QStringLiteral("basic test"),
        QStringLiteral("<p>test with tag</p>"),
        QStringLiteral("line with <em>combined <strong>tags</strong></em>"),
    };
    for (const QString &inputHtml : testCases) {
        const QString outputHtml = Markdown::detail::fixupHtmlTags(QString(inputHtml));
        QCOMPARE(outputHtml, wrapperTagBegin + inputHtml + wrapperTagEnd);
    }

    // <del> should become <s>.
    {
        const QString outputHtml = Markdown::detail::fixupHtmlTags(QStringLiteral("<del>test</del>"));
        QCOMPARE(outputHtml, wrapperTagBegin + QStringLiteral("<s>test</s>") + wrapperTagEnd);
    }

    // Check that the wrapper is ignored by Qt.
    {
        QTextDocument dom;
        dom.setHtml(wrapperTagBegin + QStringLiteral("basic test") + wrapperTagEnd);
        QVERIFY(!dom.toHtml().contains(wrapperTag));
    }
}

QTEST_MAIN(MarkdownTest)
#include "markdowntest.moc"
