/**
  Natural order sorting of strings which contains numbers.

  Copyright 2007 Tobias Koenig <tokoe@kde.org>
  based on the natural order code by Martin Pool <mbp sourcefrog net>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License version 2 or at your option version 3 as published by
  the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef QNATSORT_H
#define QNATSORT_H

#include <QString>

/**
 * The two methods can be used in qSort to sort strings which contain
 * numbers in natural order.
 *
 * Normally strings are ordered like this: fam10g fam1g fam2g fam3g
 * natural ordered it would look like this: fam1g fam2g fam3g fam10g
 *
 * Code:
 *
 * QStringList list;
 * list << "fam10g" << "fam1g" << "fam2g" << "fam5g";
 *
 * qSort( list.begin(), list.end(), caseSensitiveNaturalOderLessThen );
 */

/**
 * Returns whether the @p left string is lesser than the @p right string
 * in natural order.
 */
bool caseSensitiveNaturalOrderLessThen(const QString &left, const QString &right);

/**
 * Returns whether the @p left string is lesser than the @p right string
 * in natural order and case insensitive.
 */
bool caseInsensitiveNaturalOrderLessThen(const QString &left, const QString &right);

#endif
