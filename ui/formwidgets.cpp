/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "formwidgets.h"
#include "pageviewutils.h"

#include <qbuttongroup.h>
#include <QKeyEvent>
#include <QMenu>
#include <QEvent>
#include <klineedit.h>
#include <KLocalizedString>
#include <kstandardaction.h>
#include <qaction.h>
#include <QUrl>

// local includes
#include "core/form.h"
#include "core/document.h"
#include "debug_ui.h"

FormWidgetsController::FormWidgetsController( Okular::Document *doc )
    : QObject( doc ), m_doc( doc )
{
    // emit changed signal when a form has changed
    connect( this, &FormWidgetsController::formTextChangedByUndoRedo,
             this, &FormWidgetsController::changed );
    connect( this, &FormWidgetsController::formListChangedByUndoRedo,
             this, &FormWidgetsController::changed );
    connect( this, &FormWidgetsController::formComboChangedByUndoRedo,
             this, &FormWidgetsController::changed );

    // connect form modification signals to and from document
    connect( this, &FormWidgetsController::formTextChangedByWidget,
             doc, &Okular::Document::editFormText );
    connect( doc, &Okular::Document::formTextChangedByUndoRedo,
             this, &FormWidgetsController::formTextChangedByUndoRedo );

    connect( this, &FormWidgetsController::formListChangedByWidget,
             doc, &Okular::Document::editFormList );
    connect( doc, &Okular::Document::formListChangedByUndoRedo,
             this, &FormWidgetsController::formListChangedByUndoRedo );

    connect( this, &FormWidgetsController::formComboChangedByWidget,
             doc, &Okular::Document::editFormCombo );
    connect( doc, &Okular::Document::formComboChangedByUndoRedo,
             this, &FormWidgetsController::formComboChangedByUndoRedo );

    connect( this, &FormWidgetsController::formButtonsChangedByWidget,
             doc, &Okular::Document::editFormButtons );
    connect( doc, &Okular::Document::formButtonsChangedByUndoRedo,
             this, &FormWidgetsController::slotFormButtonsChangedByUndoRedo );

    // Connect undo/redo signals
    connect( this, &FormWidgetsController::requestUndo,
             doc, &Okular::Document::undo );
    connect( this, &FormWidgetsController::requestRedo,
             doc, &Okular::Document::redo );

    connect( doc, &Okular::Document::canUndoChanged,
             this, &FormWidgetsController::canUndoChanged );
    connect( doc, &Okular::Document::canRedoChanged,
             this, &FormWidgetsController::canRedoChanged );
}

FormWidgetsController::~FormWidgetsController()
{
}

void FormWidgetsController::signalAction( Okular::Action *a )
{
    emit action( a );
}

void FormWidgetsController::registerRadioButton( FormWidgetIface *fwButton, Okular::FormFieldButton *formButton )
{
    if ( !fwButton )
        return;

    QAbstractButton *button = dynamic_cast<QAbstractButton *>(fwButton);
    if ( !button )
    {
        qWarning() << "fwButton is not a QAbstractButton" << fwButton;
        return;
    }

    QList< RadioData >::iterator it = m_radios.begin(), itEnd = m_radios.end();
    const int id = formButton->id();
    m_buttons.insert( id, button );
    for ( ; it != itEnd; ++it )
    {
        const QList< int >::const_iterator idsIt = qFind( (*it).ids, id );
        if ( idsIt != (*it).ids.constEnd() )
        {
            qCDebug(OkularUiDebug) << "Adding id" << id << "To group including" << (*it).ids;
            (*it).group->addButton( button );
            (*it).group->setId( button, id );
            return;
        }
    }

    const QList< int > siblings = formButton->siblings();

    RadioData newdata;
    newdata.ids = siblings;
    newdata.ids.append( id );
    newdata.group = new QButtonGroup();
    newdata.group->addButton( button );
    newdata.group->setId( button, id );

    // Groups of 1 (like checkboxes) can't be exclusive
    if (siblings.isEmpty())
        newdata.group->setExclusive( false );

    connect( newdata.group, SIGNAL( buttonClicked(QAbstractButton* ) ),
             this, SLOT( slotButtonClicked( QAbstractButton* ) ) );
    m_radios.append( newdata );
}

