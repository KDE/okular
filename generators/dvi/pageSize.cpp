// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
// pageSize.cpp
//
// Part of KVIEWSHELL - A framework for multipage text/gfx viewers
//
// SPDX-FileCopyrightText: 2002-2003 Stefan Kebekus
// SPDX-License-Identifier: GPL-2.0-or-later

#include <config.h>

#include "debug_dvi.h"
#include "length.h"
#include "pageSize.h"

#include <KLocalizedString>

#include <QLocale>
#include <QLoggingCategory>

struct pageSizeItem {
    const char *name;
    float width;  // in mm
    float height; // in mm
};

#define defaultMetricPaperSize 4   // Default paper size is "DIN A4"
#define defaultImperialPaperSize 8 // Default paper size is "US Letter"

static const pageSizeItem staticList[] = {
    {"DIN A0", 841.0f, 1189.0f},
    {"DIN A1", 594.0f, 841.0f},
    {"DIN A2", 420.0f, 594.0f},
    {"DIN A3", 297.0f, 420.0f},
    {"DIN A4", 210.0f, 297.0f},
    {"DIN A5", 148.5f, 210.0f},
    {"DIN B4", 250.0f, 353.0f},
    {"DIN B5", 176.0f, 250.0f},
    {"US Letter", 215.9f, 279.4f},
    {"US Legal", 215.9f, 355.6f},
    {nullptr, 0.0f, 0.0f} // marks the end of the list.
};

pageSize::pageSize()
{
    const int sizeIndex = defaultPageSizeIndex();
    pageWidth.setLength_in_mm(staticList[sizeIndex].width);
    pageHeight.setLength_in_mm(staticList[sizeIndex].height);
}

bool pageSize::setPageSize(const QString &name)
{
    // See if we can recognize the string
    for (int i = 0; staticList[i].name != nullptr; i++) {
        QString currentName = QString::fromLocal8Bit(staticList[i].name);
        if (currentName == name) {
            // Set page width/height accordingly
            pageWidth.setLength_in_mm(staticList[i].width);
            pageHeight.setLength_in_mm(staticList[i].height);
            return true;
        }
    }

    // Check if the string contains 'x'. If yes, we assume it is of type
    // "<number>x<number>". If yes, the first number is interpreted as
    // the width in mm, the second as the height in mm
    if (name.indexOf(QLatin1Char('x')) >= 0) {
        bool wok, hok;
        float pageWidth_tmp = name.section(QLatin1Char('x'), 0, 0).toFloat(&wok);
        float pageHeight_tmp = name.section(QLatin1Char('x'), 1, 1).toFloat(&hok);
        if (wok && hok) {
            pageWidth.setLength_in_mm(pageWidth_tmp);
            pageHeight.setLength_in_mm(pageHeight_tmp);

            rectifySizes();
            reconstructCurrentSize();
            return true;
        }
    }

    // Check if the string contains ','. If yes, we assume it is of type
    // "<number><unit>,<number><uni>". The first number is supposed to
    // be the width, the second the height.
    if (name.indexOf(QLatin1Char(',')) >= 0) {
        bool wok, hok;
        float pageWidth_tmp = Length::convertToMM(name.section(QLatin1Char(','), 0, 0), &wok);
        float pageHeight_tmp = Length::convertToMM(name.section(QLatin1Char(','), 1, 1), &hok);
        if (wok && hok) {
            pageWidth.setLength_in_mm(pageWidth_tmp);
            pageHeight.setLength_in_mm(pageHeight_tmp);

            rectifySizes();
            reconstructCurrentSize();
            return true;
        }
    }

    // Last resource. Set the default, in case the string is
    // unintelligible to us.
    const int sizeIndex = defaultPageSizeIndex();
    pageWidth.setLength_in_mm(staticList[sizeIndex].width);
    pageHeight.setLength_in_mm(staticList[sizeIndex].height);
    qCCritical(OkularDviShellDebug) << "pageSize::setPageSize: could not parse '" << name << "'. Using " << staticList[sizeIndex].name << " as a default.";
    return false;
}

void pageSize::rectifySizes()
{
    // Now do some sanity checks to make sure that values are not
    // outrageous. We allow values between 5cm and 50cm.
    if (pageWidth.getLength_in_mm() < 50) {
        pageWidth.setLength_in_mm(50.0);
    }
    if (pageWidth.getLength_in_mm() > 1200) {
        pageWidth.setLength_in_mm(1200);
    }
    if (pageHeight.getLength_in_mm() < 50) {
        pageHeight.setLength_in_mm(50);
    }
    if (pageHeight.getLength_in_mm() > 1200) {
        pageHeight.setLength_in_mm(1200);
    }
    return;
}

void pageSize::reconstructCurrentSize()
{
    for (int i = 0; staticList[i].name != nullptr; i++) {
        if ((fabs(staticList[i].width - pageWidth.getLength_in_mm()) <= 2) && (fabs(staticList[i].height - pageHeight.getLength_in_mm()) <= 2)) {
            pageWidth.setLength_in_mm(staticList[i].width);
            pageHeight.setLength_in_mm(staticList[i].height);
            return;
        }
        if ((fabs(staticList[i].height - pageWidth.getLength_in_mm()) <= 2) && (fabs(staticList[i].width - pageHeight.getLength_in_mm()) <= 2)) {
            pageWidth.setLength_in_mm(staticList[i].height);
            pageHeight.setLength_in_mm(staticList[i].width);
            return;
        }
    }
}

int pageSize::defaultPageSizeIndex()
{
    // FIXME: static_cast<QPrinter::PageSize>(KLocale::global()->pageSize())
    //        is the proper solution here.  Then you can determine the values
    //        without using your hardcoded table too!
    if (QLocale::system().measurementSystem() == QLocale::MetricSystem) {
        return defaultMetricPaperSize;
    } else {
        return defaultImperialPaperSize;
    }
}
