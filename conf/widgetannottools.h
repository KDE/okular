/***************************************************************************
 *   Copyright (C) 2012 by Fabio D'Urso <fabiodurso@hotmail.it>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _WIDGETANNOTTOOLS_H_
#define _WIDGETANNOTTOOLS_H_

#include "widgetconfigurationtoolsbase.h"

#include <qdom.h>
#include <qwidget.h>

namespace Okular
{
class Annotation;
}

class WidgetAnnotTools : public WidgetConfigurationToolsBase
{
    Q_OBJECT
    public:
        explicit WidgetAnnotTools( QWidget * parent = Q_NULLPTR );
        ~WidgetAnnotTools();

        QStringList tools() const Q_DECL_OVERRIDE;
        void setTools(const QStringList& items) Q_DECL_OVERRIDE;

    protected Q_SLOTS:
        void slotAdd() Q_DECL_OVERRIDE;
        void slotEdit() Q_DECL_OVERRIDE;
};

#endif