void FormWidgetsController::dropRadioButtons()
{
    QList< RadioData >::iterator it = m_radios.begin(), itEnd = m_radios.end();
    for ( ; it != itEnd; ++it )
    {
        delete (*it).group;
    }
    m_radios.clear();
    m_buttons.clear();
}

bool FormWidgetsController::canUndo()
{
    return m_doc->canUndo();
}

bool FormWidgetsController::canRedo()
{
    return m_doc->canRedo();
}

void FormWidgetsController::slotButtonClicked( QAbstractButton *button )
{
    int pageNumber = -1;
    if ( CheckBoxEdit *check = qobject_cast< CheckBoxEdit * >( button ) )
    {
        // Checkboxes need to be uncheckable so if clicking a checked one
        // disable the exclusive status temporarily and uncheck it
        Okular::FormFieldButton *formButton = static_cast<Okular::FormFieldButton *>( check->formField() );
        if ( formButton->state() ) {
            const bool wasExclusive = button->group()->exclusive();
            button->group()->setExclusive(false);
            check->setChecked(false);
            button->group()->setExclusive(wasExclusive);
        }
        pageNumber = check->pageItem()->pageNumber();
    }
    else if ( RadioButtonEdit *radio = qobject_cast< RadioButtonEdit * >( button ) )
    {
        pageNumber = radio->pageItem()->pageNumber();
    }

    const QList< QAbstractButton* > buttons = button->group()->buttons();
    QList< bool > checked;
    QList< bool > prevChecked;
    QList< Okular::FormFieldButton*> formButtons;

    foreach ( QAbstractButton* button, buttons )
    {
        checked.append( button->isChecked() );
        Okular::FormFieldButton *formButton = static_cast<Okular::FormFieldButton *>( dynamic_cast<FormWidgetIface*>(button)->formField() );
        formButtons.append( formButton );
        prevChecked.append( formButton->state() );
    }
    if (checked != prevChecked)
        emit formButtonsChangedByWidget( pageNumber, formButtons, checked );
}

void FormWidgetsController::slotFormButtonsChangedByUndoRedo( int pageNumber, const QList< Okular::FormFieldButton* > & formButtons)
{
    foreach ( Okular::FormFieldButton* formButton, formButtons )
    {
        int id = formButton->id();
        QAbstractButton* button = m_buttons[id];
        // temporarily disable exclusiveness of the button group
        // since it breaks doing/redoing steps into which all the checkboxes
        // are unchecked
        const bool wasExclusive = button->group()->exclusive();
        button->group()->setExclusive(false);
        bool checked = formButton->state();
        button->setChecked( checked );
        button->group()->setExclusive(wasExclusive);
        button->setFocus();
    }
    emit changed( pageNumber );
}

FormWidgetIface * FormWidgetFactory::createWidget( Okular::FormField * ff, QWidget * parent )
{
    FormWidgetIface * widget = nullptr;

    if (ff->isReadOnly())
        return nullptr;

    switch ( ff->type() )
    {
        case Okular::FormField::FormButton:
        {
            Okular::FormFieldButton * ffb = static_cast< Okular::FormFieldButton * >( ff );
            switch ( ffb->buttonType() )
            {
                case Okular::FormFieldButton::Push:
                    widget = new PushButtonEdit( ffb, parent );
                    break;
                case Okular::FormFieldButton::CheckBox:
                    widget = new CheckBoxEdit( ffb, parent );
                    break;
                case Okular::FormFieldButton::Radio:
                    widget = new RadioButtonEdit( ffb, parent );
                    break;
                default: ;
            }
            break;
        }
        case Okular::FormField::FormText:
        {
            Okular::FormFieldText * fft = static_cast< Okular::FormFieldText * >( ff );
            switch ( fft->textType() )
            {
                case Okular::FormFieldText::Multiline:
                    widget = new TextAreaEdit( fft, parent );
                    break;
                case Okular::FormFieldText::Normal:
                    widget = new FormLineEdit( fft, parent );
                    break;
                case Okular::FormFieldText::FileSelect:
                    widget = new FileEdit( fft, parent );
                    break;
            }
            break;
        }
        case Okular::FormField::FormChoice:
        {
            Okular::FormFieldChoice * ffc = static_cast< Okular::FormFieldChoice * >( ff );
            switch ( ffc->choiceType() )
            {
                case Okular::FormFieldChoice::ListBox:
                    widget = new ListEdit( ffc, parent );
                    break;
                case Okular::FormFieldChoice::ComboBox:
                    widget = new ComboEdit( ffc, parent );
                    break;
            }
            break;
        }
        default: ;
    }
    return widget;
}


