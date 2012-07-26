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

#include "core/annotations.h"
#include "ui/annotationwidgets.h"
#include "ui/pageviewannotator.h"

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
            listEntry->setIcon( PageViewAnnotator::makeToolPixmap( toolElement ) );
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

    QDomDocument entryParser;
    entryParser.setContent( t.toolXml() );
    QDomElement toolElement = entryParser.documentElement();

    // Create list entry and attach XML string as data
    QListWidgetItem * listEntry = new QListWidgetItem( t.name(), m_list );
    listEntry->setData( ToolXmlRole, qVariantFromValue( entryParser.toString(-1) ) );
    listEntry->setIcon( PageViewAnnotator::makeToolPixmap( toolElement ) );

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
    : KDialog( parent ), m_stubann( 0 ), m_annotationWidget( 0 )
{
    setCaption( i18n("Create annotation tool") );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );

    QLabel * tmplabel;
    QWidget *widget = new QWidget( this );
    QGridLayout * widgetLayout = new QGridLayout( widget );

    setMainWidget(widget);

    m_name = new KLineEdit( widget );
    tmplabel = new QLabel( i18n( "&Name:" ), widget );
    tmplabel->setBuddy( m_name );
    widgetLayout->addWidget( tmplabel, 0, 0, Qt::AlignRight );
    widgetLayout->addWidget( m_name, 0, 1 );

    m_type = new KComboBox( false, widget );
    connect( m_type, SIGNAL( currentIndexChanged(int) ), this, SLOT( slotTypeChanged() ) );
    tmplabel = new QLabel( i18n( "&Type:" ), widget );
    tmplabel->setBuddy( m_type );
    widgetLayout->addWidget( tmplabel, 1, 0, Qt::AlignRight );
    widgetLayout->addWidget( m_type, 1, 1 );

    m_appearanceBox = new QGroupBox( i18n( "Appearance" ), widget );
    m_appearanceBox->setLayout( new QVBoxLayout( m_appearanceBox ) );
    widgetLayout->addWidget( m_appearanceBox, 2, 0, 1, 2 );

    // Populate combobox with annotation types
    m_type->addItem( i18n("Note"), QByteArray("note-linked") );
    m_type->addItem( i18n("Inline Note"), QByteArray("note-inline") );
    m_type->addItem( i18n("Freehand Line"), QByteArray("ink") );
    m_type->addItem( i18n("Straight Line"), QByteArray("straight-line") );
    m_type->addItem( i18n("Polygon"), QByteArray("polygon") );
    m_type->addItem( i18n("Text markup"), QByteArray("text-markup") );
    m_type->addItem( i18n("Geometrical shape"), QByteArray("geometrical-shape") );
    m_type->addItem( i18n("Stamp"), QByteArray("stamp") );

    rebuildAppearanceBox();
    updateDefaultName();
}

NewAnnotToolDialog::~NewAnnotToolDialog()
{
    delete m_annotationWidget;
}

QString NewAnnotToolDialog::name() const
{
    QString userText = m_name->text();
    if ( userText.isEmpty() )
        userText = m_name->placeholderText();
    return userText;
}

