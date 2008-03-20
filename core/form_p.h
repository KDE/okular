/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_FORM_P_H
#define OKULAR_FORM_P_H

#include "form.h"

#include <QtCore/QString>

namespace Okular {

class Action;
class FormField;

class FormFieldPrivate
{
    public:
        FormFieldPrivate( FormField::FieldType type );
        virtual ~FormFieldPrivate();

        void setDefault();

        virtual void setValue( const QString& ) = 0;
        virtual QString value() const = 0;

        FormField::FieldType m_type;
        QString m_default;
        Action *m_activateAction;

        Q_DECLARE_PUBLIC( FormField )
        FormField *q_ptr;
};

}

#endif
