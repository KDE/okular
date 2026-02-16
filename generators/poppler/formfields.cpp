/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2018 Intevation GmbH <intevation@intevation.de>
    SPDX-FileCopyrightText: 2019 Oliver Sander <oliver.sander@tu-dresden.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "formfields.h"

#include "core/action.h"

#include "generator_pdf.h"
#include "pdfsettings.h"
#include "pdfsignatureutils.h"

#include "popplerversion.h"
#include <poppler-qt6.h>

extern Okular::Action *createLinkFromPopplerLink(std::variant<const Poppler::Link *, std::unique_ptr<Poppler::Link>> popplerLink);
#define SET_ANNOT_ACTIONS                                                                                                                                                                                                                      \
    setAdditionalAction(Okular::Annotation::CursorEntering, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::CursorEnteringAction)));                                                                                  \
    setAdditionalAction(Okular::Annotation::CursorLeaving, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::CursorLeavingAction)));                                                                                    \
    setAdditionalAction(Okular::Annotation::MousePressed, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::MousePressedAction)));                                                                                      \
    setAdditionalAction(Okular::Annotation::MouseReleased, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::MouseReleasedAction)));                                                                                    \
    setAdditionalAction(Okular::Annotation::FocusIn, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::FocusInAction)));                                                                                                \
    setAdditionalAction(Okular::Annotation::FocusOut, createLinkFromPopplerLink(m_field->additionalAction(Poppler::Annotation::FocusOutAction)));

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
    m_field->setReadOnly(value);
}

bool PopplerFormFieldButton::isVisible() const
{
    return m_field->isVisible();
}

void PopplerFormFieldButton::setVisible(bool value)
{
    m_field->setVisible(value);
}

bool PopplerFormFieldButton::isPrintable() const
{
    return m_field->isPrintable();
}

