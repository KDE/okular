/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *            Based on code written by Bill Janssen 2002                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef IMAGE_H
#define IMAGE_H

#include "unpluck.h"

class QImage;

bool TranscribePalmImageToJPEG(unsigned char *image_bytes_in, QImage &image);
bool TranscribeMultiImageRecord(plkr_Document *doc, QImage &image, unsigned char *bytes);

#endif
