/* magstep.c: fix up fixed-point vs. floating-point.

Copyright (C) 1994, 95 Karl Berry.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <kpathsea/config.h>

#include <kpathsea/magstep.h>


/* Return true magstep N, where the lsb of N means ``half'' (see
   magstep.h) for resolution BDPI.  From Tom Rokicki's dvips.  */

static int
magstep P2C(int, n,  int, bdpi)
{
   double t;
   int step;
   int neg = 0;

   if (n < 0)
     {
       neg = 1;
       n = -n;
     }

   if (n & 1)
     {
       n &= ~1;
       t = 1.095445115;
     }
    else
      t = 1.0;

   while (n > 8)
     {
       n -= 8;
       t = t * 2.0736;
     }

   while (n > 0)
     {
       n -= 2;
       t = t * 1.2;
     }

   /* Unnecessary casts to shut up stupid compilers. */
   step = (int)(0.5 + (neg ? bdpi / t : bdpi * t));
   return step;
}

/* This is adapted from code written by Tom Rokicki for dvips.  It's
   part of Kpathsea now so all the drivers can use it.  The idea is to
   return the true dpi corresponding to DPI with a base resolution of
   BDPI.  If M_RET is non-null, we also set that to the mag value.  */
   
/* Don't bother trying to use fabs or some other ``standard'' routine
   which can only cause trouble; just roll our own simple-minded
   absolute-value function that is all we need.  */
#undef ABS /* be safe */
#define ABS(expr) ((expr) < 0 ? -(expr) : (expr))

#define MAGSTEP_MAX 40

unsigned
kpse_magstep_fix P3C(unsigned, dpi,  unsigned, bdpi,  int *, m_ret)
{
  int m;
  int mdpi = -1;
  unsigned real_dpi = 0;
  int sign = dpi < bdpi ? -1 : 1; /* negative or positive magsteps? */
  
  for (m = 0; !real_dpi && m < MAGSTEP_MAX; m++) /* don't go forever */
    {
      mdpi = magstep (m * sign, bdpi);
      if (ABS (mdpi - (int) dpi) <= 1) /* if this magstep matches, quit */
        real_dpi = mdpi;
      else if ((mdpi - (int) dpi) * sign > 0) /* if gone too far, quit */
        real_dpi = dpi;
    }
  
  /* If requested, return the encoded magstep (the loop went one too far).  */
  /* More unnecessary casts. */
  if (m_ret)
    *m_ret = real_dpi == (unsigned)(mdpi ? (m - 1) * sign : 0);

  /* Always return the true dpi found.  */
  return real_dpi ? real_dpi : dpi;
}
