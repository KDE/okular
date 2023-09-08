/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "js_field_p.h"

#include <QDebug>
#include <QHash>
#include <QJSEngine>
#include <QTimer>

#include "../debug_p.h"
#include "../document_p.h"
#include "../form.h"
#include "../page.h"
#include "../page_p.h"
#include "js_display_p.h"

using namespace Okular;

#define OKULAR_NAME QStringLiteral("okular_name")

typedef QHash<FormField *, Page *> FormCache;
Q_GLOBAL_STATIC(FormCache, g_fieldCache)
typedef QHash<QString, FormField *> ButtonCache;
Q_GLOBAL_STATIC(ButtonCache, g_buttonCache)

// Helper for modified fields
static void updateField(FormField *field)
{
    Page *page = g_fieldCache->value(field);
    if (page) {
        Document *doc = PagePrivate::get(page)->m_doc->m_parent;
        const int pageNumber = page->number();
        QTimer::singleShot(0, doc, [doc, pageNumber] { doc->refreshPixmaps(pageNumber); });
        Q_EMIT doc->refreshFormWidget(field);
    } else {
        qWarning() << "Could not get page of field" << field;
    }
}

// Field.doc
QJSValue JSField::doc() const
{
    return qjsEngine(this)->globalObject();
}

// Field.name
QString JSField::name() const
{
    return m_field->fullyQualifiedName();
}

// Field.readonly (getter)
bool JSField::readonly() const
{
    return m_field->isReadOnly();
}

// Field.readonly (setter)
void JSField::setReadonly(bool readonly)
{
    m_field->setReadOnly(readonly);

    updateField(m_field);
}