FormWidgetIface::FormWidgetIface( QWidget * w, Okular::FormField * ff, bool canBeEnabled )
    : m_controller( nullptr ), m_ff( ff ), m_widget( w ), m_pageItem( nullptr ),
      m_canBeEnabled( canBeEnabled )
{
    m_widget->setEnabled( m_canBeEnabled );
}

FormWidgetIface::~FormWidgetIface()
{
}

Okular::NormalizedRect FormWidgetIface::rect() const
{
    return m_ff->rect();
}

void FormWidgetIface::setWidthHeight( int w, int h )
{
    m_widget->resize( w, h );
}

void FormWidgetIface::moveTo( int x, int y )
{
    m_widget->move( x, y );
}

bool FormWidgetIface::setVisibility( bool visible )
{
    bool hadfocus = m_widget->hasFocus();
    if ( hadfocus )
        m_widget->clearFocus();
    m_widget->setVisible( visible );
    return hadfocus;
}

void FormWidgetIface::setCanBeFilled( bool fill )
{
    m_widget->setEnabled( fill && m_canBeEnabled );
}

void FormWidgetIface::setPageItem( PageViewItem *pageItem )
{
    m_pageItem = pageItem;
}

void FormWidgetIface::setFormField( Okular::FormField *field )
{
    m_ff = field;
}

Okular::FormField* FormWidgetIface::formField() const
{
    return m_ff;
}

PageViewItem* FormWidgetIface::pageItem() const
{
    return m_pageItem;
}

void FormWidgetIface::setFormWidgetsController( FormWidgetsController *controller )
{
    m_controller = controller;
}


PushButtonEdit::PushButtonEdit( Okular::FormFieldButton * button, QWidget * parent )
    : QPushButton( parent ), FormWidgetIface( this, button, !button->isReadOnly() )
{
    setText( button->caption() );
    setVisible( button->isVisible() );
    setCursor( Qt::ArrowCursor );

    connect( this, &QAbstractButton::clicked, this, &PushButtonEdit::slotClicked );
}

void PushButtonEdit::slotClicked()
{
    if ( m_ff->activationAction() )
        m_controller->signalAction( m_ff->activationAction() );
}


CheckBoxEdit::CheckBoxEdit( Okular::FormFieldButton * button, QWidget * parent )
    : QCheckBox( parent ), FormWidgetIface( this, button, !button->isReadOnly() )
{
    setText( button->caption() );

    setVisible( button->isVisible() );
    setCursor( Qt::ArrowCursor );
}

void CheckBoxEdit::setFormWidgetsController( FormWidgetsController *controller )
{
    Okular::FormFieldButton *form = static_cast<Okular::FormFieldButton *>(m_ff);
    FormWidgetIface::setFormWidgetsController( controller );
    m_controller->registerRadioButton( this, form );
    setChecked( form->state() );
    connect( this, &QCheckBox::stateChanged, this, &CheckBoxEdit::slotStateChanged );
}

void CheckBoxEdit::slotStateChanged( int state )
{
    Okular::FormFieldButton *form = static_cast<Okular::FormFieldButton *>(m_ff);
    if ( state == Qt::Checked && form->activationAction() )
        m_controller->signalAction( form->activationAction() );
}


RadioButtonEdit::RadioButtonEdit( Okular::FormFieldButton * button, QWidget * parent )
    : QRadioButton( parent ), FormWidgetIface( this, button, !button->isReadOnly() )
{
    setText( button->caption() );

    setVisible( button->isVisible() );
    setCursor( Qt::ArrowCursor );
}

void RadioButtonEdit::setFormWidgetsController( FormWidgetsController *controller )
{
    Okular::FormFieldButton *form = static_cast<Okular::FormFieldButton *>(m_ff);
    FormWidgetIface::setFormWidgetsController( controller );
    m_controller->registerRadioButton( this, form );
    setChecked( form->state() );
}


