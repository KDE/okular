/***************************************************************************
 *   Copyright (C) 2008 by Brad Hards <bradh@frogmouth.net>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef _OKULAR_MSTIFFTAGS_H
#define _OKULAR_MSTIFFTAGS_H

static const int MSTIFFTAG_TEXT = 37679;

static const TIFFFieldInfo msFieldInfo[] = {
    { MSTIFFTAG_TEXT,      -1,     -1,     TIFF_UNDEFINED,      FIELD_CUSTOM,   1,      1,      "AssumedMSText" }
};

#define N(a)    (sizeof (a) / sizeof (a[0]))

void _MsTiffTagExtender(TIFF *tif)
{
    /* Install the extended Tag field info */
    TIFFMergeFieldInfo(tif, msFieldInfo, N(msFieldInfo));
}

#endif
