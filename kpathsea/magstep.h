/* magstep.h: declaration for magstep fixing.

Copyright (C) 1994 Karl Berry.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef KPATHSEA_MAGSTEP_H
#define KPATHSEA_MAGSTEP_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* If DPI is close enough to some magstep of BDPI, return the true dpi
   value, and the magstep found (or zero) in M_RET (if
   non-null). ``Close enough'' means within one pixel.
   
   M_RET is slightly encoded: the least significant bit is on for a
   half-magstep, off otherwise.  Thus, a returned M_RET of 1 means
   \magstephalf, i.e., sqrt(1.2), i.e., 1.09544.  Put another way,
   return twice the number of magsteps.
   
   In practice, this matters for magstephalf.  Floating-point computation
   with the fixed-point DVI representation leads to 328 (for BDPI ==
   300); specifying `at 11pt' yields 330; the true \magstephalf is 329
   (that's what you get if you run Metafont with mag:=magstep(.5)).
   
   The time to call this is after you read the font spec from the DVI
   file, but before you look up any files -- do the usual floating-point
   computations, and then fix up the result.  */

extern unsigned kpse_magstep_fix P3H(unsigned dpi, unsigned bdpi, int *m_ret);

#endif /* not KPATHSEA_MAGSTEP_H */
