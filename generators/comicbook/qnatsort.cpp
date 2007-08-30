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

#include "qnatsort.h"

static int compare_right( const QString &leftStr, int left, const QString &rightStr, int right )
{
  int bias = 0;
     
  /**
   * The longest run of digits wins.  That aside, the greatest
	 * value wins, but we can't know that it will until we've scanned
	 * both numbers to know that they have the same magnitude, so we
	 * remember it in BIAS.
   */
  for ( ;; left++, right++ ) {
    if ( !leftStr[ left ].isDigit() && !rightStr[ right ].isDigit() )
      return bias;
    else if ( !leftStr[ left ].isDigit() )
      return -1;
    else if ( !rightStr[ right ].isDigit() )
      return +1;
    else if ( leftStr[ left ] < rightStr[ right ] ) {
      if ( !bias )
        bias = -1;
    } else if ( leftStr[ left ] > rightStr[ right ] ) {
      if ( !bias )
        bias = +1;
    } else if ( leftStr[ left ].isNull() && rightStr[ right ].isNull() )
      return bias;
  }

  return 0;
}

static int compare_left( const QString &leftStr, int left, const QString &rightStr, int right )
{
  /**
   * Compare two left-aligned numbers: the first to have a
   * different value wins.
   */
  for ( ;; left++, right++ ) {
    if ( !leftStr[ left ].isDigit() && !rightStr[ right ].isDigit() )
      return 0;
    else if ( !leftStr[ left ].isDigit() )
      return -1;
    else if ( !rightStr[ right ].isDigit() )
      return +1;
    else if ( leftStr[ left ] < rightStr[ right ] )
      return -1;
    else if ( leftStr[ left ] > rightStr[ right ] )
      return +1;
  }

  return 0;
}

static int natural_order_compare( const QString &leftStr, const QString &rightStr, bool fold_case )
{
  if ( leftStr.isEmpty() && rightStr.isEmpty() )
    return 0;

  int ai, bi;
  QChar ca, cb;
  int fractional, result;
     
  ai = bi = 0;

  while ( true ) {
    ca = leftStr[ ai ]; cb = rightStr[ bi ];

    /* skip over leading spaces or zeros */
    while ( ca.isSpace() )
      ca = leftStr[ ++ai ];

    while ( cb.isSpace() )
      cb = rightStr[ ++bi ];

    /* process run of digits */
    if ( ca.isDigit() && cb.isDigit() ) {
      fractional = (ca == '0' || cb == '0');

      if ( fractional ) {
        if ( (result = compare_left( leftStr, ai, rightStr, bi )) != 0 )
          return result;
      } else {
        if ( (result = compare_right( leftStr, ai, rightStr, bi )) != 0 )
          return result;
      }
    }

    if ( ca.isNull() && cb.isNull() ) {
      /* The strings compare the same.  Perhaps the caller
         will want to call strcmp to break the tie. */
      return 0;
    }

    if ( fold_case ) {
      ca = ca.toUpper();
      cb = cb.toUpper();
    }
	  
    if ( ca < cb )
      return -1;
    else if ( ca > cb )
      return +1;

    ++ai; ++bi;
  }
}

bool caseSensitiveNaturalOrderLessThen( const QString &left, const QString &right )
{
  return (natural_order_compare( left, right, false ) < 0);
}

bool caseInsensitiveNaturalOrderLessThen( const QString &left, const QString &right )
{
  return (natural_order_compare( left, right, true ) < 0);
}
