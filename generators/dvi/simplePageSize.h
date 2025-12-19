// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
//
// simplePageSize.h
//
// Part of KVIEWSHELL - A framework for multipage text/gfx viewers
//
// SPDX-FileCopyrightText: 2002-2004 Stefan Kebekus
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SIMPLEPAGESIZE_H
#define SIMPLEPAGESIZE_H

#include "length.h"

#include <QSize>

class QPaintDevice;

/** \brief This class represents physical page sizes.

This class represents page sizes. It contains nothing but two numbers,
the page width, and page height, and a few utility functions that
convert page sizes to pixel sizes and to compute the aspect ratio. A
page with width<=1mm or height<=1mm is considered invalid. pageSize is
a more elaborate class that is derived from SimplePageSize and knows
about standard paper sizes.

@author Stefan Kebekus <kebekus@kde.org>
@version 1.0 0
*/

class SimplePageSize
{
public:
    /** Constructs an invalid SimplePageSize, with size 0x0mm */
    SimplePageSize()
    {
        pageWidth.setLength_in_mm(0.0);
        pageHeight.setLength_in_mm(0.0);
    }

    ~SimplePageSize()
    {
    }

    /** \brief Returns the page width. */
    Length width() const
    {
        return pageWidth;
    }

    /** \brief Returns the page height. */
    Length height() const
    {
        return pageHeight;
    }

    /** \brief Converts the physical page size to a pixel size

    @param resolution in dots per inch

    @returns the pixel size, represented by a QSize. If the page size is
        invalid, the result is undefined. */
    QSize sizeInPixel(double resolution) const
    {
        return QSize((int)(resolution * pageWidth.getLength_in_inch() + 0.5), (int)(resolution * pageHeight.getLength_in_inch() + 0.5));
    }

    /** \brief Validity check

    @returns 'True' if the page width and height are both larger than
    1mm */
    bool isValid() const
    {
        return ((pageWidth.getLength_in_mm() > 1.0) && (pageHeight.getLength_in_mm() > 1.0));
    }

protected:
    Length pageWidth;
    Length pageHeight;
};

#endif
