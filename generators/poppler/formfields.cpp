/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "formfields.h"

PopplerFormFieldText::PopplerFormFieldText( Poppler::FormFieldText * field )
    : Okular::FormFieldText(), m_field( field )
{
    m_rect = Okular::NormalizedRect::fromQRectF( m_field->rect() );
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

Qt::Alignment PopplerFormFieldChoice::textAlignment() const
{
    return Qt::AlignTop | m_field->textAlignment();
}

bool PopplerFormFieldChoice::canBeSpellChecked() const
{
    return m_field->canBeSpellChecked();
}


