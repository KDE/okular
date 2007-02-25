/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <klineedit.h>
#include <klocale.h>

// local includes
#include "formwidgets.h"
#include "core/form.h"

FormWidgetIface * FormWidgetFactory::createWidget( Okular::FormField * ff, QWidget * parent )
{
    FormWidgetIface * widget = 0;
    switch ( ff->type() )
    {
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


FormWidgetIface::FormWidgetIface( QWidget * w, Okular::FormField * ff )
    : m_widget( w ), m_ff( ff )
{
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
    if ( !m_ff->isVisible() )
        return false;

    bool hadfocus = m_widget->hasFocus();
    if ( hadfocus )
        m_widget->clearFocus();
    m_widget->setVisible( visible );
    return hadfocus;
}


FormLineEdit::FormLineEdit( Okular::FormFieldText * text, QWidget * parent )
    : QLineEdit( parent ), FormWidgetIface( this, text ), m_form( text )
{
    int maxlen = m_form->maximumLength();
    if ( maxlen >= 0 )
        setMaxLength( maxlen );
    setAlignment( m_form->textAlignment() );
    setText( m_form->text() );
    if ( m_form->isPassword() )
        setEchoMode( QLineEdit::Password );
    setEnabled( !m_form->isReadOnly() );

    connect( this, SIGNAL( textEdited( const QString& ) ), this, SLOT( textEdited( const QString& ) ) );
    setVisible( m_form->isVisible() );
}

void FormLineEdit::textEdited( const QString& )
{
    m_form->setText( text() );
}


TextAreaEdit::TextAreaEdit( Okular::FormFieldText * text, QWidget * parent )
    : KTextEdit( parent ), FormWidgetIface( this, text ), m_form( text )
{
    setAcceptRichText( m_form->isRichText() );
    setCheckSpellingEnabled( m_form->canBeSpellChecked() );
    setAlignment( m_form->textAlignment() );
    setPlainText( m_form->text() );
    setEnabled( !m_form->isReadOnly() );

    connect( this, SIGNAL( textChanged() ), this, SLOT( slotChanged() ) );
    setVisible( m_form->isVisible() );
}

void TextAreaEdit::slotChanged()
{
    m_form->setText( toPlainText() );
}


FileEdit::FileEdit( Okular::FormFieldText * text, QWidget * parent )
    : KUrlRequester( parent ), FormWidgetIface( this, text ), m_form( text )
{
    setMode( KFile::File | KFile::ExistingOnly | KFile::LocalOnly );
    setFilter( QString( "*|" ) + i18n( "All files" ) );
    setPath( m_form->text() );
    lineEdit()->setAlignment( m_form->textAlignment() );
    setEnabled( !m_form->isReadOnly() );

    connect( this, SIGNAL( textChanged( const QString& ) ), this, SLOT( slotChanged( const QString& ) ) );
    setVisible( m_form->isVisible() );
}

void FileEdit::slotChanged( const QString& )
{
    m_form->setText( url().path() );
}


ListEdit::ListEdit( Okular::FormFieldChoice * choice, QWidget * parent )
    : QListWidget( parent ), FormWidgetIface( this, choice ), m_form( choice )
{
    addItems( m_form->choices() );
    setSelectionMode( m_form->multiSelect() ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    QList< int > selectedItems = m_form->currentChoices();
    if ( m_form->multiSelect() )
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
    setEnabled( !m_form->isReadOnly() );

    connect( this, SIGNAL( itemSelectionChanged() ), this, SLOT( selectionChanged() ) );
    setVisible( m_form->isVisible() );
}

void ListEdit::selectionChanged()
{
    QList< QListWidgetItem * > selection = selectedItems();
    QList< int > rows;
    foreach( const QListWidgetItem * item, selection )
        rows.append( row( item ) );
    m_form->setCurrentChoices( rows );
}


ComboEdit::ComboEdit( Okular::FormFieldChoice * choice, QWidget * parent )
    : QComboBox( parent ), FormWidgetIface( this, choice ), m_form( choice )
{
    addItems( m_form->choices() );
    setEditable( true );
    lineEdit()->setReadOnly( !m_form->isEditable() );
    QList< int > selectedItems = m_form->currentChoices();
    if ( selectedItems.count() == 1 && selectedItems.at(0) >= 0 && selectedItems.at(0) < count() )
        setCurrentIndex( selectedItems.at(0) );
    setEnabled( !m_form->isReadOnly() );

    connect( this, SIGNAL( currentIndexChanged( int ) ), this, SLOT( indexChanged( int ) ) );
    setVisible( m_form->isVisible() );
}

void ComboEdit::indexChanged( int index )
{
    m_form->setCurrentChoices( QList< int >() << index );
}


#include "formwidgets.moc"
