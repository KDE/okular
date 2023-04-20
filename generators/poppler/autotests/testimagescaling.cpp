/*
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "imagescaling.h"
#include <QGuiApplication>
#include <QImage>
#include <QTest>

// Tests the image scaling and fit to canvas that is
// used by the signature element in poppler.

// For this test, we don't care about the quality of the
// scaling but purely rely on Qt for that. It is mostly a
// test that we place the input image correctly in returned
// image

class ImageScalingTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testNoScaling();
    void testNoPaddingEnlarge();
    void testNoPaddingShrink();
    void testPadLeftRight();
    void testPadTopBottom();
};

void ImageScalingTest::testNoScaling()
{
    QSize size(5, 5);
    QImage input(size, QImage::Format_ARGB32);
    input.fill(Qt::black);
    QImage output = imagescaling::scaleAndFitCanvas(input, size);
    QImage expected(size, QImage::Format_ARGB32);
    expected.fill(Qt::black);
    QCOMPARE(expected, output);
}

void ImageScalingTest::testNoPaddingEnlarge()
{
    QImage input(QSize(10, 10), QImage::Format_ARGB32);
    input.fill(Qt::black);
    QSize size(20, 20);
    QImage output = imagescaling::scaleAndFitCanvas(input, size);
    QImage expected(size, QImage::Format_ARGB32);
    expected.fill(Qt::black);
    QCOMPARE(expected, output);
}

void ImageScalingTest::testNoPaddingShrink()
{
    QImage input(QSize(20, 20), QImage::Format_ARGB32);
    input.fill(Qt::black);
    QSize size(10, 10);
    QImage output = imagescaling::scaleAndFitCanvas(input, size);
    QImage expected(size, QImage::Format_ARGB32);
    expected.fill(Qt::black);
    QCOMPARE(expected, output);
}

void ImageScalingTest::testPadLeftRight()
{
    QImage input(QSize(5, 5), QImage::Format_ARGB32);
    input.fill(Qt::black);
    QSize size(9, 5);
    QImage output = imagescaling::scaleAndFitCanvas(input, size);
    // check top row
    QCOMPARE(output.pixelColor(0, 0), Qt::transparent);
    QCOMPARE(output.pixelColor(1, 0), Qt::transparent);
    QCOMPARE(output.pixelColor(2, 0), Qt::black);
    QCOMPARE(output.pixelColor(3, 0), Qt::black);
    QCOMPARE(output.pixelColor(4, 0), Qt::black);
    QCOMPARE(output.pixelColor(5, 0), Qt::black);
    QCOMPARE(output.pixelColor(6, 0), Qt::black);
    QCOMPARE(output.pixelColor(7, 0), Qt::transparent);
    QCOMPARE(output.pixelColor(8, 0), Qt::transparent);
    // check columns
    for (int x = 0; x < output.size().width(); x++) {
        QColor top = output.pixelColor(x, 0);
        for (int y = 1; y < output.size().height(); y++) {
            QCOMPARE(output.pixelColor(x, y), top);
        }
    }
}

void ImageScalingTest::testPadTopBottom()
{
    QImage input(QSize(5, 5), QImage::Format_ARGB32);
    input.fill(Qt::black);
    QSize size(5, 9);
    QImage output = imagescaling::scaleAndFitCanvas(input, size);
    // check first column
    QCOMPARE(output.pixelColor(0, 0), Qt::transparent);
    QCOMPARE(output.pixelColor(0, 1), Qt::transparent);
    QCOMPARE(output.pixelColor(0, 2), Qt::black);
    QCOMPARE(output.pixelColor(0, 3), Qt::black);
    QCOMPARE(output.pixelColor(0, 4), Qt::black);
    QCOMPARE(output.pixelColor(0, 5), Qt::black);
    QCOMPARE(output.pixelColor(0, 6), Qt::black);
    QCOMPARE(output.pixelColor(0, 7), Qt::transparent);
    QCOMPARE(output.pixelColor(0, 8), Qt::transparent);
    // check rows
    for (int y = 0; y < output.size().height(); y++) {
        QColor first = output.pixelColor(0, y);
        for (int x = 1; x < output.size().width(); x++) {
            QCOMPARE(output.pixelColor(x, y), first);
        }
    }
}

QTEST_MAIN(ImageScalingTest)
#include "testimagescaling.moc"

// No need to export it, but we need to be able to call the functions
#include "imagescaling.cpp"
