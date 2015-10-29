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
#include "form.h"
#include "utils_p.h"
#include "page.h"

#include <KLocalizedString>

namespace Okular {

void moveViewportIfBoundingRectNotFullyVisible( Okular::NormalizedRect boundingRect,
                                                DocumentPrivate *docPriv,
                                                int pageNumber )
{
    const Rotation pageRotation = docPriv->m_parent->page( pageNumber )->rotation();
    const QTransform rotationMatrix = Okular::buildRotationMatrix( pageRotation );
    boundingRect.transform( rotationMatrix );
    if ( !docPriv->isNormalizedRectangleFullyVisible( boundingRect, pageNumber ) )
    {
        DocumentViewport searchViewport( pageNumber );
        searchViewport.rePos.enabled = true;
        searchViewport.rePos.normalizedX = ( boundingRect.left + boundingRect.right ) / 2.0;
        searchViewport.rePos.normalizedY = ( boundingRect.top + boundingRect.bottom ) / 2.0;
        docPriv->m_parent->setViewport( searchViewport, 0, true );
    }
}

Okular::NormalizedRect buildBoundingRectangleForButtons( const QList<Okular::FormFieldButton*> & formButtons )
{
    // Initialize coordinates of the bounding rect
    double left = 1.0;
    double top = 1.0;
    double right = 0.0;
    double bottom = 0.0;

    foreach( FormFieldButton* formButton, formButtons )
    {
        left = qMin<double>( left, formButton->rect().left );
        top = qMin<double>( top, formButton->rect().top );
        right = qMax<double>( right, formButton->rect().right );
        bottom = qMax<double>( bottom, formButton->rect().bottom );
    }
    Okular::NormalizedRect boundingRect( left, top, right, bottom );
    return boundingRect;
}

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
    moveViewportIfBoundingRectNotFullyVisible( m_annotation->boundingRectangle(), m_docPriv, m_pageNumber );
    m_docPriv->performRemovePageAnnotation( m_pageNumber, m_annotation );
    m_done = false;
}

void AddAnnotationCommand::redo()
{
    moveViewportIfBoundingRectNotFullyVisible( m_annotation->boundingRectangle(), m_docPriv, m_pageNumber );
    m_docPriv->performAddPageAnnotation( m_pageNumber,  m_annotation );
    m_done = true;
}


RemoveAnnotationCommand::RemoveAnnotationCommand(Okular::DocumentPrivate * doc,  Okular::Annotation* annotation, int pageNumber)
 : m_docPriv( doc ),
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
    moveViewportIfBoundingRectNotFullyVisible( m_annotation->boundingRectangle(), m_docPriv, m_pageNumber );
    m_docPriv->performAddPageAnnotation( m_pageNumber,  m_annotation );
    m_done = false;
}

void RemoveAnnotationCommand::redo(){
    moveViewportIfBoundingRectNotFullyVisible( m_annotation->boundingRectangle(), m_docPriv, m_pageNumber );
    m_docPriv->performRemovePageAnnotation( m_pageNumber, m_annotation );
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
    moveViewportIfBoundingRectNotFullyVisible( m_annotation->boundingRectangle(), m_docPriv, m_pageNumber );
    m_annotation->setAnnotationProperties( m_prevProperties );
    m_docPriv->performModifyPageAnnotation( m_pageNumber,  m_annotation, true );
}

void ModifyAnnotationPropertiesCommand::redo()
{
    moveViewportIfBoundingRectNotFullyVisible( m_annotation->boundingRectangle(), m_docPriv, m_pageNumber );
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
    moveViewportIfBoundingRectNotFullyVisible(translateBoundingRectangle(  minusDelta() ), m_docPriv, m_pageNumber );
    m_annotation->translate( minusDelta() );
    m_docPriv->performModifyPageAnnotation( m_pageNumber,  m_annotation, true );
}