FormLineEdit::FormLineEdit( Okular::FormFieldText * text, QWidget * parent )
    : QLineEdit( parent ), FormWidgetIface( this, text, true )
{
    int maxlen = text->maximumLength();
    if ( maxlen >= 0 )
        setMaxLength( maxlen );
    setAlignment( text->textAlignment() );
    setText( text->text() );
    if ( text->isPassword() )
        setEchoMode( QLineEdit::Password );

    m_prevCursorPos = cursorPosition();
    m_prevAnchorPos = cursorPosition();

    connect( this, &QLineEdit::textEdited, this, &FormLineEdit::slotChanged );
    connect( this, &QLineEdit::cursorPositionChanged, this, &FormLineEdit::slotChanged );

    setVisible( text->isVisible() );
}

void FormLineEdit::setFormWidgetsController(FormWidgetsController* controller)
{
    FormWidgetIface::setFormWidgetsController(controller);
    connect( m_controller, &FormWidgetsController::formTextChangedByUndoRedo,
             this, &FormLineEdit::slotHandleTextChangedByUndoRedo );
}

bool FormLineEdit::event( QEvent* e )
{
    if ( e->type() == QEvent::KeyPress )
    {
        QKeyEvent *keyEvent = static_cast< QKeyEvent* >( e );
        if ( keyEvent == QKeySequence::Undo )
        {
            emit m_controller->requestUndo();
            return true;
        }
        else if ( keyEvent == QKeySequence::Redo )
        {
            emit m_controller->requestRedo();
            return true;
        }
    }
    return QLineEdit::event( e );
}

void FormLineEdit::contextMenuEvent( QContextMenuEvent* event )
{
    QMenu *menu = createStandardContextMenu();

    QList<QAction *> actionList = menu->actions();
    enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, DeleteAct, SelectAllAct };

    QAction *kundo = KStandardAction::create( KStandardAction::Undo, m_controller, SIGNAL( requestUndo() ), menu );
    QAction *kredo = KStandardAction::create( KStandardAction::Redo, m_controller, SIGNAL( requestRedo() ), menu );
    connect( m_controller, &FormWidgetsController::canUndoChanged, kundo, &QAction::setEnabled );
    connect( m_controller, &FormWidgetsController::canRedoChanged, kredo, &QAction::setEnabled );
    kundo->setEnabled( m_controller->canUndo() );
    kredo->setEnabled( m_controller->canRedo() );

    QAction *oldUndo, *oldRedo;
    oldUndo = actionList[UndoAct];
    oldRedo = actionList[RedoAct];

    menu->insertAction( oldUndo, kundo );
    menu->insertAction( oldRedo, kredo );

    menu->removeAction( oldUndo );
    menu->removeAction( oldRedo );

    menu->exec( event->globalPos() );
    delete menu;
}

void FormLineEdit::slotChanged()
{
    Okular::FormFieldText *form = static_cast<Okular::FormFieldText *>(m_ff);
    QString contents = text();
    int cursorPos = cursorPosition();
    if ( contents != form->text() )
    {
        m_controller->formTextChangedByWidget( pageItem()->pageNumber(),
                                               form,
                                               contents,
                                               cursorPos,
                                               m_prevCursorPos,
                                               m_prevAnchorPos );
    }

    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = cursorPos;
    if ( hasSelectedText() ) {
        if ( cursorPos == selectionStart() ) {
            m_prevAnchorPos = selectionStart() + selectedText().size();
        } else {
            m_prevAnchorPos = selectionStart();
        }
    }
}

void FormLineEdit::slotHandleTextChangedByUndoRedo( int pageNumber,
                                                    Okular::FormFieldText* textForm,
                                                    const QString & contents,
                                                    int cursorPos,
                                                    int anchorPos )
{
    Q_UNUSED(pageNumber);
    if ( textForm != m_ff || contents == text() )
    {
        return;
    }
    disconnect( this, &QLineEdit::cursorPositionChanged, this, &FormLineEdit::slotChanged );
    setText(contents);
    setCursorPosition(anchorPos);
    cursorForward( true, cursorPos - anchorPos );
    connect( this, &QLineEdit::cursorPositionChanged, this, &FormLineEdit::slotChanged );
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = anchorPos;
    setFocus();
}