QString NewAnnotToolDialog::toolXml() const
{
    const QByteArray toolType = m_type->itemData( m_type->currentIndex() ).toByteArray();
    const QString color = m_stubann->style().color().name();
    const double opacity = m_stubann->style().opacity();
    const double width = m_stubann->style().width();

    if ( toolType == "note-linked" )
    {
        Okular::TextAnnotation * ta = static_cast<Okular::TextAnnotation*>( m_stubann );
        return QString( "<tool type=\"note-linked\">"
                         "<engine type=\"PickPoint\" color=\"%1\" hoverIcon=\"tool-note\">"
                          "<annotation type=\"Text\" color=\"%1\" opacity=\"%2\" icon=\"%3\" />"
                         "</engine>"
                        "</tool>" ).arg( color ).arg( opacity ).arg( ta->textIcon() );
    }
    else if ( toolType == "note-inline" )
    {
        Okular::TextAnnotation * ta = static_cast<Okular::TextAnnotation*>( m_stubann );
        Q_UNUSED( ta );
        return QString( "<tool type=\"note-inline\">"
                         "<engine type=\"PickPoint\" color=\"%1\" hoverIcon=\"tool-note-inline\" block=\"true\">"
                          "<annotation type=\"FreeText\" color=\"%1\" opacity=\"%2\" />"
                         "</engine>"
                        "</tool>" ).arg( color ).arg( opacity );
    }
    else if ( toolType == "ink" )
    {
        Okular::InkAnnotation * ia = static_cast<Okular::InkAnnotation*>( m_stubann );
        Q_UNUSED( ia );
        return QString( "<tool type=\"ink\">"
                         "<engine type=\"SmoothLine\" color=\"%1\">"
                          "<annotation type=\"Ink\" color=\"%1\" opacity=\"%2\" width=\"%3\" />"
                         "</engine>"
                        "</tool>").arg( color ).arg( opacity ).arg( width );
    }
    else if ( toolType == "straight-line" )
    {
        Okular::LineAnnotation * la = static_cast<Okular::LineAnnotation*>( m_stubann );
        Q_UNUSED( la );
        return QString( "<tool type=\"straight-line\">"
                         "<engine type=\"PolyLine\" color=\"%1\" points=\"2\">"
                          "<annotation type=\"Line\" color=\"%1\" opacity=\"%2\" width=\"%3\" />"
                         "</engine>"
                        "</tool>" ).arg( color ).arg( opacity ).arg( width );
    }
    else if ( toolType == "polygon" )
    {
        Okular::LineAnnotation * la = static_cast<Okular::LineAnnotation*>( m_stubann );
        Q_UNUSED( la );
        return QString( "<tool type=\"polygon\">"
                         "<engine type=\"PolyLine\" color=\"%1\" points=\"-1\">"
                          "<annotation type=\"Line\" color=\"%1\" opacity=\"%2\" width=\"%3\" />"
                         "</engine>"
                        "</tool>" ).arg( color ).arg( opacity ).arg( width );
    }
    else if ( toolType == "text-markup" )
    {
        Okular::HighlightAnnotation * ha = static_cast<Okular::HighlightAnnotation*>( m_stubann );

        QString toolType, annotationType;
        switch ( ha->highlightType() )
        {
            case Okular::HighlightAnnotation::Highlight:
                toolType = "highlight";
                annotationType = "Highlight";
                break;
            case Okular::HighlightAnnotation::Squiggly:
                toolType = "squiggly";
                annotationType = "Squiggly";
                break;
            case Okular::HighlightAnnotation::Underline:
                toolType = "underline";
                annotationType = "Underline";
                break;
            case Okular::HighlightAnnotation::StrikeOut:
                toolType = "strikeout";
                annotationType = "StrikeOut";
                break;
        }

        return QString( "<tool type=\"%3\">"
                         "<engine type=\"TextSelector\" color=\"%1\">"
                          "<annotation type=\"%4\" color=\"%1\" opacity=\"%2\" />"
                         "</engine>"
                        "</tool>").arg( color ).arg( opacity )
                                  .arg( toolType ).arg( annotationType );
    }
    else if ( toolType == "geometrical-shape" )
    {
        Okular::GeomAnnotation * ga = static_cast<Okular::GeomAnnotation*>( m_stubann );
        const bool isCircle = (ga->geometricalType() == Okular::GeomAnnotation::InscribedCircle);
        return QString( "<tool type=\"%4\">"
                         "<engine type=\"PickPoint\" color=\"%1\" block=\"true\">"
                          "<annotation type=\"%5\" color=\"%1\" opacity=\"%2\" width=\"%3\" />"
                         "</engine>"
                        "</tool>").arg( color ).arg( opacity ).arg( width )
                                  .arg( isCircle ? "ellipse" : "rectangle" )
                                  .arg( isCircle ? "GeomCircle" : "GeomSquare" );
    }
    else if ( toolType == "stamp" )
    {
        Okular::StampAnnotation * sa = static_cast<Okular::StampAnnotation*>( m_stubann );
        return QString( "<tool type=\"stamp\">"
                         "<engine type=\"PickPoint\" hoverIcon=\"%1\" size=\"64\" block=\"true\">"
                          "<annotation type=\"Stamp\" icon=\"%1\"/>"
                         "</engine>"
                        "</tool>" ).arg( sa->stampIconName() ); // FIXME: Check bad strings
    }

    return QString(); // This should never happen
}