static QString fieldGetTypeHelper(const FormField *field)
{
    switch (field->type()) {
    case FormField::FormButton: {
        const FormFieldButton *button = static_cast<const FormFieldButton *>(field);
        switch (button->buttonType()) {
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
    case FormField::FormChoice: {
        const FormFieldChoice *choice = static_cast<const FormFieldChoice *>(field);
        switch (choice->choiceType()) {
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
QString JSField::type() const
{
    return fieldGetTypeHelper(m_field);
}

QJSValue JSField::fieldGetValueCore(bool asString) const
{
    QJSValue result(QJSValue::UndefinedValue);

    switch (m_field->type()) {
    case FormField::FormButton: {
        const FormFieldButton *button = static_cast<const FormFieldButton *>(m_field);
        if (button->state()) {
            result = QStringLiteral("Yes");
        } else {
            result = QStringLiteral("Off");
        }
        break;
    }
    case FormField::FormText: {
        const FormFieldText *text = static_cast<const FormFieldText *>(m_field);
        const QLocale locale;
        bool ok;
        const double textAsNumber = locale.toDouble(text->text(), &ok);
        if (ok && !asString) {
            result = textAsNumber;
        } else {
            result = text->text();
        }
        break;
    }
    case FormField::FormChoice: {
        const FormFieldChoice *choice = static_cast<const FormFieldChoice *>(m_field);
        const QList<int> currentChoices = choice->currentChoices();
        if (currentChoices.count() == 1) {
            result = choice->exportValueForChoice(choice->choices().at(currentChoices[0]));
        }
        break;
    }
    case FormField::FormSignature: {
        break;
    }
    }

    qCDebug(OkularCoreDebug) << "fieldGetValueCore:"
                             << " Field: " << m_field->fullyQualifiedName() << " Type: " << fieldGetTypeHelper(m_field) << " Value: " << result.toString() << (result.isString() ? "(as string)" : "");
    return result;
}
// Field.value (getter)
QJSValue JSField::value() const
{
    return fieldGetValueCore(/*asString*/ false);
}

// Field.value (setter)
void JSField::setValue(const QJSValue &value)
{
    qCDebug(OkularCoreDebug) << "fieldSetValue: Field: " << m_field->fullyQualifiedName() << " Type: " << fieldGetTypeHelper(m_field) << " Value: " << value.toString();
    switch (m_field->type()) {
    case FormField::FormButton: {
        FormFieldButton *button = static_cast<FormFieldButton *>(m_field);
        const QString text = value.toString();
        if (text == QStringLiteral("Yes")) {
            button->setState(true);
            updateField(m_field);
        } else if (text == QStringLiteral("Off")) {
            button->setState(false);
            updateField(m_field);
        }
        break;
    }
    case FormField::FormText: {
        FormFieldText *textField = static_cast<FormFieldText *>(m_field);
        const QString text = value.toString();
        if (text != textField->text()) {
            textField->setText(text);
            updateField(m_field);
        }
        break;
    }
    case FormField::FormChoice: {
        FormFieldChoice *choice = static_cast<FormFieldChoice *>(m_field);
        Q_UNUSED(choice); // ###
        break;
    }
    case FormField::FormSignature: {
        break;
    }
    }
}

// Field.valueAsString (getter)
QJSValue JSField::valueAsString() const
{
    return fieldGetValueCore(/*asString*/ true);
}

// Field.hidden (getter)
bool JSField::hidden() const
{
    return !m_field->isVisible();
}

// Field.hidden (setter)
void JSField::setHidden(bool hidden)
{
    m_field->setVisible(!hidden);

    updateField(m_field);
}

// Field.display (getter)
int JSField::display() const
{
    bool visible = m_field->isVisible();
    if (visible) {
        return m_field->isPrintable() ? FormDisplay::FormVisible : FormDisplay::FormNoPrint;
    }
    return m_field->isPrintable() ? FormDisplay::FormNoView : FormDisplay::FormHidden;
}

// Field.display (setter)
void JSField::setDisplay(int display)
{
    switch (display) {
    case FormDisplay::FormVisible:
        m_field->setVisible(true);
        m_field->setPrintable(true);
        break;
    case FormDisplay::FormHidden:
        m_field->setVisible(false);
        m_field->setPrintable(false);
        break;
    case FormDisplay::FormNoPrint:
        m_field->setVisible(true);
        m_field->setPrintable(false);
        break;
    case FormDisplay::FormNoView:
        m_field->setVisible(false);
        m_field->setPrintable(true);
        break;
    }
    updateField(m_field);
}

//  Instead of getting the Icon, we pick the field.
QJSValue JSField::buttonGetIcon([[maybe_unused]] int nFace) const
{
    QJSValue fieldObject = qjsEngine(this)->newObject();
    fieldObject.setProperty(OKULAR_NAME, m_field->fullyQualifiedName());
    g_buttonCache->insert(m_field->fullyQualifiedName(), m_field);

    return fieldObject;
}

/*
 * Now we send to the button what Icon should be drawn on it
 */
void JSField::buttonSetIcon(const QJSValue &oIcon, [[maybe_unused]] int nFace)
{
    const QString fieldName = oIcon.property(OKULAR_NAME).toString();

    if (m_field->type() == Okular::FormField::FormButton) {
        FormFieldButton *button = static_cast<FormFieldButton *>(m_field);
        const auto formField = g_buttonCache->value(fieldName);
        if (formField) {
            button->setIcon(formField);
        }
    }

    updateField(m_field);
}

JSField::JSField(FormField *field, QObject *parent)
    : QObject(parent)
    , m_field(field)
{
}

JSField::~JSField() = default;

QJSValue JSField::wrapField(QJSEngine *engine, FormField *field, Page *page)
{
    // ### cache unique wrapper
    QJSValue f = engine->newQObject(new JSField(field));
    f.setProperty(QStringLiteral("page"), page->number());
    g_fieldCache->insert(field, page);
    return f;
}

void JSField::clearCachedFields()
{
    if (g_fieldCache.exists()) {
        g_fieldCache->clear();
    }

    if (g_buttonCache.exists()) {
        g_buttonCache->clear();
    }
}
