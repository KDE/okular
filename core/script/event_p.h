/*
    SPDX-FileCopyrightText: 2018 Intevation GmbH <intevation@intevation.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_EVENT_P_H
#define OKULAR_SCRIPT_EVENT_P_H

#include <QString>
#include <QVariant>

#include <memory>

namespace Okular
{
class FormField;
class Page;

/**
 * @short A JavaScript Event Object data container.
 *
 * Object to represent a JavaScript Event Object as described in the
 * Acrobat JavaScript Scripting Reference.
 *
 * The implementation is currently limited. To implement support
 * for a new event create the according data fields / getters
 * and setters and update the handling in js_event
 * accordingly.
 *
 * See Acrobat JavaScript Scripting Reference for the meaning
 * the fields and add getter / setter according for the
 * event you wish to implement.
 */
class Event
{
private:
    Event();

public:
    enum EventType {
        UnknownEvent,    /// < Unknown
        AppInit,         /// < Not implemented.
        BatchExec,       /// < Not implemented.
        BookmarkMouseUp, /// < Not implemented.
        ConsoleExec,     /// < Not implemented.
        DocDidPrint,     /// < When a document is printed.
        DocDidSave,      /// < When a document is saved.
        DocOpen,         /// < This event is triggered when a document is opened. The document level script functions are scanned and any exposed scripts are executed
        DocWillClose,    /// < When a document is about to be closed.
        DocWillPrint,    /// < When a document is about to be printed.
        DocWillSave,     /// < When a document is about to be saved.
        ExternalExec,    /// < Not implemented.
        FieldBlur,       /// < Not implemented.
        FieldCalculate,  /// < This event is defined in a field re-calculation.
        FieldFocus,      /// < This event is defined when the field gains or loses focus.
        FieldFormat,     /// < When a format action is executed
        FieldKeystroke,  /// < Checks if the entered value is valid.
        FieldMouseDown,  /// < This event is the result of a mouse down on a field.
        FieldMouseEnter, /// < This event is the result of mouse entering in a field.
        FieldMouseExit,  /// < This event is the result of mouse exiting from a field.
        FieldMouseUp,    /// < This event is the result of a mouse up on a field.
        /* Validates the field after every change is committed
         * (clicked outside or tabbed to another field).
         * The enter event is not handled
         */
        FieldValidate,
        LinkMouseUp, /// < Not implemented.
        MenuExec,    /// < Not implemented.
        PageOpen,    /// < Not implemented.
        PageClose,   /// < Not implemented.
    };

    explicit Event(EventType type);

    /** One of the defined EventTypes */
    EventType eventType() const;

    QString name() const;

    QString type() const;

    QString targetName() const;
    void setTargetName(const QString &val);

    Page *targetPage() const;
    void setTargetPage(Page *val);

    FormField *source() const;
    void setSource(FormField *val);

    Page *sourcePage() const;
    void setSourcePage(Page *val);

    void *target() const;
    void setTarget(void *target);

    QVariant value() const;
    void setValue(const QVariant &val);

    bool returnCode() const;
    void setReturnCode(bool returnCode);

    // Checks if the shift key was down when creating the event.
    bool shiftModifier() const;
    void setShiftModifier(bool shiftModifier);

    bool willCommit() const;
    void setWillCommit(bool willCommit);

    QString change() const;
    void setChange(const QString &change);

    int selStart() const;
    void setSelStart(const int selStart);

    int selEnd() const;
    void setSelEnd(const int selEnd);

    static std::shared_ptr<Event> createFormCalculateEvent(FormField *target, Page *targetPage, FormField *source = nullptr, Page *sourcePage = nullptr, const QString &targetName = QString());
    static std::shared_ptr<Event> createFormatEvent(FormField *target, Page *targetPage, const QString &targetName = QString());
    static std::shared_ptr<Event> createKeystrokeEvent(FormField *target, Page *targetPage);
    static std::shared_ptr<Event> createFormFocusEvent(FormField *target, Page *targetPage, const QString &targetName = QString());
    static std::shared_ptr<Event> createFormValidateEvent(FormField *target, Page *targetPage, const QString &targetName = QString());
    static std::shared_ptr<Event> createFieldMouseEvent(FormField *target, Page *targetPage, Event::EventType fieldMouseEventType);
    static std::shared_ptr<Event> createDocEvent(Event::EventType type);

private:
    class Private;
    std::shared_ptr<Private> d;
    Q_DISABLE_COPY(Event)
};

} // namespace Okular
#endif // OKULAR_SCRIPT_EVENT_P_H
