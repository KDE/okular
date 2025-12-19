// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
//
// Class: length
//
// Part of KVIEWSHELL
//
// SPDX-FileCopyrightText: 2005 Stefan Kebekus
// SPDX-FileCopyrightText: 2006 Wilfried Huss
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _length_h_
#define _length_h_

#include <cmath>
#include <math.h>

class QString;

#define mm_per_inch 25.4

/** @short Represents a phyical length

    This class is used to represent a physical length. Its main purpose
    it to help in the conversion of units, and to avoid confusion
    about units. To avoid misunderstandings, there is no default
    constructor so that this class needs to be explicitly initialized
    with one of the functions below.

    @warning Lengths are stored internally in mm. If you convert to
    or from any other unit, expect floating point round-off errors.

    @author Stefan Kebekus <kebekus@kde.org>
    @version 1.0.0
*/

class Length
{
public:
    /** constructs a 'length = 0mm' object */
    Length()
    {
        length_in_mm = 0;
    }

    /** sets the length in millimeters */
    void setLength_in_mm(double l)
    {
        length_in_mm = l;
    }

    /** sets the length in inches */
    void setLength_in_inch(double l)
    {
        length_in_mm = l * mm_per_inch;
    }

    /** @returns the length in millimeters */
    double getLength_in_mm() const
    {
        return length_in_mm;
    }

    /** @returns the length in inches */
    double getLength_in_inch() const
    {
        return length_in_mm / mm_per_inch;
    }

    /** @returns the length in pixel. The parameter @param res is the resolution of the
        used device in DPI. */
    int getLength_in_pixel(double res) const
    {
        return int(getLength_in_inch() * res);
    }

    /** Comparison of two lengths */
    bool operator>(const Length o) const
    {
        return (length_in_mm > o.getLength_in_mm());
    }
    bool operator<(const Length o) const
    {
        return (length_in_mm < o.getLength_in_mm());
    }

    /** Comparison of two lengths */
    bool operator>=(const Length o) const
    {
        return (length_in_mm >= o.getLength_in_mm());
    }
    bool operator<=(const Length o) const
    {
        return (length_in_mm <= o.getLength_in_mm());
    }

    /** Ratio of two lengths

    @warning There is no safeguard to prevent you from division by
    zero. If the length in the denominator is near 0.0, a floating point
    exception may occur.

    @returns the ratio of the two lengths as a double
    */
    double operator/(const Length o) const
    {
        return (length_in_mm / o.getLength_in_mm());
    }

    /** Sum of two lengths

    @returns the sum of the lengths as a Length
    */
    Length operator+(const Length o) const
    {
        Length r;
        r.length_in_mm = length_in_mm + o.length_in_mm;
        return r;
    }

    /** Difference of two lengths

    @returns the difference of the lengths as a Length
    */
    Length operator-(const Length o) const
    {
        Length r;
        r.length_in_mm = length_in_mm - o.length_in_mm;
        return r;
    }

    /** Division of a length

    @warning There is no safeguard to prevent you from division by
    zero. If the number in the denominator is near 0.0, a floating point
    exception may occur.

    @returns a fraction of the original length as a Length
    */
    Length operator/(const double l) const
    {
        Length r;
        r.length_in_mm = length_in_mm / l;
        return r;
    }

    /** Multiplication of a length

    @returns a multiplied length as a Length
    */
    Length operator*(const double l) const
    {
        Length r;
        r.length_in_mm = length_in_mm * l;
        return r;
    }

    /** This method converts a string that gives a distance in one of the
    commonly used units, such as "12.3mm", "12 inch" or "15 didot" to
    millimeters. For a complete list of supported units, see the
    static lists that are hardcoded in "units.cpp".

    If the conversion is not possible *ok is set to "false" and an
    undefined value is returned. If the unit could not be recognized,
    an error message is printed via kdError(). Otherwise, *ok is set
    to true.

    It is possible in rare circumstances that ok is set to true
    although the string is malformed.

    It is fine to set ok to 0. */
    static float convertToMM(const QString &distance, bool *ok = nullptr);

private:
    /** Length in millimeters */
    double length_in_mm;
};

#undef mm_per_inch

#endif
