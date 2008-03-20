/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_FORM_H_
#define _OKULAR_FORM_H_

#include <okular/core/okular_export.h>
#include <okular/core/area.h>

#include <QtCore/QStringList>

namespace Okular {

class Action;
class Page;
class PagePrivate;
class FormFieldPrivate;
class FormFieldButtonPrivate;
class FormFieldTextPrivate;
class FormFieldChoicePrivate;

/**
 * @short The base interface of a form field.
 *
 * This is the very basic interface to represent a field in a form.
 *
 * This is not meant to be used as a direct base for the form fields in a
 * document, but its abstract subclasses are.
 */
class OKULAR_EXPORT FormField
{
    /// @cond PRIVATE
    friend class Page;
    friend class PagePrivate;
    /// @endcond

    public:
        /**
         * The types of form field.
         */
        enum FieldType
        {
            FormButton,      ///< A "button". See @ref FormFieldButton::ButtonType.
            FormText,        ///< A field of variable text. See @ref FormFieldText::TextType.
            FormChoice,      ///< A choice field. See @ref FormFieldChoice::ChoiceType.
            FormSignature    ///< A signature.
        };

        virtual ~FormField();

        /**
         * The type of the field.
         */
        FieldType type() const;

        /**
         * The bouding rect of the field, in normalized coordinates.
         */
        virtual NormalizedRect rect() const = 0;

        /**
         * The ID of the field.
         */
        virtual int id() const = 0;

        /**
         * The internal name of the field, to be used when referring to the
         * field in eg scripts.
         */
        virtual QString name() const = 0;

        /**
         * The visible name of the field, to be used in the user interface
         * (eg in error messages, etc).
         */
        virtual QString uiName() const = 0;

        /**
         * Whether the field is read-only.
         */
        virtual bool isReadOnly() const;

        /**
         * Whether this form field is visible.
         */
        virtual bool isVisible() const;

        Action* activationAction() const;

    protected:
        /// @cond PRIVATE
        FormField( FormFieldPrivate &dd );
        Q_DECLARE_PRIVATE( FormField )
        FormFieldPrivate *d_ptr;
        /// @endcond

        void setActivationAction( Action *action );

    private:
        Q_DISABLE_COPY( FormField )
};


/**
 * @short Interface of a button form field.
 *
 * This is the base interface to reimplement to represent a button field, like
 * a push button, a check box or a radio button.
 *
 * @since 0.7 (KDE 4.1)
 */
class OKULAR_EXPORT FormFieldButton : public FormField
{
    public:
        /**
         * The types of button field.
         */
        enum ButtonType
        {
            Push,          ///< A simple push button.
            CheckBox,      ///< A check box.
            Radio          ///< A radio button.
        };

        virtual ~FormFieldButton();

        /**
          The particular type of the button field.
         */
        virtual ButtonType buttonType() const = 0;

        /**
         * The caption to be used for the button.
         */
        virtual QString caption() const = 0;

        /**
         * The state of the button.
         */
        virtual bool state() const = 0;

        /**
         * Sets the state of the button to the new \p state .
         */
        virtual void setState( bool state );

        /**
         * The list with the IDs of siblings (ie, buttons belonging to the same
         * group as the current one.
         *
         * Valid only for \ref Radio buttons, an empty list otherwise.
         */
        virtual QList< int > siblings() const = 0;

    protected:
        FormFieldButton();

    private:
        Q_DECLARE_PRIVATE( FormFieldButton )
        Q_DISABLE_COPY( FormFieldButton )
};


/**
 * @short Interface of a text form field.
 *
 * This is the base interface to reimplement to represent a text field, ie a
 * field where the user insert text.
 */
class OKULAR_EXPORT FormFieldText : public FormField
{
    public:
        /**
         * The types of text field.
         */
        enum TextType
        {
            Normal,        ///< A simple singleline text field.
            Multiline,     ///< A multiline text field.
            FileSelect     ///< An input field to select the path of a file on disk.
        };

        virtual ~FormFieldText();

        /**
         * The particular type of the text field.
         */
        virtual TextType textType() const = 0;

        /**
         * The text of text field.
         */
        virtual QString text() const = 0;

        /**
         * Sets the new @p text in the text field.
         *
         * The default implementation does nothing.
         *
         * Reimplemented only if the setting of new text is supported.
         */
        virtual void setText( const QString& text );

        /**
         * Whether this text field is a password input, eg its text @b must be
         * replaced with asterisks.
         *
         * Always false for @ref FileSelect text fields.
         */
        virtual bool isPassword() const;

        /**
         * Whether this text field should allow rich text.
         */
        virtual bool isRichText() const;

        /**
         * The maximum length allowed for the text of text field, or -1 if
         * there is no limitation for the text.
         */
        virtual int maximumLength() const;

        /**
         * The alignment of the text within the field.
         */
        virtual Qt::Alignment textAlignment() const;

        /**
         * Whether the text inserted manually in the field (where possible)
         * can be spell-checked.
         *
         * @note meaningful only if the field is editable.
         */
        virtual bool canBeSpellChecked() const;

    protected:
        FormFieldText();

    private:
        Q_DECLARE_PRIVATE( FormFieldText )
        Q_DISABLE_COPY( FormFieldText )
};


/**
 * @short Interface of a choice form field.
 *
 * This is the base interface to reimplement to represent a choice field, ie a
 * field where the user can select one (of more) element(s) among a set of
 * choices.
 */
class OKULAR_EXPORT FormFieldChoice : public FormField
{
    public:
        /**
         * The types of choice field.
         */
        enum ChoiceType
        {
            ComboBox,     ///< A combo box choice field.
            ListBox       ///< A list box choice field.
        };

        virtual ~FormFieldChoice();

        /**
         * The particular type of the choice field.
         */
        virtual ChoiceType choiceType() const = 0;

        /**
         * The possible choices of the choice field.
         */
        virtual QStringList choices() const = 0;

        /**
         * Whether this ComboBox is editable, ie the user can type in a custom
         * value.
         *
         * Always false for the other types of choices.
         */
        virtual bool isEditable() const;

        /**
         * Whether more than one choice of this ListBox can be selected at the
         * same time.
         *
         * Always false for the other types of choices.
         */
        virtual bool multiSelect() const;

        /**
         * The currently selected choices.
         *
         * Always one element in the list in case of single choice elements.
         */
        virtual QList< int > currentChoices() const = 0;

        /**
         * Sets the selected choices to @p choices .
         */
        virtual void setCurrentChoices( const QList< int >& choices );

        /**
         * The alignment of the text within the field.
         */
        virtual Qt::Alignment textAlignment() const;

        /**
         * Whether the text inserted manually in the field (where possible)
         * can be spell-checked.
         *
         * @note meaningful only if the field is editable.
         */
        virtual bool canBeSpellChecked() const;

    protected:
        FormFieldChoice();

    private:
        Q_DECLARE_PRIVATE( FormFieldChoice )
        Q_DISABLE_COPY( FormFieldChoice )
};

}

#endif
