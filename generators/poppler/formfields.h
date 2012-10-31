/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_PDF_FORMFIELDS_H_
#define _OKULAR_GENERATOR_PDF_FORMFIELDS_H_

#include <poppler-form.h>
#include "core/form.h"

class PopplerFormFieldButton : public Okular::FormFieldButton
{
    public:
        PopplerFormFieldButton( Poppler::FormFieldButton * field );
        virtual ~PopplerFormFieldButton();

        // inherited from Okular::FormField
        virtual Okular::NormalizedRect rect() const;
        virtual int id() const;
        virtual QString name() const;
        virtual QString uiName() const;
        virtual bool isReadOnly() const;
        virtual bool isVisible() const;

        // inherited from Okular::FormFieldButton
        virtual ButtonType buttonType() const;
        virtual QString caption() const;
        virtual bool state() const;
        virtual void setState( bool state );
        virtual QList< int > siblings() const;

    private:
        Poppler::FormFieldButton * m_field;
        Okular::NormalizedRect m_rect;

};

class PopplerFormFieldText : public Okular::FormFieldText
{
    public:
        PopplerFormFieldText( Poppler::FormFieldText * field );
        virtual ~PopplerFormFieldText();

        // inherited from Okular::FormField
        virtual Okular::NormalizedRect rect() const;
        virtual int id() const;
        virtual QString name() const;
        virtual QString uiName() const;
        virtual bool isReadOnly() const;
        virtual bool isVisible() const;

        // inherited from Okular::FormFieldText
        virtual Okular::FormFieldText::TextType textType() const;
        virtual QString text() const;
        virtual void setText( const QString& text );
        virtual bool isPassword() const;
        virtual bool isRichText() const;
        virtual int maximumLength() const;
        virtual Qt::Alignment textAlignment() const;
        virtual bool canBeSpellChecked() const;

    private:
        Poppler::FormFieldText * m_field;
        Okular::NormalizedRect m_rect;

};

class PopplerFormFieldChoice : public Okular::FormFieldChoice
{
    public:
        PopplerFormFieldChoice( Poppler::FormFieldChoice * field );
        virtual ~PopplerFormFieldChoice();

        // inherited from Okular::FormField
        virtual Okular::NormalizedRect rect() const;
        virtual int id() const;
        virtual QString name() const;
        virtual QString uiName() const;
        virtual bool isReadOnly() const;
        virtual bool isVisible() const;

        // inherited from Okular::FormFieldChoice
        virtual ChoiceType choiceType() const;
        virtual QStringList choices() const;
        virtual bool isEditable() const;
        virtual bool multiSelect() const;
        virtual QList<int> currentChoices() const;
        virtual void setCurrentChoices( const QList<int>& choices );
        virtual QString editChoice() const;
        virtual void setEditChoice( const QString& text );
        virtual Qt::Alignment textAlignment() const;
        virtual bool canBeSpellChecked() const;

    private:
        Poppler::FormFieldChoice * m_field;
        Okular::NormalizedRect m_rect;

};

#endif
