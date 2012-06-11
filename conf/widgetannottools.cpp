/***************************************************************************
 *   Copyright (C) 2012 by Fabio D'Urso <fabiodurso@hotmail.it>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "widgetannottools.h"

#include <kcolorbutton.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kicon.h>
#include <klocalizedstring.h>
#include <klineedit.h>
#include <knuminput.h>
#include <kpushbutton.h>

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>
#include <QtGui/QStackedWidget>
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
    NewAnnotToolDialog t( this );

    if ( t.exec() != QDialog::Accepted )
        return;

    // Create list entry and attach XML string as data
    QListWidgetItem * listEntry = new QListWidgetItem( t.name(), m_list );
    listEntry->setData( ToolXmlRole, qVariantFromValue( t.toolXml() ) );

    // Select and scroll
    m_list->setCurrentItem( listEntry );
    m_list->scrollToItem( listEntry );
    updateButtons();
    emit changed();
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

NewAnnotToolDialog::NewAnnotToolDialog( QWidget *parent )
    : KDialog( parent )
{
    setCaption( i18n("Create annotation tool") );
    setButtons( Ok | Cancel );
    enableButton( Ok, false );
    setDefaultButton( Ok );

    QLabel * tmplabel;
    QWidget *widget = new QWidget( this );
    QGridLayout * widgetLayout = new QGridLayout( widget );

    setMainWidget(widget);

    m_name = new KLineEdit( widget );
    connect( m_name, SIGNAL( textEdited(const QString &) ), this, SLOT( slotNameEdited(const QString &) ) );
    tmplabel = new QLabel( i18n( "&Name:" ), widget );
    tmplabel->setBuddy( m_name );
    widgetLayout->addWidget( tmplabel, 0, 0, Qt::AlignRight );
    widgetLayout->addWidget( m_name, 0, 1 );

    m_type = new KComboBox( false, widget );
    tmplabel = new QLabel( i18n( "&Type:" ), widget );
    tmplabel->setBuddy( m_type );
    widgetLayout->addWidget( tmplabel, 1, 0, Qt::AlignRight );
    widgetLayout->addWidget( m_type, 1, 1 );

    QGroupBox * appearance = new QGroupBox( i18n( "Appearance" ), widget );
    QGridLayout * appearanceLayout = new QGridLayout( appearance );

    m_color = new KColorButton( appearance );
    tmplabel = new QLabel( i18n( "&Color:" ), appearance );
    m_color->setColor( Qt::green );
    tmplabel->setBuddy( m_color );
    appearanceLayout->addWidget( tmplabel, 0, 0, Qt::AlignRight );
    appearanceLayout->addWidget( m_color, 0, 1 );

    m_opacity = new KIntNumInput( appearance );
    m_opacity->setRange( 0, 100 );
    m_opacity->setValue( 100 );
    m_opacity->setSuffix( i18nc( "Suffix for the opacity level, eg '80 %'", " %" ) );
    tmplabel = new QLabel( i18n( "&Opacity:" ), appearance );
    tmplabel->setBuddy( m_opacity );
    appearanceLayout->addWidget( tmplabel, 1, 0, Qt::AlignRight );
    appearanceLayout->addWidget( m_opacity, 1, 1 );

    widgetLayout->addWidget( appearance, 2, 0, 1, 2 );

#define TYPE(name, template) \
    m_type->addItem( name, qVariantFromValue(QString( template )) )

    TYPE( i18n("Note"),
            "<tool type=\"note-linked\">"
             "<engine type=\"PickPoint\" color=\"%1\" hoverIcon=\"tool-note\">"
              "<annotation type=\"Text\" color=\"%1\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    TYPE( i18n("Inline Note" ),
            "<tool type=\"note-inline\">"
             "<engine type=\"PickPoint\" color=\"%1\" hoverIcon=\"tool-note-inline\" block=\"true\">"
              "<annotation type=\"FreeText\" color=\"%1\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    TYPE( i18n("Freehand Line" ),
            "<tool type=\"ink\">"
             "<engine type=\"SmoothLine\" color=\"%1\">"
              "<annotation type=\"Ink\" color=\"%1\" width=\"2\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    TYPE( i18n("Straight Line" ),
            "<tool type=\"straight-line\">"
             "<engine type=\"PolyLine\" color=\"%1\" points=\"2\">"
              "<annotation type=\"Line\" width=\"4\" color=\"%1\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    TYPE( i18n("Polygon" ),
            "<tool type=\"polygon\">"
             "<engine type=\"PolyLine\" color=\"%1\" points=\"-1\">"
              "<annotation type=\"Line\" width=\"4\" color=\"%1\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    TYPE( i18n("Highlight" ),
            "<tool type=\"highlight\">"
             "<engine type=\"TextSelector\" color=\"%1\">"
              "<annotation type=\"Highlight\" color=\"%1\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    TYPE( i18n("Squiggly" ),
            "<tool type=\"squiggly\">"
             "<engine type=\"TextSelector\" color=\"%1\">"
              "<annotation type=\"Squiggly\" color=\"%1\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    TYPE( i18n("Underline" ),
            "<tool type=\"underline\">"
             "<engine type=\"TextSelector\" color=\"%1\">"
              "<annotation type=\"Underline\" color=\"%1\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    TYPE( i18n("Strike out" ),
            "<tool type=\"strikeout\">"
             "<engine type=\"TextSelector\" color=\"%1\">"
              "<annotation type=\"StrikeOut\" color=\"%1\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    TYPE( i18n("Ellipse" ),
            "<tool type=\"ellipse\">"
             "<engine type=\"PickPoint\" color=\"%1\" block=\"true\">"
              "<annotation type=\"GeomCircle\" color=\"%1\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    TYPE( i18n("Rectangle" ),
            "<tool type=\"rectangle\">"
             "<engine type=\"PickPoint\" color=\"%1\" block=\"true\">"
              "<annotation type=\"GeomSquare\" color=\"%1\" opacity=\"%2\" />"
             "</engine>"
            "</tool>" );
    // TODO: Stamp
#undef ADDTYPE
}

QString NewAnnotToolDialog::name() const
{
    return m_name->text();
}

QString NewAnnotToolDialog::toolXml() const
{
    const QString templ = m_type->itemData( m_type->currentIndex() ).value<QString>();
    const double opacity = (double)m_opacity->value() / 100.0;
    return templ.arg( m_color->color().name() ).arg( opacity );
}

void NewAnnotToolDialog::slotNameEdited( const QString &new_name )
{
    enableButton( Ok, !new_name.isEmpty() );
}

#include "moc_widgetannottools.cpp"
