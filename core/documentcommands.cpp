/***************************************************************************
 *   Copyright (C) 2013 Jon Mease <jon.mease@gmail.com>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "documentcommands_p.h"

#include "annotations.h"
#include "debug_p.h"
#include "document_p.h"

#include <KLocalizedString>

namespace Okular {

AddAnnotationCommand::AddAnnotationCommand( Okular::DocumentPrivate * docPriv,  Okular::Annotation* annotation, int pageNumber )
 : m_docPriv( docPriv ),
   m_annotation( annotation ),
   m_pageNumber( pageNumber ),
   m_done( false )
{
    setText( i18nc ("Add an annotation to the page", "add annotation" ) );
}

AddAnnotationCommand::~AddAnnotationCommand()
{
    if ( !m_done )
    {
        delete m_annotation;
    }
}

void AddAnnotationCommand::undo()
{
    m_docPriv->performRemovePageAnnotation( m_pageNumber, m_annotation );
    m_done = false;
}

void AddAnnotationCommand::redo()
{
    m_docPriv->performAddPageAnnotation( m_pageNumber,  m_annotation );
    m_done = true;
}


RemoveAnnotationCommand::RemoveAnnotationCommand(Okular::DocumentPrivate * doc,  Okular::Annotation* annotation, int pageNumber)
 : m_doc( doc ),
   m_annotation( annotation ),
   m_pageNumber( pageNumber ),
   m_done( false )
{
    setText( i18nc( "Remove an annotation from the page", "remove annotation" ) );
}

RemoveAnnotationCommand::~RemoveAnnotationCommand()
{
    if ( m_done )
    {
        delete m_annotation;
    }
}

void RemoveAnnotationCommand::undo()
{
    m_doc->performAddPageAnnotation( m_pageNumber,  m_annotation );
    m_done = false;
}

void RemoveAnnotationCommand::redo(){
    m_doc->performRemovePageAnnotation( m_pageNumber, m_annotation );
    m_done = true;
}


ModifyAnnotationPropertiesCommand::ModifyAnnotationPropertiesCommand( DocumentPrivate* docPriv,
                                                                      Annotation* annotation,
                                                                      int pageNumber,
                                                                      QDomNode oldProperties,
                                                                      QDomNode newProperties )
 : m_docPriv( docPriv ),
   m_annotation( annotation ),
   m_pageNumber( pageNumber ),
   m_prevProperties( oldProperties ),
   m_newProperties( newProperties )
{
    setText(i18nc("Modify an annotation's internal properties (Color, line-width, etc.)", "modify annotation properties"));
}

void ModifyAnnotationPropertiesCommand::undo()
{
    m_annotation->setAnnotationProperties( m_prevProperties );
    m_docPriv->performModifyPageAnnotation( m_pageNumber,  m_annotation, true );
}

void ModifyAnnotationPropertiesCommand::redo()
{
    m_annotation->setAnnotationProperties( m_newProperties );
    m_docPriv->performModifyPageAnnotation( m_pageNumber,  m_annotation, true );
}

TranslateAnnotationCommand::TranslateAnnotationCommand( DocumentPrivate* docPriv,
                                                        Annotation* annotation,
                                                        int pageNumber,
                                                        const Okular::NormalizedPoint & delta,
                                                        bool completeDrag )
 : m_docPriv( docPriv ),
   m_annotation( annotation ),
   m_pageNumber( pageNumber ),
   m_delta( delta ),
   m_completeDrag( completeDrag )
{
    setText( i18nc( "Translate an annotation's position on the page", "translate annotation" ) );
}

void TranslateAnnotationCommand::undo()
{
    m_annotation->translate( minusDelta() );
    m_docPriv->performModifyPageAnnotation( m_pageNumber,  m_annotation, true );
}

void TranslateAnnotationCommand::redo()
{
    m_annotation->translate( m_delta );
    m_docPriv->performModifyPageAnnotation( m_pageNumber,  m_annotation, true );
}

int TranslateAnnotationCommand::id() const
{
    return 1;
}

bool TranslateAnnotationCommand::mergeWith( const QUndoCommand* uc )
{
    TranslateAnnotationCommand *tuc = (TranslateAnnotationCommand*)uc;

    if ( tuc->m_annotation != m_annotation )
        return false;

    if ( m_completeDrag )
    {
        return false;
    }
    m_delta = Okular::NormalizedPoint( tuc->m_delta.x + m_delta.x, tuc->m_delta.y + m_delta.y );
    m_completeDrag = tuc->m_completeDrag;
    return true;
}

Okular::NormalizedPoint TranslateAnnotationCommand::minusDelta()
{
    return Okular::NormalizedPoint( -m_delta.x, -m_delta.y );
}


EditTextCommand::EditTextCommand( const QString & newContents,
                                  int newCursorPos,
                                  const QString & prevContents,
                                  int prevCursorPos,
                                  int prevAnchorPos )
 : m_newContents( newContents ),
   m_newCursorPos( newCursorPos ),
   m_prevContents( prevContents ),
   m_prevCursorPos( prevCursorPos ),
   m_prevAnchorPos( prevAnchorPos )
{
    setText( i18nc( "Generic text edit command", "edit text" ) );

    //// Determine edit type
    // If There was a selection then edit was not a simple single character backspace, delete, or insert
    if (m_prevCursorPos != m_prevAnchorPos)
    {
        kDebug(OkularDebug) << "OtherEdit, selection";
        m_editType = OtherEdit;
    }
    else if ( newContentsRightOfCursor() == oldContentsRightOfCursor() &&
              newContentsLeftOfCursor() == oldContentsLeftOfCursor().left(oldContentsLeftOfCursor().length() - 1) &&
              oldContentsLeftOfCursor().right(1) != "\n" )
    {
        kDebug(OkularDebug) << "CharBackspace";
        m_editType = CharBackspace;
    }
    else if ( newContentsLeftOfCursor() == oldContentsLeftOfCursor() &&
              newContentsRightOfCursor() == oldContentsRightOfCursor().right(oldContentsRightOfCursor().length() - 1) &&
              oldContentsRightOfCursor().left(1) != "\n" )
    {
        kDebug(OkularDebug) << "CharDelete";
        m_editType = CharDelete;
    }
    else if ( newContentsRightOfCursor() == oldContentsRightOfCursor() &&
              newContentsLeftOfCursor().left( newContentsLeftOfCursor().length() - 1) == oldContentsLeftOfCursor() &&
              newContentsLeftOfCursor().right(1) != "\n" )
    {
        kDebug(OkularDebug) << "CharInsert";
        m_editType = CharInsert;
    }
    else
    {
        kDebug(OkularDebug) << "OtherEdit";
        m_editType = OtherEdit;
    }
}

bool EditTextCommand::mergeWith(const QUndoCommand* uc)
{
    EditTextCommand *euc = (EditTextCommand*)uc;

    // Only attempt merge of euc into this if our new state matches euc's old state and
    // the editTypes match and are not type OtherEdit
    if ( m_newContents == euc->m_prevContents
        && m_newCursorPos == euc->m_prevCursorPos
        && m_editType == euc->m_editType
        && m_editType != OtherEdit )
    {
        m_newContents = euc->m_newContents;
        m_newCursorPos = euc->m_newCursorPos;
        return true;
    }
    return false;
}

QString EditTextCommand::oldContentsLeftOfCursor()
{
    return m_prevContents.left(m_prevCursorPos);
}

QString EditTextCommand::oldContentsRightOfCursor()
{
    return m_prevContents.right(m_prevContents.length() - m_prevCursorPos);
}

QString EditTextCommand::newContentsLeftOfCursor()
{
    return m_newContents.left(m_newCursorPos);
}

QString EditTextCommand::newContentsRightOfCursor()
{
    return m_newContents.right(m_newContents.length() - m_newCursorPos);
}

EditAnnotationContentsCommand::EditAnnotationContentsCommand( DocumentPrivate* docPriv,
                                                              Annotation* annotation,
                                                              int pageNumber,
                                                              const QString & newContents,
                                                              int newCursorPos,
                                                              const QString & prevContents,
                                                              int prevCursorPos,
                                                              int prevAnchorPos )
: EditTextCommand( newContents, newCursorPos, prevContents, prevCursorPos, prevAnchorPos ),
  m_docPriv( docPriv ),
  m_annotation( annotation ),
  m_pageNumber( pageNumber )
{
    setText( i18nc( "Edit an annotation's text contents", "edit annotation contents" ) );
}

void EditAnnotationContentsCommand::undo()
{
    m_docPriv->performSetAnnotationContents( m_prevContents, m_annotation, m_pageNumber );
    emit m_docPriv->m_parent->annotationContentsChangedByUndoRedo( m_annotation, m_prevContents, m_prevCursorPos, m_prevAnchorPos );
}

void EditAnnotationContentsCommand::redo()
{
    m_docPriv->performSetAnnotationContents( m_newContents, m_annotation, m_pageNumber );
    emit m_docPriv->m_parent->annotationContentsChangedByUndoRedo( m_annotation, m_newContents, m_newCursorPos, m_newCursorPos );
}

int EditAnnotationContentsCommand::id() const
{
    return 2;
}

bool EditAnnotationContentsCommand::mergeWith(const QUndoCommand* uc)
{
    EditAnnotationContentsCommand *euc = (EditAnnotationContentsCommand*)uc;
    // Only attempt merge of euc into this if they modify the same annotation
    if ( m_annotation == euc->m_annotation )
    {
        return EditTextCommand::mergeWith( uc );
    }
    else
    {
        return false;
    }
}

}

