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

FormField::FormField( FormField::FieldType t )
    : m_type( t )
{
}

FormField::~FormField()
{
}

FormField::FieldType FormField::type() const
{
    return m_type;
}

bool FormField::isReadOnly() const
{
    return false;
}

bool FormField::isVisible() const
{
    return true;
}


FormFieldText::FormFieldText()
    : FormField( FormField::FormText )
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


FormFieldChoice::FormFieldChoice()
    : FormField( FormField::FormChoice )
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