void PopplerFormFieldButton::setPrintable(bool value)
{
    m_field->setPrintable(value);
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

Poppler::FormFieldIcon PopplerFormFieldButton::icon() const
{
    return m_field->icon();
}

void PopplerFormFieldButton::setIcon(Okular::FormField *field)
{
    if (field->type() == Okular::FormField::FormButton) {
        const PopplerFormFieldButton *button = static_cast<PopplerFormFieldButton *>(field);
        m_field->setIcon(button->icon());
    }
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
    m_field->setReadOnly(value);
}

bool PopplerFormFieldText::isVisible() const
{
    return m_field->isVisible();
}

void PopplerFormFieldText::setVisible(bool value)
{
    m_field->setVisible(value);
}

bool PopplerFormFieldText::isPrintable() const
{
    return m_field->isPrintable();
}

void PopplerFormFieldText::setPrintable(bool value)
{
    m_field->setPrintable(value);
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
    m_field->setAppearanceText(text);
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

    QMap<QString, QString> values;
    const auto fieldChoicesWithExportValues = m_field->choicesWithExportValues();
    for (const QPair<QString, QString> &value : fieldChoicesWithExportValues) {
        values.insert(value.first, value.second);
    }
    setExportValues(values);
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
    m_field->setReadOnly(value);
}

bool PopplerFormFieldChoice::isVisible() const
{
    return m_field->isVisible();
}

void PopplerFormFieldChoice::setVisible(bool value)
{
    m_field->setVisible(value);
}

bool PopplerFormFieldChoice::isPrintable() const
{
    return m_field->isPrintable();
}

void PopplerFormFieldChoice::setPrintable(bool value)
{
    m_field->setPrintable(value);
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
    if (!m_choices) {
        m_choices = m_field->choices();
    }
    return *m_choices;
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

void PopplerFormFieldChoice::setAppearanceChoiceText(const QString &text)
{
    m_field->setAppearanceChoiceText(text);
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
    int validateOptions = Poppler::FormFieldSignature::ValidateVerifyCertificate;
    if (!PDFSettings::checkOCSPServers()) {
        validateOptions = validateOptions | Poppler::FormFieldSignature::ValidateWithoutOCSPRevocationCheck;
    }

    auto result = m_field->validateAsync(static_cast<Poppler::FormFieldSignature::ValidateOptions>(validateOptions));
    m_info = fromPoppler(result.first);
    m_asyncObject = result.second;
    QObject::connect(m_asyncObject.get(), &Poppler::AsyncObject::done, m_asyncObject.get(), [this]() {
        m_info.setCertificateStatus(fromPoppler(m_field->validateResult()));
        for (const auto &[_, callback] : m_updateSubscriptions) {
            callback();
        }
    });
    SET_ACTIONS
}

PopplerFormFieldSignature::~PopplerFormFieldSignature() = default;

static Okular::FormFieldSignature::SubscriptionHandle globalHandle = 0;

Okular::FormFieldSignature::SubscriptionHandle PopplerFormFieldSignature::subscribeUpdates(const std::function<void()> &callback) const
{
    auto handle = (globalHandle++);
    m_updateSubscriptions.emplace(handle, callback);
    return handle;
}

bool PopplerFormFieldSignature::unsubscribeUpdates(const SubscriptionHandle &handle) const
{
    return m_updateSubscriptions.erase(handle) == 1;
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
    case Poppler::FormFieldSignature::UnsignedSignature:
        return Okular::FormFieldSignature::UnsignedSignature;
    case Poppler::FormFieldSignature::UnknownSignatureType:
        return Okular::FormFieldSignature::UnknownType;
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(25, 02, 90)
    case Poppler::FormFieldSignature::G10cPgpSignatureDetached:
        return Okular::FormFieldSignature::G10cPgpSignatureDetached;
#endif
    }
    return Okular::FormFieldSignature::UnknownType;
}

Okular::SignatureInfo PopplerFormFieldSignature::signatureInfo() const
{
    return m_info;
}

Okular::SigningResult fromPoppler(Poppler::FormFieldSignature::SigningResult r)
{
    switch (r) {
    case Poppler::FormFieldSignature::SigningResult::FieldAlreadySigned:
        return Okular::SigningResult::FieldAlreadySigned;
    case Poppler::FormFieldSignature::SigningResult::GenericSigningError:
        return Okular::SigningResult::GenericSigningError;
    case Poppler::FormFieldSignature::SigningSuccess:
        return Okular::SigningResult::SigningSuccess;
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(24, 12, 0)
    case Poppler::FormFieldSignature::InternalError:
        return Okular::SigningResult::InternalSigningError;
    case Poppler::FormFieldSignature::KeyMissing:
        return Okular::SigningResult::KeyMissing;
    case Poppler::FormFieldSignature::UserCancelled:
        return Okular::SigningResult::UserCancelled;
    case Poppler::FormFieldSignature::WriteFailed:
        return Okular::SigningResult::SignatureWriteFailed;
#endif
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(25, 03, 12)
    case Poppler::FormFieldSignature::BadPassphrase:
        return Okular::SigningResult::BadPassphrase;
#endif
    }
    return Okular::SigningResult::GenericSigningError;
}

std::pair<Okular::SigningResult, QString> PopplerFormFieldSignature::sign(const Okular::NewSignatureData &oData, const QString &newPath) const
{
    Poppler::PDFConverter::NewSignatureData pData;
    PDFGenerator::okularToPoppler(oData, &pData);
    // 0 means "Chose an appropriate size"
    pData.setFontSize(0);
    pData.setLeftFontSize(0);
    auto result = fromPoppler(m_field->sign(newPath, pData));
#if POPPLER_VERSION_MACRO > QT_VERSION_CHECK(25, 06, 0)
    QString errorDetails = m_field->lastSigningErrorDetails().data.toString();
#else
    QString errorDetails;
#endif
    return {result, errorDetails};
}
