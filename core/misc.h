/***************************************************************************
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_MISC_H_
#define _OKULAR_MISC_H_

#include <okular/core/okular_export.h>
#include <okular/core/area.h>

namespace Okular {

/**
  @short Wrapper around the information needed to generate the selection area
  There are two assumptions inside this class:
  1. the start never changes, one instance of this class is used for one selection,
     therefore the start of the selection will not change, only end and direction of 
     the selection will change.
     By direction we mean the direction in which the end moves in relation to the start, 
     forward selection is when end is after the start, backward when its before.

  2. The following changes might appear during selection:
    a. the end moves without changing the direction (it can move up and down but not past the start): 
       only itE will be updated
    b. the end moves with changing the direction then itB becomes itE if the previous direction was forward
       or itE becomes itB 

  3. Internally it that is related to the start cursor is always at it[0] while it related to end is it[1],
     transition between meanings (itB/itE) is done with dir modifier;
*/
class OKULAR_EXPORT TextSelection
{
    public:
        /**
         * Creates a new text selection with the given @p start and @p end point.
         */
        TextSelection( const NormalizedPoint &start, const NormalizedPoint &end );

        /**
         * Destroys the text selection.
         */
        ~TextSelection();

        /**
         * Changes the end point of the selection to the given @p point.
         */
        void end( const NormalizedPoint &point );

        void itE( int pos );
        void itB( int pos );

        /**
         * Returns the direction of the selection.
         */
        int direction() const;

        /**
         * Returns the start point of the selection.
         */
        NormalizedPoint start() const;

        /**
         * Returns the end point of the selection.
         */
        NormalizedPoint end() const;

        int itB() const;
        int itE() const;

    private:
        class Private;
        Private* const d;
};

}

#endif
