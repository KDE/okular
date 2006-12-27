/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef ANNOTATIONGUIUTILS_H
#define ANNOTATIONGUIUTILS_H

namespace Okular {
class Annotation;
}

class AnnotationGuiUtils
{
    public:
        /**
         * Returns the translated string with the type of the given @p annotation.
         */
        static QString captionForAnnotation( Okular::Annotation * annotation );
};


#endif
