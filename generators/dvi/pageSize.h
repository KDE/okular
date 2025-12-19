// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
//
// pageSize.h
//
// Part of KVIEWSHELL - A framework for multipage text/gfx viewers
//
// SPDX-FileCopyrightText: 2002-2005 Stefan Kebekus
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PAGESIZE_H
#define PAGESIZE_H

#include "simplePageSize.h"

class QString;

/* \brief This class represents physical page sizes.

The main difference to the SimplePageSize class are the following.

- This class knows about standard page sizes and accepts page sizes in
  various formats, e.g. as a string "DIN A4", or by specifying the
  page width and height. Several units (inch, millimeters,
  centimeters) are possible.

- It is made sure that page width an height are always in a reasonable
  range, which is currently set to 5cm .. 50cm

- The default constructor provides a locale-depending default.

@author Stefan Kebekus <kebekus@kde.org>
@version 1.0.0
*/

class pageSize : public SimplePageSize
{
public:
    /** \brief Default constructor, initializes the pageSize with a
        reasonable default

        The default chosen depends on the locale. At the moment, A4 size
        is chosen for countries with metric measurement system, and US
        letter otherwise.
    */
    pageSize();

    pageSize(const pageSize &) = delete;
    pageSize &operator=(const pageSize &) = delete;

    /** \brief Set page size by name.

    Acceptable strings are

    (1) a name from the staticList defined in the cpp file as "DIN
        A4"

    (2) a string like "500x300", which describes a page of width 500mm
        and height 300mm.

    (3) a string like "3in, 4 cm". A number of different units,
        including "in", "mm" and "cm", and a few typographical units are
        recognized

    If the name is not of these types, and error message is printed to
    stderr using kError() and a default value, which depends on the
    locale, is set.

    In any case, the values will be trimmed so as not to exceed the
    minima/maxima of 5cm and 50cm, respectively. If the page size found
    matches one of the standard sizes by an error of no more than 2mm,
    the standard page size will be set.

    @param name string that represents the page size

    @returns 'True', if the parameter could be parsed, and 'false'
    otherwise.
    */
    bool setPageSize(const QString &name);

private:
    /** Makes sure that pageWidth and pageHeight are in the permissible
        range and not, e.g., negative. */
    void rectifySizes();

    /** Tries to find one of the known sizes which matches pageWidth and
        pageHeight, with an error margin of 2 millimeters. If found, the
        value of 'currentsize' is set to point to the known size, and
        pageWidth and pageHeight are set to the correct values for that
        size. Otherwise, currentSize is set to -1 to indicate "custom
        size". Note: this method does not take care of orientation,
        e.g. the method will set 'currentsize' to point to "DIN A4" if
        either the page size is 210x297 or 297x210. */
    void reconstructCurrentSize();

    /** Gives a default value for currentSize, which depends on the
        locale. In countries with metric system, this will be "DIN A4",
        in countries with the imperial system, "US Letter". */
    static int defaultPageSizeIndex();
};

#endif
