/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2018 by Intevation GmbH <intevation@intevation.de>      *
 *   Copyright (C) 2019 by Oliver Sander <oliver.sander@tu-dresden.de>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "formfields.h"

#include "core/action.h"

#include "pdfsignatureutils.h"

#include <poppler-qt5.h>

#include <config-okular-poppler.h>

extern Okular::Action *createLinkFromPopplerLink(const Poppler::Link *popplerLink, bool deletePopplerLink = true);
#ifdef HAVE_POPPLER_0_65
#define SET_ANNOT_ACTIONS                                                                                                                                                                                                                      \
    setAdditionalAction(Okular::Annotation::CursorEntering, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::CursorEnteringAction)));                                                                                  \
    setAdditionalAction(Okular::Annotation::CursorLeaving, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::CursorLeavingAction)));                                                                                    \
    setAdditionalAction(Okular::Annotation::MousePressed, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::MousePressedAction)));                                                                                      \
    setAdditionalAction(Okular::Annotation::MouseReleased, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::MouseReleasedAction)));                                                                                    \
    setAdditionalAction(Okular::Annotation::FocusIn, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::FocusInAction)));                                                                                                \
    setAdditionalAction(Okular::Annotation::FocusOut, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::FocusOutAction)));
#else
#define SET_ANNOT_ACTIONS
#endif

#define SET_ACTIONS                                                                                                                                                                                                                            \
    setActivationAction(createLinkFromPopplerLink(m_field->activationAction()));                                                                                                                                                               \
    setAdditionalAction(Okular::FormField::FieldModified, createLinkFromPopplerLink(m_field->additionalAction(Poppler::FormField::FieldModified)));                                                                                            \
    setAdditionalAction(Okular::FormField::FormatField, createLinkFromPopplerLink(m_field->additionalAction(Poppler::FormField::FormatField)));                                                                                                \
    setAdditionalAction(Okular::FormField::ValidateField, createLinkFromPopplerLink(m_field->additionalAction(Poppler::FormField::ValidateField)));                                                                                            \
    setAdditionalAction(Okular::FormField::CalculateField, createLinkFromPopplerLink(m_field->additionalAction(Poppler::FormField::CalculateField)));                                                                                          \
    SET_ANNOT_ACTIONS

PopplerFormFieldButton::PopplerFormFieldButton(std::unique_ptr<Poppler::FormFieldButton> field)
    : Okular::FormFieldButton()
    , m_field(std::move(field))
{
    m_rect = Okular::NormalizedRect::fromQRectF(m_field->rect());
    m_id = m_field->id();
    SET_ACTIONS
}

Okular::NormalizedRect PopplerFormFieldButton::rect() const
{
    return m_rect;
}

int PopplerFormFieldButton::id() const
{
    return m_id;
}

QString PopplerFormFieldButton::name() const
{
    return m_field->name();
}

QString PopplerFormFieldButton::uiName() const
{
    return m_field->uiName();
}

QString PopplerFormFieldButton::fullyQualifiedName() const
{
    return m_field->fullyQualifiedName();
}

bool PopplerFormFieldButton::isReadOnly() const
{
    return m_field->isReadOnly();
}

void PopplerFormFieldButton::setReadOnly(bool value)
{
#ifdef HAVE_POPPLER_0_64
    m_field->setReadOnly(value);
#else
    Q_UNUSED(value);
#endif
}

bool PopplerFormFieldButton::isVisible() const
{
    return m_field->isVisible();
}

void PopplerFormFieldButton::setVisible(bool value)
{
#ifdef HAVE_POPPLER_0_64
    m_field->setVisible(value);
#else
    Q_UNUSED(value);
#endif
}

bool PopplerFormFieldButton::isPrintable() const
{
#ifdef HAVE_POPPLER_0_79
    return m_field->isPrintable();
#else
    return true;
#endif
}

void PopplerFormFieldButton::setPrintable(bool value)
{
#ifdef HAVE_POPPLER_0_79
    m_field->setPrintable(value);
#else
    Q_UNUSED(value);
#endif
}

