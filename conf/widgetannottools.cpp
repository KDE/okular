/***************************************************************************
 *   Copyright (C) 2012 by Fabio D'Urso <fabiodurso@hotmail.it>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "widgetannottools.h"

#include <kdebug.h>
#include <kicon.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kpushbutton.h>

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

// Used to store tools' XML description in m_list's items
static const int ToolXmlRole = Qt::UserRole;

WidgetAnnotTools::WidgetAnnotTools( QWidget * parent )
    : QWidget( parent )
{
    QHBoxLayout *hBoxLayout = new QHBoxLayout( this );
    m_list = new QListWidget( this );
    m_list->setIconSize( QSize( 32, 32 ) );
    hBoxLayout->addWidget( m_list );

    QVBoxLayout *vBoxLayout = new QVBoxLayout();
    m_btnAdd = new KPushButton( i18n("&Add..."), this );
    m_btnAdd->setIcon( KIcon("list-add") );
    vBoxLayout->addWidget( m_btnAdd );
    m_btnRemove = new KPushButton( i18n("&Remove"), this );
    m_btnRemove->setIcon( KIcon("list-remove") );
    m_btnRemove->setEnabled( false );
    vBoxLayout->addWidget( m_btnRemove );
    m_btnMoveUp = new KPushButton( i18n("Move &Up"), this );
    m_btnMoveUp->setIcon( KIcon("arrow-up") );
    m_btnMoveUp->setEnabled( false );
    vBoxLayout->addWidget( m_btnMoveUp );
    m_btnMoveDown = new KPushButton( i18n("Move &Down"), this );
    m_btnMoveDown->setIcon( KIcon("arrow-down") );
    m_btnMoveDown->setEnabled( false );
    vBoxLayout->addWidget( m_btnMoveDown );
    vBoxLayout->addStretch();
    hBoxLayout->addLayout( vBoxLayout );

    connect( m_list, SIGNAL( currentRowChanged(int) ), this, SLOT( slotRowChanged(int) ) );
    connect( m_btnAdd, SIGNAL( clicked(bool) ), this, SLOT( slotAdd(bool) ) );
    connect( m_btnRemove, SIGNAL( clicked(bool) ), this, SLOT( slotRemove(bool) ) );
    connect( m_btnMoveUp, SIGNAL( clicked(bool) ), this, SLOT( slotMoveUp(bool) ) );
    connect( m_btnMoveDown, SIGNAL( clicked(bool) ), this, SLOT( slotMoveDown(bool) ) );
}

WidgetAnnotTools::~WidgetAnnotTools()
{
}

/* Before returning the XML strings, this functions updates the name, id and
 * shortcut properties.
 * Note: The shortcut is only assigned to the first nine tools */
QStringList WidgetAnnotTools::tools() const
{
    QStringList res;

    const int count = m_list->count();
    for ( int i = 0; i < count; ++i )
    {
        QListWidgetItem * listEntry = m_list->item(i);

        // Parse associated DOM data
        QDomDocument doc;
        doc.setContent( listEntry->data( ToolXmlRole ).value<QString>() );

        // Set name and id
        QDomElement toolElement = doc.documentElement();
        toolElement.setAttribute( "name", listEntry->text() );
        toolElement.setAttribute( "id", i+1 );

        // Remove old shortcut, if any
        QDomNode oldShortcut = toolElement.elementsByTagName( "shortcut" ).item( 0 );
        if ( oldShortcut.isElement() )
            toolElement.removeChild( oldShortcut );

        // Create new shortcut element (only the first 9 tools are assigned a shortcut key)
        if ( i < 9 )
        {
            QDomElement newShortcut = doc.createElement( "shortcut" );
            newShortcut.appendChild( doc.createTextNode(QString::number( i+1 )) );
            toolElement.appendChild( newShortcut );
        }

        // Append to output
        res << doc.toString(-1);
    }

    return res;
}

void WidgetAnnotTools::setTools(const QStringList& items)
{
    m_list->clear();

    // Parse each string and populate the list widget
    foreach ( const QString &toolXml, items )
    {
        QDomDocument entryParser;
        if ( !entryParser.setContent( toolXml ) )
        {
            kWarning() << "Skipping malformed tool XML string";
            break;
        }

        QDomElement toolElement = entryParser.documentElement();
        if ( toolElement.tagName() == "tool" )
        {
            // Create list item and attach the source XML string as data
            const QString itemText = toolElement.attribute( "name" );
            QListWidgetItem * listEntry = new QListWidgetItem( itemText, m_list );
            listEntry->setData( ToolXmlRole, qVariantFromValue(toolXml) );
        }
    }

    updateButtons();
}

void WidgetAnnotTools::updateButtons()
{
    const int row = m_list->currentRow();
    const int last = m_list->count() - 1;

    m_btnRemove->setEnabled( row != -1 );
    m_btnMoveUp->setEnabled( row > 0 );
    m_btnMoveDown->setEnabled( row != -1 && row != last );
}

void WidgetAnnotTools::slotRowChanged( int )
{
    updateButtons();
}

void WidgetAnnotTools::slotAdd( bool )
{
    KMessageBox::sorry( this, "Not implemented yet" );
}

void WidgetAnnotTools::slotRemove( bool )
{
    const int row = m_list->currentRow();
    delete m_list->takeItem(row);
    updateButtons();
    emit changed();
}

void WidgetAnnotTools::slotMoveUp( bool )
{
    const int row = m_list->currentRow();
    m_list->insertItem( row, m_list->takeItem(row-1) );
    m_list->scrollToItem( m_list->currentItem() );
    updateButtons();
    emit changed();
}

void WidgetAnnotTools::slotMoveDown( bool )
{
    const int row = m_list->currentRow();
    m_list->insertItem( row, m_list->takeItem(row+1) );
    m_list->scrollToItem( m_list->currentItem() );
    updateButtons();
    emit changed();
}

#include "moc_widgetannottools.cpp"