TextAreaEdit::TextAreaEdit( Okular::FormFieldText * text, QWidget * parent )
: KTextEdit( parent ), FormWidgetIface( this, text, true )
{
    setAcceptRichText( text->isRichText() );
    setCheckSpellingEnabled( text->canBeSpellChecked() );
    setAlignment( text->textAlignment() );
    setPlainText( text->text() );
    setUndoRedoEnabled( false );

    connect( this, &QTextEdit::textChanged, this, &TextAreaEdit::slotChanged );
    connect( this, &QTextEdit::cursorPositionChanged, this, &TextAreaEdit::slotChanged );
    connect( this, &KTextEdit::aboutToShowContextMenu,
             this, &TextAreaEdit::slotUpdateUndoAndRedoInContextMenu );
    m_prevCursorPos = textCursor().position();
    m_prevAnchorPos = textCursor().anchor();
    setVisible( text->isVisible() );
}

bool TextAreaEdit::event( QEvent* e )
{
    if ( e->type() == QEvent::KeyPress )
    {
        QKeyEvent *keyEvent = static_cast< QKeyEvent* >(e);
        if ( keyEvent == QKeySequence::Undo )
        {
            emit m_controller->requestUndo();
            return true;
        }
        else if ( keyEvent == QKeySequence::Redo )
        {
            emit m_controller->requestRedo();
            return true;
        }
    }
    return KTextEdit::event( e );
}

void TextAreaEdit::slotUpdateUndoAndRedoInContextMenu( QMenu* menu )
{
    if ( !menu ) return;

    QList<QAction *> actionList = menu->actions();
    enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, ClearAct, SelectAllAct, NCountActs };

    QAction *kundo = KStandardAction::create( KStandardAction::Undo, m_controller, SIGNAL( requestUndo() ), menu );
    QAction *kredo = KStandardAction::create( KStandardAction::Redo, m_controller, SIGNAL( requestRedo() ), menu );
    connect(m_controller, &FormWidgetsController::canUndoChanged, kundo, &QAction::setEnabled );
    connect(m_controller, &FormWidgetsController::canRedoChanged, kredo, &QAction::setEnabled );
    kundo->setEnabled( m_controller->canUndo() );
    kredo->setEnabled( m_controller->canRedo() );

    QAction *oldUndo, *oldRedo;
    oldUndo = actionList[UndoAct];
    oldRedo = actionList[RedoAct];

    menu->insertAction( oldUndo, kundo );
    menu->insertAction( oldRedo, kredo );

    menu->removeAction( oldUndo );
    menu->removeAction( oldRedo );
}

void TextAreaEdit::setFormWidgetsController( FormWidgetsController* controller )
{
    FormWidgetIface::setFormWidgetsController( controller );
    connect( m_controller, &FormWidgetsController::formTextChangedByUndoRedo,
             this, &TextAreaEdit::slotHandleTextChangedByUndoRedo );
}

void TextAreaEdit::slotHandleTextChangedByUndoRedo( int pageNumber,
                                                    Okular::FormFieldText* textForm,
                                                    const QString & contents,
                                                    int cursorPos,
                                                    int anchorPos )
{
    Q_UNUSED(pageNumber);
    if ( textForm != m_ff )
    {
        return;
    }
    setPlainText( contents );
    QTextCursor c = textCursor();
    c.setPosition( anchorPos );
    c.setPosition( cursorPos,QTextCursor::KeepAnchor );
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = anchorPos;
    setTextCursor( c );
    setFocus();
}

void TextAreaEdit::slotChanged()
{
    Okular::FormFieldText *form = static_cast<Okular::FormFieldText *>(m_ff);
    QString contents = toPlainText();
    int cursorPos = textCursor().position();
    if (contents != form->text())
    {
        m_controller->formTextChangedByWidget( pageItem()->pageNumber(),
                                               form,
                                               contents,
                                               cursorPos,
                                               m_prevCursorPos,
                                               m_prevAnchorPos );
    }
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = textCursor().anchor();
}


