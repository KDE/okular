/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt / kde includes
#include <qrect.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qimage.h>

// local includes
#include "annotations.h"

Annotation::Annotation()
    : NormalizedRect()
{
}

Annotation::~Annotation()
{
}
/*
void mousePressEvent( double x, double y, Qt::ButtonState b );
void mouseMoveEvent( double x, double y, Qt::ButtonState b );
void mouseReleaseEvent( double x, double y, Qt::ButtonState b );
void overlayPaint( QPainter * painter );
void finalPaint( QPixmap * pixmap, MouseState mouseState );
*/