Okular::FormFieldButton::ButtonType PopplerFormFieldButton::buttonType() const
{
    switch (m_field->buttonType()) {
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

void PopplerFormFieldButton::setState(bool state)
{
    m_field->setState(state);
}

QList<int> PopplerFormFieldButton::siblings() const
{
    return m_field->siblings();
}

#ifdef HAVE_POPPLER_0_79
Poppler::FormFieldIcon PopplerFormFieldButton::icon() const
{
    return m_field->icon();
}
#endif

void PopplerFormFieldButton::setIcon(Okular::FormField *field)
{
#ifdef HAVE_POPPLER_0_79
    if (field->type() == Okular::FormField::FormButton) {
        PopplerFormFieldButton *button = static_cast<PopplerFormFieldButton *>(field);
        m_field->setIcon(button->icon());
    }
#else
    Q_UNUSED(field);
#endif
}

PopplerFormFieldText::PopplerFormFieldText(std::unique_ptr<Poppler::FormFieldText> field)
    : Okular::FormFieldText()
    , m_field(std::move(field))
{
    m_rect = Okular::NormalizedRect::fromQRectF(m_field->rect());
    m_id = m_field->id();
    SET_ACTIONS
}

Okular::NormalizedRect PopplerFormFieldText::rect() const
{
    return m_rect;
}

int PopplerFormFieldText::id() const
{
    return m_id;
}

QString PopplerFormFieldText::name() const
{
    return m_field->name();
}

QString PopplerFormFieldText::uiName() const
{
    return m_field->uiName();
}

QString PopplerFormFieldText::fullyQualifiedName() const
{
    return m_field->fullyQualifiedName();
}

bool PopplerFormFieldText::isReadOnly() const
{
    return m_field->isReadOnly();
}

void PopplerFormFieldText::setReadOnly(bool value)
{
#ifdef HAVE_POPPLER_0_64
    m_field->setReadOnly(value);
#else
    Q_UNUSED(value);
#endif
}

bool PopplerFormFieldText::isVisible() const
{
    return m_field->isVisible();
}

void PopplerFormFieldText::setVisible(bool value)
{
#ifdef HAVE_POPPLER_0_64
    m_field->setVisible(value);
#else
    Q_UNUSED(value);
#endif
}

bool PopplerFormFieldText::isPrintable() const
{
#ifdef HAVE_POPPLER_0_79
    return m_field->isPrintable();
#else
    return true;
#endif
}

void PopplerFormFieldText::setPrintable(bool value)
{
#ifdef HAVE_POPPLER_0_79
    m_field->setPrintable(value);
#else
    Q_UNUSED(value);
#endif
}

Okular::FormFieldText::TextType PopplerFormFieldText::textType() const
{
    switch (m_field->textType()) {
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

void PopplerFormFieldText::setText(const QString &text)
{
    m_field->setText(text);
}

void PopplerFormFieldText::setAppearanceText(const QString &text)
{
#ifdef HAVE_POPPLER_0_80
    m_field->setAppearanceText(text);
#else
    Q_UNUSED(text);
#endif
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

PopplerFormFieldChoice::PopplerFormFieldChoice(std::unique_ptr<Poppler::FormFieldChoice> field)
    : Okular::FormFieldChoice()
    , m_field(std::move(field))
{
    m_rect = Okular::NormalizedRect::fromQRectF(m_field->rect());
    m_id = m_field->id();
    SET_ACTIONS

#ifdef HAVE_POPPLER_0_87
    QMap<QString, QString> values;
    const auto fieldChoicesWithExportValues = m_field->choicesWithExportValues();
    for (const QPair<QString, QString> &value : fieldChoicesWithExportValues) {
        values.insert(value.first, value.second);
    }
    setExportValues(values);
#endif
}

Okular::NormalizedRect PopplerFormFieldChoice::rect() const
{
    return m_rect;
}

int PopplerFormFieldChoice::id() const
{
    return m_id;
}

QString PopplerFormFieldChoice::name() const
{
    return m_field->name();
}

QString PopplerFormFieldChoice::uiName() const
{
    return m_field->uiName();
}

QString PopplerFormFieldChoice::fullyQualifiedName() const
{
    return m_field->fullyQualifiedName();
}

bool PopplerFormFieldChoice::isReadOnly() const
{
    return m_field->isReadOnly();
}

void PopplerFormFieldChoice::setReadOnly(bool value)
{
#ifdef HAVE_POPPLER_0_64
    m_field->setReadOnly(value);
#else
    Q_UNUSED(value);
#endif
}

bool PopplerFormFieldChoice::isVisible() const
{
    return m_field->isVisible();
}

void PopplerFormFieldChoice::setVisible(bool value)
{
#ifdef HAVE_POPPLER_0_64
    m_field->setVisible(value);
#else
    Q_UNUSED(value);
#endif
}

bool PopplerFormFieldChoice::isPrintable() const
{
#ifdef HAVE_POPPLER_0_79
    return m_field->isPrintable();
#else
    return true;
#endif
}

void PopplerFormFieldChoice::setPrintable(bool value)
{
#ifdef HAVE_POPPLER_0_79
    m_field->setPrintable(value);
#else
    Q_UNUSED(value);
#endif
}

Okular::FormFieldChoice::ChoiceType PopplerFormFieldChoice::choiceType() const
{
    switch (m_field->choiceType()) {
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

void PopplerFormFieldChoice::setCurrentChoices(const QList<int> &choices)
{
    m_field->setCurrentChoices(choices);
}

QString PopplerFormFieldChoice::editChoice() const
{
    return m_field->editChoice();
}

void PopplerFormFieldChoice::setEditChoice(const QString &text)
{
    m_field->setEditChoice(text);
}

Qt::Alignment PopplerFormFieldChoice::textAlignment() const
{
    return Qt::AlignTop | m_field->textAlignment();
}

bool PopplerFormFieldChoice::canBeSpellChecked() const
{
    return m_field->canBeSpellChecked();
}

PopplerFormFieldSignature::PopplerFormFieldSignature(std::unique_ptr<Poppler::FormFieldSignature> field)
    : Okular::FormFieldSignature()
    , m_field(std::move(field))
{
    m_rect = Okular::NormalizedRect::fromQRectF(m_field->rect());
    m_id = m_field->id();
    m_info = new PopplerSignatureInfo(m_field->validate(Poppler::FormFieldSignature::ValidateVerifyCertificate));
    SET_ACTIONS
}

PopplerFormFieldSignature::~PopplerFormFieldSignature()
{
    delete m_info;
}

Okular::NormalizedRect PopplerFormFieldSignature::rect() const
{
    return m_rect;
}

int PopplerFormFieldSignature::id() const
{
    return m_id;
}

QString PopplerFormFieldSignature::name() const
{
    return m_field->name();
}

QString PopplerFormFieldSignature::uiName() const
{
    return m_field->uiName();
}

QString PopplerFormFieldSignature::fullyQualifiedName() const
{
    return m_field->fullyQualifiedName();
}

bool PopplerFormFieldSignature::isReadOnly() const
{
    return m_field->isReadOnly();
}

bool PopplerFormFieldSignature::isVisible() const
{
    return m_field->isVisible();
}

PopplerFormFieldSignature::SignatureType PopplerFormFieldSignature::signatureType() const
{
    switch (m_field->signatureType()) {
    case Poppler::FormFieldSignature::AdbePkcs7sha1:
        return Okular::FormFieldSignature::AdbePkcs7sha1;
    case Poppler::FormFieldSignature::AdbePkcs7detached:
        return Okular::FormFieldSignature::AdbePkcs7detached;
    case Poppler::FormFieldSignature::EtsiCAdESdetached:
        return Okular::FormFieldSignature::EtsiCAdESdetached;
    default:
        return Okular::FormFieldSignature::UnknownType;
    }
}

const Okular::SignatureInfo &PopplerFormFieldSignature::signatureInfo() const
{
    return *m_info;
}
