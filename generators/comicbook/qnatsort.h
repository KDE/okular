/**
  Natural order sorting of strings which contains numbers.

  Copyright 2007 Tobias Koenig <tokoe@kde.org>

  based on the natural order code by Martin Pool <mbp sourcefrog net>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <QtCore/QString>

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
 * Returns whether the @p left string is lesser then the @p right string
 * in natural order.
 */
bool caseSensitiveNaturalOrderLessThen( const QString &left, const QString &right );

/**
 * Returns whether the @p left string is lesser then the @p right string
 * in natural order and case insensitive.
 */
bool caseInsensitiveNaturalOrderLessThen( const QString &left, const QString &right );
