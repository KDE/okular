/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qcolor.h>

// local includes
#include "annotations.h"

/** Annotation **/

Annotation::Annotation()
{
    state = Opened;
    flags = 0;
}

Annotation::~Annotation()
{
}

/** TextAnnotation **/

TextAnnotation::TextAnnotation()
    : subType( InPlace ) {}


/** LineAnnotation **/

LineAnnotation::LineAnnotation()
    : srcArrow( false ), dstArrow( false ), width( 2 ) {}


/** GeomAnnotation **/

GeomAnnotation::GeomAnnotation()
    : subType( Square ) {}


/** PathAnnotation **/

PathAnnotation::PathAnnotation()
    : subType( Ink ) {}


/** HighlightAnnotation **/

HighlightAnnotation::HighlightAnnotation()
    : subType( Highlight ) {}


/** StampAnnotation **/

StampAnnotation::StampAnnotation() {}


/** MediaAnnotation **/

MediaAnnotation::MediaAnnotation()
    : subType( URL ) {}
