/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PAGE_PRIVATE_H_
#define _OKULAR_PAGE_PRIVATE_H_

// qt/kde includes
#include <qlinkedlist.h>
#include <qmatrix.h>
#include <qstring.h>

// local includes
#include "global.h"

namespace Okular {

class FormField;
class Link;
class Page;
class PageTransition;
class RotationJob;
class TextPage;

class PagePrivate
{
    public:
        PagePrivate( Page *page, uint n, double w, double h, Rotation o );
        ~PagePrivate();

        void imageRotationDone( RotationJob * job );
        QMatrix rotationMatrix() const;

        Page *m_page;
        int m_number;
        Rotation m_orientation;
        double m_width, m_height;
        Rotation m_rotation;
        int m_maxuniqueNum;

        TextPage * m_text;
        PageTransition * m_transition;
        QLinkedList< FormField * > formfields;
        Link * m_openingAction;
        Link * m_closingAction;
        double m_duration;
        QString m_label;
};

}

#endif
