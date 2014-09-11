/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kjs_field_p.h"

#include <kjs/kjsinterpreter.h>
#include <kjs/kjsprototype.h>
#include <kjs/kjsarguments.h>

#include <qhash.h>

#include <kdebug.h>
#include <kglobal.h>

#include "../debug_p.h"
#include "../document_p.h"
#include "../form.h"
#include "../page.h"

using namespace Okular;

static KJSPrototype *g_fieldProto;

typedef QHash< FormField *, KJSObject > FormCache;
K_GLOBAL_STATIC( FormCache, g_fieldCache )

// Field.doc
static KJSObject fieldGetDoc( KJSContext *context, void *  )
{
    return context->interpreter().globalObject();
}

// Field.name
static KJSObject fieldGetName( KJSContext *, void *object  )
{
    const FormField *field = reinterpret_cast< FormField * >( object );
    return KJSString( field->name() );
}

// Field.readonly (getter)
static KJSObject fieldGetReadOnly( KJSContext *, void *object )
{
    const FormField *field = reinterpret_cast< FormField * >( object );
    return KJSBoolean( field->isReadOnly() );
}

// Field.readonly (setter)
static void fieldSetReadOnly( KJSContext *context, void *object, KJSObject value )
{
#if 0
    FormField *field = reinterpret_cast< FormField * >( object );
    bool b = value.toBoolean( context );
    field->setReadOnly( b );
#else
    Q_UNUSED( context );
    Q_UNUSED( object );
    Q_UNUSED( value );
    kDebug(OkularDebug) << "Not implemented: setting readonly property";
#endif
}

static QString fieldGetTypeHelper( const FormField *field )
{
    switch ( field->type() )
    {
        case FormField::FormButton:
        {
            const FormFieldButton *button = static_cast< const FormFieldButton * >( field );
            switch ( button->buttonType() )
            {
                case FormFieldButton::Push:
                    return "button";
                case FormFieldButton::CheckBox:
                    return "checkbox";
                case FormFieldButton::Radio:
                    return "radiobutton";
            }
            break;
        }
        case FormField::FormText:
            return "text";
        case FormField::FormChoice:
        {
            const FormFieldChoice *choice = static_cast< const FormFieldChoice * >( field );
            switch ( choice->choiceType() )
            {
                case FormFieldChoice::ComboBox:
                    return "combobox";
                case FormFieldChoice::ListBox:
                    return "listbox";
            }
            break;
        }
        case FormField::FormSignature:
            return "signature";
    }
    return QString();
}

// Field.type
static KJSObject fieldGetType( KJSContext *, void *object  )
{
    const FormField *field = reinterpret_cast< FormField * >( object );

    return KJSString( fieldGetTypeHelper( field ) );
}

// Field.value (getter)
static KJSObject fieldGetValue( KJSContext *context, void *object )
{
    FormField *field = reinterpret_cast< FormField * >( object );
    if ( field->isReadOnly() )
    {
        KJSObject value = g_fieldCache->value( field );
        if ( g_fieldCache.exists() && g_fieldCache->contains( field ) )
            value = g_fieldCache->value( field );
        else
            value = KJSString("");
        kDebug(OkularDebug) << "Getting the value of a readonly field" << field->name() << ":" << value.toString( context );
        return value;
    }

    switch ( field->type() )
    {
        case FormField::FormButton:
        {
            const FormFieldButton *button = static_cast< const FormFieldButton * >( field );
            Q_UNUSED( button ); // ###
            break;
        }
        case FormField::FormText:
        {
            const FormFieldText *text = static_cast< const FormFieldText * >( field );
            return KJSString( text->text() );
        }
        case FormField::FormChoice:
        {
            const FormFieldChoice *choice = static_cast< const FormFieldChoice * >( field );
            Q_UNUSED( choice ); // ###
            break;
        }
        case FormField::FormSignature:
        {
            break;
        }
    }

    return KJSUndefined();
}

// Field.value (setter)
static void fieldSetValue( KJSContext *context, void *object, KJSObject value )
{
    FormField *field = reinterpret_cast< FormField * >( object );

    if ( field->isReadOnly() )
    {
        // ### throw exception?
        kDebug(OkularDebug) << "Trying to change the readonly field" << field->name() << "to" << value.toString( context );
        g_fieldCache->insert( field, value );
        return;
    }

    switch ( field->type() )
    {
        case FormField::FormButton:
        {
            FormFieldButton *button = static_cast< FormFieldButton * >( field );
            Q_UNUSED( button ); // ###
            break;
        }
        case FormField::FormText:
        {
            FormFieldText *text = static_cast< FormFieldText * >( field );
            text->setText( value.toString( context ) );
            break;
        }
        case FormField::FormChoice:
        {
            FormFieldChoice *choice = static_cast< FormFieldChoice * >( field );
            Q_UNUSED( choice ); // ###
            break;
        }
        case FormField::FormSignature:
        {
            break;
        }
    }
}

void JSField::initType( KJSContext *ctx )
{
    static bool initialized = false;
    if ( initialized )
        return;
    initialized = true;

    if ( !g_fieldProto )
        g_fieldProto = new KJSPrototype();

    g_fieldProto->defineProperty( ctx, "doc", fieldGetDoc );
    g_fieldProto->defineProperty( ctx, "name", fieldGetName );
    g_fieldProto->defineProperty( ctx, "readonly",
                                  fieldGetReadOnly, fieldSetReadOnly );
    g_fieldProto->defineProperty( ctx, "type", fieldGetType );
    g_fieldProto->defineProperty( ctx, "value", fieldGetValue, fieldSetValue );
}

KJSObject JSField::wrapField( KJSContext *ctx, FormField *field, Page *page )
{
    // ### cache unique wrapper
    KJSObject f = g_fieldProto->constructObject( ctx, field );
    f.setProperty( ctx, "page", page->number() );
    return f;
}

void JSField::clearCachedFields()
{
    if ( g_fieldCache.exists() )
    {
        g_fieldCache->clear();
    }
}