FileEdit::FileEdit( Okular::FormFieldText * text, QWidget * parent )
    : KUrlRequester( parent ), FormWidgetIface( this, text, !text->isReadOnly() )
{
    setMode( KFile::File | KFile::ExistingOnly | KFile::LocalOnly );
    setFilter( i18n( "*|All Files" ) );
    setUrl( QUrl::fromUserInput( text->text() ) );
    lineEdit()->setAlignment( text->textAlignment() );

    m_prevCursorPos = lineEdit()->cursorPosition();
    m_prevAnchorPos = lineEdit()->cursorPosition();

    connect( this, &KUrlRequester::textChanged, this, &FileEdit::slotChanged );
    connect( lineEdit(), &QLineEdit::cursorPositionChanged, this, &FileEdit::slotChanged );
    setVisible( text->isVisible() );
}

void FileEdit::setFormWidgetsController( FormWidgetsController* controller )
{
    FormWidgetIface::setFormWidgetsController( controller );
    connect( m_controller, &FormWidgetsController::formTextChangedByUndoRedo,
             this, &FileEdit::slotHandleFileChangedByUndoRedo );
}

bool FileEdit::eventFilter( QObject* obj, QEvent* event )
{
    if ( obj == lineEdit() ) {
        if ( event->type() == QEvent::KeyPress )
        {
            QKeyEvent *keyEvent = static_cast< QKeyEvent* >( event );
            if ( keyEvent == QKeySequence::Undo )
            {
                emit m_controller->requestUndo();
                return true;
            }
            else if ( keyEvent == QKeySequence::Redo )
            {
                emit m_controller->requestRedo();
                return true;
            }
        }
        else if( event->type() == QEvent::ContextMenu )
        {
            QContextMenuEvent *contextMenuEvent = static_cast< QContextMenuEvent* >( event );

            QMenu *menu = ( (QLineEdit*) lineEdit() )->createStandardContextMenu();

            QList< QAction* > actionList = menu->actions();
            enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, DeleteAct, SelectAllAct };

            QAction *kundo = KStandardAction::create( KStandardAction::Undo, m_controller, SIGNAL( requestUndo() ), menu );
            QAction *kredo = KStandardAction::create( KStandardAction::Redo, m_controller, SIGNAL( requestRedo() ), menu );
            connect(m_controller, &FormWidgetsController::canUndoChanged, kundo, &QAction::setEnabled );
            connect(m_controller, &FormWidgetsController::canRedoChanged, kredo, &QAction::setEnabled );
            kundo->setEnabled( m_controller->canUndo() );
            kredo->setEnabled( m_controller->canRedo() );

            QAction *oldUndo, *oldRedo;
            oldUndo = actionList[UndoAct];
            oldRedo = actionList[RedoAct];

            menu->insertAction( oldUndo, kundo );
            menu->insertAction( oldRedo, kredo );

            menu->removeAction( oldUndo );
            menu->removeAction( oldRedo );

            menu->exec( contextMenuEvent->globalPos() );
            delete menu;
            return true;
        }
    }
    return KUrlRequester::eventFilter( obj, event );
}

void FileEdit::slotChanged()
{
    // Make sure line edit's text matches url expansion
    if ( text() != url().toLocalFile() )
        this->setText( url().toLocalFile() );

    Okular::FormFieldText *form = static_cast<Okular::FormFieldText *>(m_ff);

    QString contents = text();
    int cursorPos = lineEdit()->cursorPosition();
    if (contents != form->text())
    {
        m_controller->formTextChangedByWidget( pageItem()->pageNumber(),
                                               form,
                                               contents,
                                               cursorPos,
                                               m_prevCursorPos,
                                               m_prevAnchorPos );
    }

    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = cursorPos;
    if ( lineEdit()->hasSelectedText() ) {
        if ( cursorPos == lineEdit()->selectionStart() ) {
            m_prevAnchorPos = lineEdit()->selectionStart() + lineEdit()->selectedText().size();
        } else {
            m_prevAnchorPos = lineEdit()->selectionStart();
        }
    }
}

void FileEdit::slotHandleFileChangedByUndoRedo( int pageNumber,
                                                Okular::FormFieldText* form,
                                                const QString & contents,
                                                int cursorPos,
                                                int anchorPos )
{
    Q_UNUSED(pageNumber);
    if ( form != m_ff || contents == text() )
    {
        return;
    }
    disconnect( this, SIGNAL( cursorPositionChanged( int, int ) ), this, SLOT( slotChanged() ) );
    setText( contents );
    lineEdit()->setCursorPosition( anchorPos );
    lineEdit()->cursorForward( true, cursorPos - anchorPos );
    connect( this, SIGNAL(cursorPositionChanged( int, int ) ), this, SLOT( slotChanged() ) );
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = anchorPos;
    setFocus();
}

