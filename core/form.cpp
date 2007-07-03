/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// local includes
#include "form.h"

using namespace Okular;

class Okular::FormFieldPrivate
{
    public:
        FormFieldPrivate( FormField::FieldType type )
            : m_type( type )
        {
        }

        FormField::FieldType m_type;
};

FormField::FormField( FormFieldPrivate &dd )
    : d_ptr( &dd )
{
}

FormField::~FormField()
{
    delete d_ptr;
}

FormField::FieldType FormField::type() const
{
    Q_D( const FormField );
    return d->m_type;
}

bool FormField::isReadOnly() const
{
    return false;
}

bool FormField::isVisible() const
{
    return true;
}


class Okular::FormFieldTextPrivate : public Okular::FormFieldPrivate
{
    public:
        FormFieldTextPrivate()
            : FormFieldPrivate( FormField::FormText )
        {
        }
};


FormFieldText::FormFieldText()
    : FormField( *new FormFieldTextPrivate() )
{
}

FormFieldText::~FormFieldText()
{
}

void FormFieldText::setText( const QString& )
{
}

bool FormFieldText::isPassword() const
{
    return false;
}

bool FormFieldText::isRichText() const
{
    return false;
}

int FormFieldText::maximumLength() const
{
    return -1;
}

Qt::Alignment FormFieldText::textAlignment() const
{
    return Qt::AlignVCenter | Qt::AlignLeft;
}

bool FormFieldText::canBeSpellChecked() const
{
    return false;
}


class Okular::FormFieldChoicePrivate : public Okular::FormFieldPrivate
{
    public:
        FormFieldChoicePrivate()
            : FormFieldPrivate( FormField::FormChoice )
        {
        }
};


FormFieldChoice::FormFieldChoice()
    : FormField( *new FormFieldChoicePrivate() )
{
}

FormFieldChoice::~FormFieldChoice()
{
}

bool FormFieldChoice::isEditable() const
{
    return false;
}

bool FormFieldChoice::multiSelect() const
{
    return false;
}

void FormFieldChoice::setCurrentChoices( const QList< int >& )
{
}

Qt::Alignment FormFieldChoice::textAlignment() const
{
    return Qt::AlignVCenter | Qt::AlignLeft;
}

bool FormFieldChoice::canBeSpellChecked() const
{
    return false;
}

