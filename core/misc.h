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

#include "area.h"

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
class TextSelection
{
    public:
    TextSelection( const NormalizedPoint & a, const NormalizedPoint & b )
    { 
          if (b.y-a.y<0 || (b.y-a.y==0 && b.x-a.x <0))
            direction=1;
          else
            direction=0;
          cur[0]=a,cur[1]=b;
          it[direction%2]=-1,it[(direction+1)%2]=-1;
    };
    void end( const NormalizedPoint & p )
    {
      // changing direction as in 2b , assuming the bool->int conversion is correct
      int dir1=direction;
      direction = (p.y-cur[0].y<0 || (p.y-cur[0].y==0 && p.x-cur[0].x <0));
      if (direction!=dir1)
        kDebug() << "changing direction in selection\n";
      cur[1]=p;
    }
    void itE (int p) { it[(direction+1)%2]=p; }
    void itB (int p) { it[(direction)%2]=p; }
    int dir () { return direction; }
    NormalizedPoint start() const { return cur[direction%2]; }
    NormalizedPoint end() const { return cur[(direction+1)%2]; }
    int itB() {return it[direction%2];}
    int itE() {return it[(direction+1)%2];}
    private:
    int direction;
    int it[2];
    NormalizedPoint cur[2];
};

}

#endif
