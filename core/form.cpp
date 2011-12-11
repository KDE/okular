/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "form.h"
#include "form_p.h"

// qt includes
#include <QtCore/QVariant>

#include "action.h"

using namespace Okular;

FormFieldPrivate::FormFieldPrivate( FormField::FieldType type )
    : m_type( type ), m_activateAction( 0 )
{
}

FormFieldPrivate::~FormFieldPrivate()
{
    delete m_activateAction;
}

void FormFieldPrivate::setDefault()
{
    m_default = value();
}


FormField::FormField( FormFieldPrivate &dd )
    : d_ptr( &dd )
{
    d_ptr->q_ptr = this;
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

Action* FormField::activationAction() const
{
    Q_D( const FormField );
    return d->m_activateAction;
}

void FormField::setActivationAction( Action *action )
{
    Q_D( FormField );
    delete d->m_activateAction;
    d->m_activateAction = action;
}


class Okular::FormFieldButtonPrivate : public Okular::FormFieldPrivate
{
    public:
        FormFieldButtonPrivate()
            : FormFieldPrivate( FormField::FormButton )
        {
        }

        Q_DECLARE_PUBLIC( FormFieldButton )

        void setValue( const QString& v )
        {
            Q_Q( FormFieldButton );
            q->setState( QVariant( v ).toBool() );
        }

        QString value() const
        {
            Q_Q( const FormFieldButton );
            return qVariantFromValue<bool>( q->state() ).toString();
        }
};


FormFieldButton::FormFieldButton()
    : FormField( *new FormFieldButtonPrivate() )
{
}

FormFieldButton::~FormFieldButton()
{
}

void FormFieldButton::setState( bool )
{
}


class Okular::FormFieldTextPrivate : public Okular::FormFieldPrivate
{
    public:
        FormFieldTextPrivate()
            : FormFieldPrivate( FormField::FormText )
        {
        }

        Q_DECLARE_PUBLIC( FormFieldText )

        void setValue( const QString& v )
        {
            Q_Q( FormFieldText );
            q->setText( v );
        }

        QString value() const
        {
            Q_Q( const FormFieldText );
            return q->text();
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

        Q_DECLARE_PUBLIC( FormFieldChoice )

        void setValue( const QString& v )
        {
            Q_Q( FormFieldChoice );
            QStringList choices = v.split( ';', QString::SkipEmptyParts );
            QList<int> newchoices;
            foreach ( const QString& str, choices )
            {
                bool ok = true;
                int val = str.toInt( &ok );
                if ( ok )
                    newchoices.append( val );
            }
            if ( !newchoices.isEmpty() )
                q->setCurrentChoices( newchoices );
        }

        QString value() const
        {
            Q_Q( const FormFieldChoice );
            QList<int> choices = q->currentChoices();
            qSort( choices );
            QStringList list;
            foreach ( int c, choices )
            {
                list.append( QString::number( c ) );
            }
            return list.join( QLatin1String( ";" ) );
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

