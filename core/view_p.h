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

#include <QHash>
#include <QString>

namespace Okular
{
class DocumentPrivate;
class View;

class ViewPrivate
{
public:
    ViewPrivate();
    virtual ~ViewPrivate();

    ViewPrivate(const ViewPrivate &) = delete;
    ViewPrivate &operator=(const ViewPrivate &) = delete;

    QString name;
    DocumentPrivate *document;
};

}

#endif