ListEdit::ListEdit( Okular::FormFieldChoice * choice, QWidget * parent )
    : QListWidget( parent ), FormWidgetIface( this, choice, !choice->isReadOnly() )
{
    addItems( choice->choices() );
    setSelectionMode( choice->multiSelect() ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    QList< int > selectedItems = choice->currentChoices();
    if ( choice->multiSelect() )
    {
        foreach ( int index, selectedItems )
            if ( index >= 0 && index < count() )
                item( index )->setSelected( true );
    }
    else
    {
        if ( selectedItems.count() == 1 && selectedItems.at(0) >= 0 && selectedItems.at(0) < count() )
        {
            setCurrentRow( selectedItems.at(0) );
            scrollToItem( item( selectedItems.at(0) ) );
        }
    }

    connect( this, &QListWidget::itemSelectionChanged, this, &ListEdit::slotSelectionChanged );

    setVisible( choice->isVisible() );
    setCursor( Qt::ArrowCursor );
}

void ListEdit::setFormWidgetsController( FormWidgetsController* controller )
{
    FormWidgetIface::setFormWidgetsController( controller );
    connect( m_controller, &FormWidgetsController::formListChangedByUndoRedo,
             this, &ListEdit::slotHandleFormListChangedByUndoRedo );
}

void ListEdit::slotSelectionChanged()
{
    QList< QListWidgetItem * > selection = selectedItems();
    QList< int > rows;
    foreach( const QListWidgetItem * item, selection )
        rows.append( row( item ) );

    Okular::FormFieldChoice *form = static_cast<Okular::FormFieldChoice *>(m_ff);
    if ( rows != form->currentChoices() ) {
        m_controller->formListChangedByWidget( pageItem()->pageNumber(),
                                               form,
                                               rows );
    }
}

void ListEdit::slotHandleFormListChangedByUndoRedo( int pageNumber,
                                                    Okular::FormFieldChoice* listForm,
                                                    const QList< int > & choices )
{
    Q_UNUSED(pageNumber);

    if ( m_ff != listForm ) {
        return;
    }

    disconnect( this, &QListWidget::itemSelectionChanged, this, &ListEdit::slotSelectionChanged );
    for(int i=0; i < count(); i++)
    {
        item( i )->setSelected( choices.contains(i) );
    }
    connect( this, &QListWidget::itemSelectionChanged, this, &ListEdit::slotSelectionChanged );

    setFocus();
}

ComboEdit::ComboEdit( Okular::FormFieldChoice * choice, QWidget * parent )
    : QComboBox( parent ), FormWidgetIface( this, choice, !choice->isReadOnly() )
{
    addItems( choice->choices() );
    setEditable( true );
    setInsertPolicy( NoInsert );
    lineEdit()->setReadOnly( !choice->isEditable() );
    QList< int > selectedItems = choice->currentChoices();
    if ( selectedItems.count() == 1 && selectedItems.at(0) >= 0 && selectedItems.at(0) < count() )
        setCurrentIndex( selectedItems.at(0) );

    if ( choice->isEditable() && !choice->editChoice().isEmpty() )
        lineEdit()->setText( choice->editChoice() );

    connect( this, SIGNAL(currentIndexChanged(int)), this, SLOT(slotValueChanged()) );
    connect( this, &QComboBox::editTextChanged, this, &ComboEdit::slotValueChanged );
    connect( lineEdit(), &QLineEdit::cursorPositionChanged, this, &ComboEdit::slotValueChanged );

    setVisible( choice->isVisible() );
    setCursor( Qt::ArrowCursor );
    m_prevCursorPos = lineEdit()->cursorPosition();
    m_prevAnchorPos = lineEdit()->cursorPosition();
}

void ComboEdit::setFormWidgetsController(FormWidgetsController* controller)
{
    FormWidgetIface::setFormWidgetsController(controller);
    connect( m_controller, &FormWidgetsController::formComboChangedByUndoRedo,
             this, &ComboEdit::slotHandleFormComboChangedByUndoRedo);

}

void ComboEdit::slotValueChanged()
{
    const QString text = lineEdit()->text();

    Okular::FormFieldChoice *form = static_cast<Okular::FormFieldChoice *>(m_ff);

    QString prevText;
    if ( form->currentChoices().isEmpty() )
    {
        prevText = form->editChoice();
    }
    else
    {
        prevText = form->choices()[form->currentChoices()[0]];
    }

    int cursorPos = lineEdit()->cursorPosition();
    if ( text != prevText )
    {
        m_controller->formComboChangedByWidget( pageItem()->pageNumber(),
                                                form,
                                                currentText(),
                                                cursorPos,
                                                m_prevCursorPos,
                                                m_prevAnchorPos
                                              );
    }
    prevText = text;
    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = cursorPos;
    if ( lineEdit()->hasSelectedText() ) {
        if ( cursorPos == lineEdit()->selectionStart() ) {
            m_prevAnchorPos = lineEdit()->selectionStart() + lineEdit()->selectedText().size();
        } else {
            m_prevAnchorPos = lineEdit()->selectionStart();
        }
    }
}

void ComboEdit::slotHandleFormComboChangedByUndoRedo( int pageNumber,
                                                      Okular::FormFieldChoice* form,
                                                      const QString & text,
                                                      int cursorPos,
                                                      int anchorPos )
{
    Q_UNUSED(pageNumber);

    if ( m_ff != form ) {
        return;
    }

    // Determine if text corrisponds to an index choices
    int index = -1;
    for ( int i = 0; i < count(); i++ )
    {
        if ( itemText(i) == text )
        {
            index = i;
        }
    }

    m_prevCursorPos = cursorPos;
    m_prevAnchorPos = anchorPos;

    disconnect( lineEdit(), &QLineEdit::cursorPositionChanged, this, &ComboEdit::slotValueChanged );
    const bool isCustomValue = index == -1;
    if ( isCustomValue )
    {
        setEditText( text );
    }
    else
    {
        setCurrentIndex( index );
    }
    lineEdit()->setCursorPosition( anchorPos );
    lineEdit()->cursorForward( true, cursorPos - anchorPos );
    connect( lineEdit(), &QLineEdit::cursorPositionChanged, this, &ComboEdit::slotValueChanged );
    setFocus();
}

void ComboEdit::contextMenuEvent( QContextMenuEvent* event )
{
    QMenu *menu = lineEdit()->createStandardContextMenu();

    QList<QAction *> actionList = menu->actions();
    enum { UndoAct, RedoAct, CutAct, CopyAct, PasteAct, DeleteAct, SelectAllAct };

    QAction *kundo = KStandardAction::create( KStandardAction::Undo, m_controller, SIGNAL( requestUndo() ), menu );
    QAction *kredo = KStandardAction::create( KStandardAction::Redo, m_controller, SIGNAL( requestRedo() ), menu );
    connect( m_controller, &FormWidgetsController::canUndoChanged, kundo, &QAction::setEnabled );
    connect( m_controller, &FormWidgetsController::canRedoChanged, kredo, &QAction::setEnabled );
    kundo->setEnabled( m_controller->canUndo() );
    kredo->setEnabled( m_controller->canRedo() );

    QAction *oldUndo, *oldRedo;
    oldUndo = actionList[UndoAct];
    oldRedo = actionList[RedoAct];

    menu->insertAction( oldUndo, kundo );
    menu->insertAction( oldRedo, kredo );

    menu->removeAction( oldUndo );
    menu->removeAction( oldRedo );

    menu->exec( event->globalPos() );
    delete menu;
}

bool ComboEdit::event( QEvent* e )
{
    if ( e->type() == QEvent::KeyPress )
    {
        QKeyEvent *keyEvent = static_cast< QKeyEvent* >(e);
        if ( keyEvent == QKeySequence::Undo )
        {
            emit m_controller->requestUndo();
            return true;
        }
        else if ( keyEvent == QKeySequence::Redo )
        {
            emit m_controller->requestRedo();
            return true;
        }
    }
    return QComboBox::event( e );
}

#include "moc_formwidgets.cpp"
