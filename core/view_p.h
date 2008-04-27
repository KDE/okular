/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_VIEW_P_H
#define OKULAR_VIEW_P_H

#include <QtCore/QHash>
#include <QtCore/QString>

namespace Okular {

class DocumentPrivate;
class View;

class ViewPrivate
{
    public:
        ViewPrivate();
        virtual ~ViewPrivate();

        QString name;
        DocumentPrivate *document;
};

}

#endif
