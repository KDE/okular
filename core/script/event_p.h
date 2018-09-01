/***************************************************************************
 *   Copyright (C) 2018 by Intevation GmbH <intevation@intevation.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SCRIPT_EVENT_P_H
#define OKULAR_SCRIPT_EVENT_P_H

#include <QString>
#include <QVariant>

#include <memory>

namespace Okular {

class FormField;
class Page;

/**
 * @short A JavaScript Event Object data container.
 *
 * Object to represet a JavaScript Event Object as described in the
 * Acrobat JavaScript Scripting Reference.
 *
 * The implementation is currently limited. To implement support
 * for a new event create the according data fields / getters
 * and setters and update the handling in kjs_event
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
            DocDidPrint,     /// < Not implemented.
            DocDidSave,      /// < Not implemented.
            DocOpen,         /// < Not implemented.
            DocWillClose,    /// < Not implemented.
            DocWillPrint,    /// < Not implemented.
            DocWillSave,     /// < Not implemented.
            ExternalExec,    /// < Not implemented.
            FieldBlur,       /// < Not implemented.
            FieldCalculate,  /// < This event is defined in a field re-calculation.
            FieldFocus,      /// < Not implemented.
            FieldFormat,     /// < Not implemented.
            FieldKeystroke,  /// < Not implemented.
            FieldMouseDown,  /// < Not implemented.
            FieldMouseEnter, /// < Not implemented.
            FieldMouseExit,  /// < Not implemented.
            FieldMouseUp,    /// < Not implemented.
            FieldValidate,   /// < Not implemented.
            LinkMouseUp,     /// < Not implemented.
            MenuExec,        /// < Not implemented.
            PageOpen,        /// < Not implemented.
            PageClose,       /// < Not implemented.
        };

        explicit Event(EventType type);

        /** One of the defined EventTypes */
        EventType eventType() const;

        QString name() const;

        QString type() const;

        QString targetName() const;
        void setTargetName( const QString &val );

        Page *targetPage() const;
        void setTargetPage( Page *val );

        FormField *source() const;
        void setSource( FormField *val );

        Page *sourcePage() const;
        void setSourcePage( Page *val );

        void *target() const;
        void setTarget( void *target );

        QVariant value() const;
        void setValue(const QVariant &val);

        static std::shared_ptr<Event> createFormCalculateEvent( FormField *target,
                                                                Page *targetPage,
                                                                FormField *source = nullptr,
                                                                Page *sourcePage = nullptr,
                                                                const QString &targetName = QString() );
    private:
        class Private;
        std::shared_ptr<Private> d;
        Q_DISABLE_COPY( Event )
};

} // namespace Okular
#endif //OKULAR_SCRIPT_EVENT_P_H