void TranslateAnnotationCommand::redo()
{
    moveViewportIfBoundingRectNotFullyVisible(translateBoundingRectangle( m_delta ), m_docPriv, m_pageNumber );
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

Okular::NormalizedRect TranslateAnnotationCommand::translateBoundingRectangle( const Okular::NormalizedPoint & delta )
{
    Okular::NormalizedRect annotBoundingRect = m_annotation->boundingRectangle();
    double left = qMin<double>( annotBoundingRect.left, annotBoundingRect.left + delta.x );
    double right = qMax<double>( annotBoundingRect.right, annotBoundingRect.right + delta.x );
    double top = qMin<double>( annotBoundingRect.top, annotBoundingRect.top + delta.y );
    double bottom = qMax<double>( annotBoundingRect.bottom, annotBoundingRect.bottom + delta.y );
    Okular::NormalizedRect boundingRect( left, top, right, bottom );
    return boundingRect;
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
        qCDebug(OkularCoreDebug) << "OtherEdit, selection";
        m_editType = OtherEdit;
    }
    else if ( newContentsRightOfCursor() == oldContentsRightOfCursor() &&
              newContentsLeftOfCursor() == oldContentsLeftOfCursor().left(oldContentsLeftOfCursor().length() - 1) &&
              oldContentsLeftOfCursor().right(1) != QLatin1String("\n") )
    {
        qCDebug(OkularCoreDebug) << "CharBackspace";
        m_editType = CharBackspace;
    }
    else if ( newContentsLeftOfCursor() == oldContentsLeftOfCursor() &&
              newContentsRightOfCursor() == oldContentsRightOfCursor().right(oldContentsRightOfCursor().length() - 1) &&
              oldContentsRightOfCursor().left(1) != QLatin1String("\n") )
    {
        qCDebug(OkularCoreDebug) << "CharDelete";
        m_editType = CharDelete;
    }
    else if ( newContentsRightOfCursor() == oldContentsRightOfCursor() &&
              newContentsLeftOfCursor().left( newContentsLeftOfCursor().length() - 1) == oldContentsLeftOfCursor() &&
              newContentsLeftOfCursor().right(1) != QLatin1String("\n") )
    {
        qCDebug(OkularCoreDebug) << "CharInsert";
        m_editType = CharInsert;
    }
    else
    {
        qCDebug(OkularCoreDebug) << "OtherEdit";
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
    moveViewportIfBoundingRectNotFullyVisible( m_annotation->boundingRectangle(), m_docPriv, m_pageNumber );
    m_docPriv->performSetAnnotationContents( m_prevContents, m_annotation, m_pageNumber );
    emit m_docPriv->m_parent->annotationContentsChangedByUndoRedo( m_annotation, m_prevContents, m_prevCursorPos, m_prevAnchorPos );
}

void EditAnnotationContentsCommand::redo()
{
    moveViewportIfBoundingRectNotFullyVisible( m_annotation->boundingRectangle(), m_docPriv, m_pageNumber );
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

EditFormTextCommand::EditFormTextCommand( Okular::DocumentPrivate* docPriv,
                                          Okular::FormFieldText* form,
                                          int pageNumber,
                                          const QString & newContents,
                                          int newCursorPos,
                                          const QString & prevContents,
                                          int prevCursorPos,
                                          int prevAnchorPos )
: EditTextCommand( newContents, newCursorPos, prevContents, prevCursorPos, prevAnchorPos ),
  m_docPriv ( docPriv ),
  m_form( form ),
  m_pageNumber( pageNumber )
{
    setText( i18nc( "Edit an form's text contents", "edit form contents" ) );
}

void EditFormTextCommand::undo()
{
    moveViewportIfBoundingRectNotFullyVisible( m_form->rect(), m_docPriv, m_pageNumber );
    m_form->setText( m_prevContents );
    emit m_docPriv->m_parent->formTextChangedByUndoRedo( m_pageNumber, m_form, m_prevContents, m_prevCursorPos, m_prevAnchorPos );
}

void EditFormTextCommand::redo()
{
    moveViewportIfBoundingRectNotFullyVisible( m_form->rect(), m_docPriv, m_pageNumber );
    m_form->setText( m_newContents  );
    emit m_docPriv->m_parent->formTextChangedByUndoRedo( m_pageNumber, m_form, m_newContents, m_newCursorPos, m_newCursorPos );
}

int EditFormTextCommand::id() const
{
    return 3;
}

bool EditFormTextCommand::mergeWith(const QUndoCommand* uc)
{
    EditFormTextCommand *euc = (EditFormTextCommand*)uc;
    // Only attempt merge of euc into this if they modify the same form
    if ( m_form == euc->m_form )
    {
        return EditTextCommand::mergeWith( uc );
    }
    else
    {
        return false;
    }
}

EditFormListCommand::EditFormListCommand( Okular::DocumentPrivate* docPriv,
                                          FormFieldChoice* form,
                                          int pageNumber,
                                          const QList< int > & newChoices,
                                          const QList< int > & prevChoices )
: m_docPriv( docPriv ),
  m_form( form ),
  m_pageNumber( pageNumber ),
  m_newChoices( newChoices ),
  m_prevChoices( prevChoices )
{
    setText( i18nc( "Edit a list form's choices", "edit list form choices" ) );
}

void EditFormListCommand::undo()
{
    moveViewportIfBoundingRectNotFullyVisible( m_form->rect(), m_docPriv, m_pageNumber );
    m_form->setCurrentChoices( m_prevChoices );
    emit m_docPriv->m_parent->formListChangedByUndoRedo( m_pageNumber, m_form, m_prevChoices );
}

void EditFormListCommand::redo()
{
    moveViewportIfBoundingRectNotFullyVisible( m_form->rect(), m_docPriv, m_pageNumber );
    m_form->setCurrentChoices( m_newChoices );
    emit m_docPriv->m_parent->formListChangedByUndoRedo( m_pageNumber, m_form, m_newChoices );
}

EditFormComboCommand::EditFormComboCommand( Okular::DocumentPrivate* docPriv,
                                            FormFieldChoice* form,
                                            int pageNumber,
                                            const QString & newContents,
                                            int newCursorPos,
                                            const QString & prevContents,
                                            int prevCursorPos,
                                            int prevAnchorPos )
: EditTextCommand( newContents, newCursorPos, prevContents, prevCursorPos, prevAnchorPos ),
  m_docPriv( docPriv ),
  m_form( form ),
  m_pageNumber( pageNumber ),
  m_newIndex( -1 ),
  m_prevIndex( -1 )
{
    setText( i18nc( "Edit a combo form's selection", "edit combo form selection" ) );

    // Determine new and previous choice indices (if any)
    for ( int i = 0; i < m_form->choices().size(); i++ )
    {
        if ( m_form->choices()[i] == m_prevContents )
        {
            m_prevIndex = i;
        }

        if ( m_form->choices()[i] == m_newContents )
        {
            m_newIndex = i;
        }
    }
}

void EditFormComboCommand::undo()
{
    if ( m_prevIndex != -1 )
    {
        m_form->setCurrentChoices( QList<int>() << m_prevIndex );
    }
    else
    {
        m_form->setEditChoice( m_prevContents );
    }
    moveViewportIfBoundingRectNotFullyVisible( m_form->rect(), m_docPriv, m_pageNumber );
    emit m_docPriv->m_parent->formComboChangedByUndoRedo( m_pageNumber, m_form, m_prevContents, m_prevCursorPos, m_prevAnchorPos );
}

void EditFormComboCommand::redo()
{
    if ( m_newIndex != -1 )
    {
        m_form->setCurrentChoices( QList<int>() << m_newIndex );
    }
    else
    {
        m_form->setEditChoice( m_newContents );
    }
    moveViewportIfBoundingRectNotFullyVisible( m_form->rect(), m_docPriv, m_pageNumber );
    emit m_docPriv->m_parent->formComboChangedByUndoRedo( m_pageNumber, m_form, m_newContents, m_newCursorPos, m_newCursorPos );
}

int EditFormComboCommand::id() const
{
    return 4;
}

bool EditFormComboCommand::mergeWith( const QUndoCommand *uc )
{
    EditFormComboCommand *euc = (EditFormComboCommand*)uc;
    // Only attempt merge of euc into this if they modify the same form
    if ( m_form == euc->m_form )
    {
        bool shouldMerge = EditTextCommand::mergeWith( uc );
        if( shouldMerge )
        {
            m_newIndex = euc->m_newIndex;
        }
        return shouldMerge;
    }
    else
    {
        return false;
    }
}

EditFormButtonsCommand::EditFormButtonsCommand( Okular::DocumentPrivate* docPriv,
                                                int pageNumber,
                                                const QList< FormFieldButton* > & formButtons,
                                                const QList< bool > & newButtonStates )
: m_docPriv( docPriv ),
  m_pageNumber( pageNumber ),
  m_formButtons( formButtons ),
  m_newButtonStates( newButtonStates ),
  m_prevButtonStates( QList< bool >() )
{
    setText( i18nc( "Edit the state of a group of form buttons", "edit form button states" ) );
    foreach( FormFieldButton* formButton, m_formButtons )
    {
        m_prevButtonStates.append( formButton->state() );
    }
}

void EditFormButtonsCommand::undo()
{
    clearFormButtonStates();
    for( int i = 0; i < m_formButtons.size(); i++ )
    {
        bool checked = m_prevButtonStates.at( i );
        if ( checked )
            m_formButtons.at( i )->setState( checked );
    }

    Okular::NormalizedRect boundingRect = buildBoundingRectangleForButtons( m_formButtons );
    moveViewportIfBoundingRectNotFullyVisible( boundingRect, m_docPriv, m_pageNumber );
    emit m_docPriv->m_parent->formButtonsChangedByUndoRedo( m_pageNumber, m_formButtons );
}

void EditFormButtonsCommand::redo()
{
    clearFormButtonStates();
    for( int i = 0; i < m_formButtons.size(); i++ )
    {
        bool checked = m_newButtonStates.at( i );
        if ( checked )
            m_formButtons.at( i )->setState( checked );
    }

    Okular::NormalizedRect boundingRect = buildBoundingRectangleForButtons( m_formButtons );
    moveViewportIfBoundingRectNotFullyVisible( boundingRect, m_docPriv, m_pageNumber );
    emit m_docPriv->m_parent->formButtonsChangedByUndoRedo( m_pageNumber, m_formButtons );
}

void EditFormButtonsCommand::clearFormButtonStates()
{
    foreach( FormFieldButton* formButton, m_formButtons )
    {
        formButton->setState( false );
    }
}

}

