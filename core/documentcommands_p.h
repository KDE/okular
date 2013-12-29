/***************************************************************************
 *   Copyright (C) 2013 Jon Mease <jon.mease@gmail.com>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_DOCUMENT_COMMANDS_P_H_
#define _OKULAR_DOCUMENT_COMMANDS_P_H_

#include <QtGui/QUndoCommand>
#include <QDomNode>

#include "area.h"

namespace Okular {

class Document;
class Annotation;
class DocumentPrivate;
class FormFieldText;
class FormFieldButton;
class FormFieldChoice;

class AddAnnotationCommand : public QUndoCommand
{
    public:
        AddAnnotationCommand(Okular::DocumentPrivate * docPriv,  Okular::Annotation* annotation, int pageNumber);

        virtual ~AddAnnotationCommand();

        virtual void undo();

        virtual void redo();

    private:
        Okular::DocumentPrivate * m_docPriv;
        Okular::Annotation* m_annotation;
        int m_pageNumber;
        bool m_done;
};

class RemoveAnnotationCommand : public QUndoCommand
{
    public:
        RemoveAnnotationCommand(Okular::DocumentPrivate * doc,  Okular::Annotation* annotation, int pageNumber);
        virtual ~RemoveAnnotationCommand();
        virtual void undo();
        virtual void redo();

    private:
        Okular::DocumentPrivate * m_docPriv;
        Okular::Annotation* m_annotation;
        int m_pageNumber;
        bool m_done;
};

class ModifyAnnotationPropertiesCommand : public QUndoCommand
{
    public:
        ModifyAnnotationPropertiesCommand( Okular::DocumentPrivate* docPriv,  Okular::Annotation*  annotation,
                                                                  int pageNumber,
                                                                  QDomNode oldProperties,
                                                                  QDomNode newProperties );

        virtual void undo();
        virtual void redo();

    private:
        Okular::DocumentPrivate * m_docPriv;
        Okular::Annotation* m_annotation;
        int m_pageNumber;
        QDomNode m_prevProperties;
        QDomNode m_newProperties;
};

class TranslateAnnotationCommand : public QUndoCommand
{
    public:
        TranslateAnnotationCommand(Okular::DocumentPrivate* docPriv,
                                   Okular::Annotation*  annotation,
                                   int pageNumber,
                                   const Okular::NormalizedPoint & delta,
                                   bool completeDrag
                                  );
        virtual void undo();
        virtual void redo();
        virtual int id() const;
        virtual bool mergeWith(const QUndoCommand *uc);
        Okular::NormalizedPoint minusDelta();
        Okular::NormalizedRect translateBoundingRectangle( const Okular::NormalizedPoint & delta );

    private:
        Okular::DocumentPrivate * m_docPriv;
        Okular::Annotation* m_annotation;
        int m_pageNumber;
        Okular::NormalizedPoint m_delta;
        bool m_completeDrag;
};

class EditTextCommand : public QUndoCommand
{
    public:
        EditTextCommand( const QString & newContents,
                         int newCursorPos,
                         const QString & prevContents,
                         int prevCursorPos,
                         int prevAnchorPos
                       );

        virtual void undo() = 0;
        virtual void redo() = 0;
        virtual int id() const = 0;
        virtual bool mergeWith(const QUndoCommand *uc);

    private:
        enum EditType {
            CharBackspace,      ///< Edit made up of one or more single character backspace operations
            CharDelete,         ///< Edit made up of one or more single character delete operations
            CharInsert,         ///< Edit made up of one or more single character insertion operations
            OtherEdit           ///< All other edit operations (these will not be merged together)
        };

        QString oldContentsLeftOfCursor();
        QString newContentsLeftOfCursor();
        QString oldContentsRightOfCursor();
        QString newContentsRightOfCursor();

    protected:
        QString m_newContents;
        int m_newCursorPos;
        QString m_prevContents;
        int m_prevCursorPos;
        int m_prevAnchorPos;
        EditType m_editType;
};


class EditAnnotationContentsCommand : public EditTextCommand
{
    public:
        EditAnnotationContentsCommand(Okular::DocumentPrivate* docPriv,
                                      Okular::Annotation*  annotation,
                                      int pageNumber,
                                      const QString & newContents,
                                      int newCursorPos,
                                      const QString & prevContents,
                                      int prevCursorPos,
                                      int prevAnchorPos
                                     );

        virtual void undo();
        virtual void redo();
        virtual int id() const;
        virtual bool mergeWith(const QUndoCommand *uc);

    private:
        Okular::DocumentPrivate * m_docPriv;
        Okular::Annotation* m_annotation;
        int m_pageNumber;
};

class EditFormTextCommand : public EditTextCommand
{
    public:
        EditFormTextCommand( Okular::DocumentPrivate* docPriv,
                             Okular::FormFieldText* form,
                             int pageNumber,
                             const QString & newContents,
                             int newCursorPos,
                             const QString & prevContents,
                             int prevCursorPos,
                             int prevAnchorPos );
        virtual void undo();
        virtual void redo();
        virtual int id() const;
        virtual bool mergeWith( const QUndoCommand *uc );
    private:
        Okular::DocumentPrivate* m_docPriv;
        Okular::FormFieldText* m_form;
        int m_pageNumber;
};

class EditFormListCommand : public QUndoCommand
{
    public:
        EditFormListCommand( Okular::DocumentPrivate* docPriv,
                             FormFieldChoice* form,
                             int pageNumber,
                             const QList< int > & newChoices,
                             const QList< int > & prevChoices
                           );

        virtual void undo();
        virtual void redo();

    private:
        Okular::DocumentPrivate* m_docPriv;
        FormFieldChoice* m_form;
        int m_pageNumber;
        QList< int > m_newChoices;
        QList< int > m_prevChoices;
};

class EditFormComboCommand : public EditTextCommand
{
    public:
        EditFormComboCommand( Okular::DocumentPrivate* docPriv,
                              FormFieldChoice* form,
                              int pageNumber,
                              const QString & newText,
                              int newCursorPos,
                              const QString & prevText,
                              int prevCursorPos,
                              int prevAnchorPos
                            );

        virtual void undo();
        virtual void redo();
        virtual int id() const;
        virtual bool mergeWith( const QUndoCommand *uc );

    private:
        Okular::DocumentPrivate* m_docPriv;
        FormFieldChoice* m_form;
        int m_pageNumber;
        int m_newIndex;
        int m_prevIndex;
};

class EditFormButtonsCommand : public QUndoCommand
{
    public:
        EditFormButtonsCommand( Okular::DocumentPrivate* docPriv,
                                int pageNumber,
                                const QList< FormFieldButton* > & formButtons,
                                const QList< bool > & newButtonStates
                              );

        virtual void undo();
        virtual void redo();

    private:
        void clearFormButtonStates();

    private:
        Okular::DocumentPrivate* m_docPriv;
        int m_pageNumber;
        QList< FormFieldButton* > m_formButtons;
        QList< bool > m_newButtonStates;
        QList< bool > m_prevButtonStates;
};

}
#endif

/* kate: replace-tabs on; indent-width 4; */
