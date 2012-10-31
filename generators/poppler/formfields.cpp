/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "formfields.h"

#include "core/action.h"

#include <poppler-qt4.h>

#include <config-okular-poppler.h>

extern Okular::Action* createLinkFromPopplerLink(const Poppler::Link *popplerLink);

PopplerFormFieldButton::PopplerFormFieldButton( Poppler::FormFieldButton * field )
    : Okular::FormFieldButton(), m_field( field )
{
    m_rect = Okular::NormalizedRect::fromQRectF( m_field->rect() );
    Poppler::Link *aAction = field->activationAction();
    if ( aAction )
    {
        setActivationAction( createLinkFromPopplerLink( aAction ) );
    }
}

PopplerFormFieldButton::~PopplerFormFieldButton()
{
    delete m_field;
}

Okular::NormalizedRect PopplerFormFieldButton::rect() const
{
    return m_rect;
}

int PopplerFormFieldButton::id() const
{
    return m_field->id();
}

QString PopplerFormFieldButton::name() const
{
    return m_field->name();
}

QString PopplerFormFieldButton::uiName() const
{
    return m_field->uiName();
}

bool PopplerFormFieldButton::isReadOnly() const
{
    return m_field->isReadOnly();
}

bool PopplerFormFieldButton::isVisible() const
{
    return m_field->isVisible();
}

Okular::FormFieldButton::ButtonType PopplerFormFieldButton::buttonType() const
{
    switch ( m_field->buttonType() )
    {
        case Poppler::FormFieldButton::Push:
            return Okular::FormFieldButton::Push;
        case Poppler::FormFieldButton::CheckBox:
            return Okular::FormFieldButton::CheckBox;
        case Poppler::FormFieldButton::Radio:
            return Okular::FormFieldButton::Radio;
    }
    return Okular::FormFieldButton::Push;
}

QString PopplerFormFieldButton::caption() const
{
    return m_field->caption();
}

bool PopplerFormFieldButton::state() const
{
    return m_field->state();
}

void PopplerFormFieldButton::setState( bool state )
{
    m_field->setState( state );
}

QList< int > PopplerFormFieldButton::siblings() const
{
    return m_field->siblings();
}


PopplerFormFieldText::PopplerFormFieldText( Poppler::FormFieldText * field )
    : Okular::FormFieldText(), m_field( field )
{
    m_rect = Okular::NormalizedRect::fromQRectF( m_field->rect() );
    Poppler::Link *aAction = field->activationAction();
    if ( aAction )
    {
        setActivationAction( createLinkFromPopplerLink( aAction ) );
    }
}

PopplerFormFieldText::~PopplerFormFieldText()
{
    delete m_field;
}

Okular::NormalizedRect PopplerFormFieldText::rect() const
{
    return m_rect;
}

int PopplerFormFieldText::id() const
{
    return m_field->id();
}

QString PopplerFormFieldText::name() const
{
    return m_field->name();
}

QString PopplerFormFieldText::uiName() const
{
    return m_field->uiName();
}

bool PopplerFormFieldText::isReadOnly() const
{
    return m_field->isReadOnly();
}

bool PopplerFormFieldText::isVisible() const
{
    return m_field->isVisible();
}

Okular::FormFieldText::TextType PopplerFormFieldText::textType() const
{
    switch ( m_field->textType() )
    {
        case Poppler::FormFieldText::Normal:
            return Okular::FormFieldText::Normal;
        case Poppler::FormFieldText::Multiline:
            return Okular::FormFieldText::Multiline;
        case Poppler::FormFieldText::FileSelect:
            return Okular::FormFieldText::FileSelect;
    }
    return Okular::FormFieldText::Normal;
}

QString PopplerFormFieldText::text() const
{
    return m_field->text();
}

void PopplerFormFieldText::setText( const QString& text )
{
    m_field->setText( text );
}

bool PopplerFormFieldText::isPassword() const
{
    return m_field->isPassword();
}

bool PopplerFormFieldText::isRichText() const
{
    return m_field->isRichText();
}

int PopplerFormFieldText::maximumLength() const
{
    return m_field->maximumLength();
}

Qt::Alignment PopplerFormFieldText::textAlignment() const
{
    return Qt::AlignTop | m_field->textAlignment();
}

bool PopplerFormFieldText::canBeSpellChecked() const
{
    return m_field->canBeSpellChecked();
}


PopplerFormFieldChoice::PopplerFormFieldChoice( Poppler::FormFieldChoice * field )
    : Okular::FormFieldChoice(), m_field( field )
{
    m_rect = Okular::NormalizedRect::fromQRectF( m_field->rect() );
    Poppler::Link *aAction = field->activationAction();
    if ( aAction )
    {
        setActivationAction( createLinkFromPopplerLink( aAction ) );
    }
}

PopplerFormFieldChoice::~PopplerFormFieldChoice()
{
    delete m_field;
}

Okular::NormalizedRect PopplerFormFieldChoice::rect() const
{
    return m_rect;
}

int PopplerFormFieldChoice::id() const
{
    return m_field->id();
}

QString PopplerFormFieldChoice::name() const
{
    return m_field->name();
}

QString PopplerFormFieldChoice::uiName() const
{
    return m_field->uiName();
}

bool PopplerFormFieldChoice::isReadOnly() const
{
    return m_field->isReadOnly();
}

bool PopplerFormFieldChoice::isVisible() const
{
    return m_field->isVisible();
}

Okular::FormFieldChoice::ChoiceType PopplerFormFieldChoice::choiceType() const
{
    switch ( m_field->choiceType() )
    {
        case Poppler::FormFieldChoice::ComboBox:
            return Okular::FormFieldChoice::ComboBox;
        case Poppler::FormFieldChoice::ListBox:
            return Okular::FormFieldChoice::ListBox;
    }
    return Okular::FormFieldChoice::ListBox;
}

QStringList PopplerFormFieldChoice::choices() const
{
    return m_field->choices();
}

bool PopplerFormFieldChoice::isEditable() const
{
    return m_field->isEditable();
}

bool PopplerFormFieldChoice::multiSelect() const
{
    return m_field->multiSelect();
}

QList<int> PopplerFormFieldChoice::currentChoices() const
{
    return m_field->currentChoices();
}

void PopplerFormFieldChoice::setCurrentChoices( const QList<int>& choices )
{
    m_field->setCurrentChoices( choices );
}

QString PopplerFormFieldChoice::editChoice() const
{
#ifdef HAVE_POPPLER_0_22
    return m_field->editChoice();
#else
    return QString();
#endif
}

void PopplerFormFieldChoice::setEditChoice( const QString& text )
{
#ifdef HAVE_POPPLER_0_22
    m_field->setEditChoice( text );
#endif
}

Qt::Alignment PopplerFormFieldChoice::textAlignment() const
{
    return Qt::AlignTop | m_field->textAlignment();
}

bool PopplerFormFieldChoice::canBeSpellChecked() const
{
    return m_field->canBeSpellChecked();
}


