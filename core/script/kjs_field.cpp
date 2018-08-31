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

#include <QDebug>

#include "../debug_p.h"
#include "../document_p.h"
#include "../form.h"
#include "../page.h"
#include "../page_p.h"

using namespace Okular;

static KJSPrototype *g_fieldProto;

typedef QHash< FormField *, Page * > FormCache;
Q_GLOBAL_STATIC( FormCache, g_fieldCache )


// Helper for modified fields
static void updateField( FormField *field )
{
    Page *page = g_fieldCache->value( field );
    if (page)
    {
        Document *doc = PagePrivate::get( page )->m_doc->m_parent;
        QMetaObject::invokeMethod( doc, "refreshPixmaps", Qt::QueuedConnection, Q_ARG( int, page->number() ) );
        emit doc->refreshFormWidget( field );
    }
    else
    {
        qWarning() << "Could not get page of field" << field;
    }
}

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
    FormField *field = reinterpret_cast< FormField * >( object );
    bool b = value.toBoolean( context );
    field->setReadOnly( b );

    updateField( field );
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
                    return QStringLiteral("button");
                case FormFieldButton::CheckBox:
                    return QStringLiteral("checkbox");
                case FormFieldButton::Radio:
                    return QStringLiteral("radiobutton");
            }
            break;
        }
        case FormField::FormText:
            return QStringLiteral("text");
        case FormField::FormChoice:
        {
            const FormFieldChoice *choice = static_cast< const FormFieldChoice * >( field );
            switch ( choice->choiceType() )
            {
                case FormFieldChoice::ComboBox:
                    return QStringLiteral("combobox");
                case FormFieldChoice::ListBox:
                    return QStringLiteral("listbox");
            }
            break;
        }
        case FormField::FormSignature:
            return QStringLiteral("signature");
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
static KJSObject fieldGetValue( KJSContext */*context*/, void *object )
{
    FormField *field = reinterpret_cast< FormField * >( object );

    switch ( field->type() )
    {
        case FormField::FormButton:
        {
            const FormFieldButton *button = static_cast< const FormFieldButton * >( field );
            if ( button->state() )
            {
                return KJSString( QStringLiteral( "Yes" ) );
            }
            return KJSString( QStringLiteral( "Off" ) );
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

    switch ( field->type() )
    {
        case FormField::FormButton:
        {
            FormFieldButton *button = static_cast< FormFieldButton * >( field );
            const QString text = value.toString( context );
            if ( text == QStringLiteral( "Yes" ) )
            {
                button->setState( true );
                updateField( field );
            }
            else if ( text == QStringLiteral( "Off" ) )
            {
                button->setState( false );
                updateField( field );
            }
            break;
        }
        case FormField::FormText:
        {
            FormFieldText *textField = static_cast< FormFieldText * >( field );
            const QString text = value.toString( context );
            if ( text != textField->text() )
            {
                textField->setText( text );
                updateField( field );
            }
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

// Field.hidden (getter)
static KJSObject fieldGetHidden( KJSContext *, void *object )
{
    const FormField *field = reinterpret_cast< FormField * >( object );
    return KJSBoolean( !field->isVisible() );
}

// Field.hidden (setter)
static void fieldSetHidden( KJSContext *context, void *object, KJSObject value )
{
    FormField *field = reinterpret_cast< FormField * >( object );
    bool b = value.toBoolean( context );
    field->setVisible( !b );

    updateField( field );
}

void JSField::initType( KJSContext *ctx )
{
    static bool initialized = false;
    if ( initialized )
        return;
    initialized = true;

    if ( !g_fieldProto )
        g_fieldProto = new KJSPrototype();

    g_fieldProto->defineProperty( ctx, QStringLiteral("doc"), fieldGetDoc );
    g_fieldProto->defineProperty( ctx, QStringLiteral("name"), fieldGetName );
    g_fieldProto->defineProperty( ctx, QStringLiteral("readonly"),
                                  fieldGetReadOnly, fieldSetReadOnly );
    g_fieldProto->defineProperty( ctx, QStringLiteral("type"), fieldGetType );
    g_fieldProto->defineProperty( ctx, QStringLiteral("value"), fieldGetValue, fieldSetValue );
    g_fieldProto->defineProperty( ctx, QStringLiteral("hidden"), fieldGetHidden, fieldSetHidden );
}

KJSObject JSField::wrapField( KJSContext *ctx, FormField *field, Page *page )
{
    // ### cache unique wrapper
    KJSObject f = g_fieldProto->constructObject( ctx, field );
    f.setProperty( ctx, QStringLiteral("page"), page->number() );
    g_fieldCache->insert( field, page );
    return f;
}

void JSField::clearCachedFields()
{
    if ( g_fieldCache.exists() )
    {
        g_fieldCache->clear();
    }
}