void NewAnnotToolDialog::rebuildAppearanceBox()
{
    const QByteArray toolType = m_type->itemData( m_type->currentIndex() ).toByteArray();

    // Delete previous stub annotation, if any
    delete m_stubann;

    // Create stub annotation
    if ( toolType == "note-linked" )
    {
        Okular::TextAnnotation * ta = new Okular::TextAnnotation();
        ta->setTextType( Okular::TextAnnotation::Linked );
        m_stubann = ta;
    }
    else if ( toolType == "note-inline" )
    {
        Okular::TextAnnotation * ta = new Okular::TextAnnotation();
        ta->setTextType( Okular::TextAnnotation::InPlace );
        m_stubann = ta;
    }
    else if ( toolType == "ink" )
    {
        m_stubann = new Okular::InkAnnotation();
    }
    else if ( toolType == "straight-line" )
    {
        Okular::LineAnnotation * la = new Okular::LineAnnotation();
        la->setLinePoints( QLinkedList<Okular::NormalizedPoint>() <<
                                Okular::NormalizedPoint(0,0) <<
                                Okular::NormalizedPoint(1,0) );
        m_stubann = la;
    }
    else if ( toolType == "polygon" )
    {
        Okular::LineAnnotation * la = new Okular::LineAnnotation();
        la->setLinePoints( QLinkedList<Okular::NormalizedPoint>() <<
                                Okular::NormalizedPoint(0,0) <<
                                Okular::NormalizedPoint(1,0) <<
                                Okular::NormalizedPoint(1,1) );
        m_stubann = la;
    }
    else if ( toolType == "text-markup" )
    {
        m_stubann = new Okular::HighlightAnnotation();
    }
    else if ( toolType == "geometrical-shape" )
    {
        m_stubann = new Okular::GeomAnnotation();
    }
    else if ( toolType == "stamp" )
    {
        Okular::StampAnnotation * sa = new Okular::StampAnnotation();
        sa->setStampIconName( "okular" );
        m_stubann = sa;
    }

    m_stubann->style().setColor( Qt::yellow ); // TODO: Choose default color according to annotation type

    // Remove previous widget (if any)
    if ( m_annotationWidget )
    {
        delete m_annotationWidget->appearanceWidget();
        delete m_annotationWidget;
    }

    m_annotationWidget = AnnotationWidgetFactory::widgetFor( m_stubann );
    m_appearanceBox->layout()->addWidget( m_annotationWidget->appearanceWidget() );

    connect( m_annotationWidget, SIGNAL(dataChanged()), this, SLOT(slotDataChanged()) );
}

void NewAnnotToolDialog::updateDefaultName()
{
    const QByteArray toolType = m_type->itemData( m_type->currentIndex() ).toByteArray();
    QString defaultName = m_type->currentText();

    if ( toolType == "text-markup" )
    {
        Okular::HighlightAnnotation * ha = static_cast<Okular::HighlightAnnotation*>( m_stubann );

        switch ( ha->highlightType() )
        {
            case Okular::HighlightAnnotation::Highlight:
                defaultName = i18n( "Highlight" );
                break;
            case Okular::HighlightAnnotation::Squiggly:
                defaultName = i18n( "Squiggly" );
                break;
            case Okular::HighlightAnnotation::Underline:
                defaultName = i18n( "Underline" );
                break;
            case Okular::HighlightAnnotation::StrikeOut:
                defaultName = i18n( "Strike out" );
                break;
        }
    }
    else if ( toolType == "geometrical-shape" )
    {
        Okular::GeomAnnotation * ga = static_cast<Okular::GeomAnnotation*>( m_stubann );

        if ( ga->geometricalType() == Okular::GeomAnnotation::InscribedCircle )
            defaultName = i18n( "Ellipse" );
        else
            defaultName = i18n( "Rectangle" );
    }

    m_name->setPlaceholderText( defaultName );
}

void NewAnnotToolDialog::slotTypeChanged()
{
    rebuildAppearanceBox();
    updateDefaultName();
}

void NewAnnotToolDialog::slotDataChanged()
{
    // Mirror changes back in the stub annotation
    m_annotationWidget->applyChanges();

    updateDefaultName();
}

#include "moc_widgetannottools.cpp"
