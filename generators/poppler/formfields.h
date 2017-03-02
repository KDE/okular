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
        Okular::NormalizedRect rect() const Q_DECL_OVERRIDE;
        int id() const Q_DECL_OVERRIDE;
        QString name() const Q_DECL_OVERRIDE;
        QString uiName() const Q_DECL_OVERRIDE;
        virtual bool isReadOnly() const Q_DECL_OVERRIDE;
        virtual bool isVisible() const Q_DECL_OVERRIDE;

        // inherited from Okular::FormFieldButton
        virtual ButtonType buttonType() const Q_DECL_OVERRIDE;
        virtual QString caption() const Q_DECL_OVERRIDE;
        virtual bool state() const Q_DECL_OVERRIDE;
        virtual void setState( bool state ) Q_DECL_OVERRIDE;
        virtual QList< int > siblings() const Q_DECL_OVERRIDE;

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
        Okular::NormalizedRect rect() const override;
        int id() const override;
        QString name() const override;
        QString uiName() const override;
        bool isReadOnly() const override;
        bool isVisible() const override;

        // inherited from Okular::FormFieldText
        Okular::FormFieldText::TextType textType() const override;
        QString text() const override;
        void setText( const QString& text ) override;
        bool isPassword() const override;
        bool isRichText() const override;
        int maximumLength() const override;
        Qt::Alignment textAlignment() const override;
        bool canBeSpellChecked() const override;

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
        Okular::NormalizedRect rect() const override;
        int id() const override;
        QString name() const override;
        QString uiName() const override;
        bool isReadOnly() const override;
        bool isVisible() const override;

        // inherited from Okular::FormFieldChoice
        ChoiceType choiceType() const override;
        QStringList choices() const override;
        bool isEditable() const override;
        bool multiSelect() const override;
        QList<int> currentChoices() const override;
        void setCurrentChoices( const QList<int>& choices ) override;
        QString editChoice() const override;
        void setEditChoice( const QString& text ) override;
        Qt::Alignment textAlignment() const override;
        bool canBeSpellChecked() const override;

    private:
        Poppler::FormFieldChoice * m_field;
        Okular::NormalizedRect m_rect;

};

#endif
