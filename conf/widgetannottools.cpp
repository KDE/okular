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

/* Before returning the XML strings, this functions updates the id and
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

        // Set id
        QDomElement toolElement = doc.documentElement();
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
            QString itemText = toolElement.attribute( "name" );
            if ( itemText.isEmpty() )
                itemText = PageViewAnnotator::defaultToolName( toolElement );
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

    QDomDocument rootDoc = t.toolXml();
    QDomElement toolElement = rootDoc.documentElement();

    QString itemText = t.name();

    // Store name attribute only if the user specified a customized name
    if ( !itemText.isEmpty() )
        toolElement.setAttribute( "name", itemText );
    else
        itemText = PageViewAnnotator::defaultToolName( toolElement );

    // Create list entry and attach XML string as data
    QListWidgetItem * listEntry = new QListWidgetItem( itemText, m_list );
    listEntry->setData( ToolXmlRole, qVariantFromValue( rootDoc.toString(-1) ) );
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
    m_type->addItem( i18n("Pop-up Note"), QByteArray("note-linked") );
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
    return m_name->text();
}

QDomDocument NewAnnotToolDialog::toolXml() const
{
    const QByteArray toolType = m_type->itemData( m_type->currentIndex() ).toByteArray();

    QDomDocument doc;
    QDomElement toolElement = doc.createElement( "tool" );
    QDomElement engineElement = doc.createElement( "engine" );
    QDomElement annotationElement = doc.createElement( "annotation" );
    doc.appendChild( toolElement );
    toolElement.appendChild( engineElement );
    engineElement.appendChild( annotationElement );

    const QString color = m_stubann->style().color().name();
    const double opacity = m_stubann->style().opacity();
    const double width = m_stubann->style().width();

    if ( toolType == "note-linked" )
    {
        Okular::TextAnnotation * ta = static_cast<Okular::TextAnnotation*>( m_stubann );
        toolElement.setAttribute( "type", "note-linked" );
        engineElement.setAttribute( "type", "PickPoint" );
        engineElement.setAttribute( "color", color );
        engineElement.setAttribute( "hoverIcon", "tool-note" );
        annotationElement.setAttribute( "type", "Text" );
        annotationElement.setAttribute( "color", color );
        annotationElement.setAttribute( "icon", ta->textIcon() );
    }
    else if ( toolType == "note-inline" )
    {
        Okular::TextAnnotation * ta = static_cast<Okular::TextAnnotation*>( m_stubann );
        toolElement.setAttribute( "type", "note-inline" );
        engineElement.setAttribute( "type", "PickPoint" );
        engineElement.setAttribute( "color", color );
        engineElement.setAttribute( "hoverIcon", "tool-note-inline" );
        engineElement.setAttribute( "block", "true" );
        annotationElement.setAttribute( "type", "FreeText" );
        annotationElement.setAttribute( "color", color );
        // TODO: Font
    }
    else if ( toolType == "ink" )
    {
        toolElement.setAttribute( "type", "ink" );
        engineElement.setAttribute( "type", "SmoothLine" );
        engineElement.setAttribute( "color", color );
        annotationElement.setAttribute( "type", "Ink" );
        annotationElement.setAttribute( "color", color );
        annotationElement.setAttribute( "width", width );
    }
    else if ( toolType == "straight-line" )
    {
        Okular::LineAnnotation * la = static_cast<Okular::LineAnnotation*>( m_stubann );
        toolElement.setAttribute( "type", "straight-line" );
        engineElement.setAttribute( "type", "PolyLine" );
        engineElement.setAttribute( "color", color );
        engineElement.setAttribute( "points", "2" );
        annotationElement.setAttribute( "type", "Line" );
        annotationElement.setAttribute( "color", color );
        annotationElement.setAttribute( "width", width );
        if ( la->lineLeadingForwardPoint() != 0 || la->lineLeadingBackwardPoint() != 0 )
        {
            annotationElement.setAttribute( "leadFwd", la->lineLeadingForwardPoint() );
            annotationElement.setAttribute( "leadBack", la->lineLeadingBackwardPoint() );
        }
    }
    else if ( toolType == "polygon" )
    {
        Okular::LineAnnotation * la = static_cast<Okular::LineAnnotation*>( m_stubann );
        toolElement.setAttribute( "type", "polygon" );
        engineElement.setAttribute( "type", "PolyLine" );
        engineElement.setAttribute( "color", color );
        engineElement.setAttribute( "points", "-1" );
        annotationElement.setAttribute( "type", "Line" );
        annotationElement.setAttribute( "color", color );
        annotationElement.setAttribute( "width", width );
        if ( la->lineInnerColor().isValid() )
        {
            annotationElement.setAttribute( "innerColor", la->lineInnerColor().name() );
        }
    }
    else if ( toolType == "text-markup" )
    {
        Okular::HighlightAnnotation * ha = static_cast<Okular::HighlightAnnotation*>( m_stubann );

        switch ( ha->highlightType() )
        {
            case Okular::HighlightAnnotation::Highlight:
                toolElement.setAttribute( "type", "highlight" );
                annotationElement.setAttribute( "type", "Highlight" );
                break;
            case Okular::HighlightAnnotation::Squiggly:
                toolElement.setAttribute( "type", "squiggly" );
                annotationElement.setAttribute( "type", "Squiggly" );
                break;
            case Okular::HighlightAnnotation::Underline:
                toolElement.setAttribute( "type", "underline" );
                annotationElement.setAttribute( "type", "Underline" );
                break;
            case Okular::HighlightAnnotation::StrikeOut:
                toolElement.setAttribute( "type", "strikeout" );
                annotationElement.setAttribute( "type", "StrikeOut" );
                break;
        }

        engineElement.setAttribute( "type", "TextSelector" );
        engineElement.setAttribute( "color", color );
        annotationElement.setAttribute( "color", color );
    }
    else if ( toolType == "geometrical-shape" )
    {
        Okular::GeomAnnotation * ga = static_cast<Okular::GeomAnnotation*>( m_stubann );

        if ( ga->geometricalType() == Okular::GeomAnnotation::InscribedCircle )
        {
            toolElement.setAttribute( "type", "ellipse" );
            annotationElement.setAttribute( "type", "GeomCircle" );
        }
        else
        {
            toolElement.setAttribute( "type", "rectangle" );
            annotationElement.setAttribute( "type", "GeomSquare" );
        }

        engineElement.setAttribute( "type", "PickPoint" );
        engineElement.setAttribute( "color", color );
        engineElement.setAttribute( "block", "true" );
        annotationElement.setAttribute( "color", color );
        annotationElement.setAttribute( "width", width );

        if ( ga->geometricalInnerColor().isValid() )
            annotationElement.setAttribute( "innerColor", ga->geometricalInnerColor().name() );
    }
    else if ( toolType == "stamp" )
    {
        Okular::StampAnnotation * sa = static_cast<Okular::StampAnnotation*>( m_stubann );
        toolElement.setAttribute( "type", "stamp" );
        engineElement.setAttribute( "type", "PickPoint" );
        engineElement.setAttribute( "hoverIcon", sa->stampIconName() );
        engineElement.setAttribute( "size", "64" );
        engineElement.setAttribute( "block", "true" );
        annotationElement.setAttribute( "type", "Stamp" );
        annotationElement.setAttribute( "icon", sa->stampIconName() );
    }

    if ( opacity != 1 )
        annotationElement.setAttribute( "opacity", opacity );

    return doc;
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
        ta->setTextIcon( "Note" );
        ta->style().setColor( Qt::yellow );
        m_stubann = ta;
    }
    else if ( toolType == "note-inline" )
    {
        Okular::TextAnnotation * ta = new Okular::TextAnnotation();
        ta->setTextType( Okular::TextAnnotation::InPlace );
        ta->style().setColor( Qt::yellow );
        m_stubann = ta;
    }
    else if ( toolType == "ink" )
    {
        m_stubann = new Okular::InkAnnotation();
        m_stubann->style().setWidth( 2.0 );
        m_stubann->style().setColor( Qt::green );
    }
    else if ( toolType == "straight-line" )
    {
        Okular::LineAnnotation * la = new Okular::LineAnnotation();
        la->setLinePoints( QLinkedList<Okular::NormalizedPoint>() <<
                                Okular::NormalizedPoint(0,0) <<
                                Okular::NormalizedPoint(1,0) );
        la->style().setColor( QColor( 0xff, 0xe0, 0x00 ) );
        m_stubann = la;
    }
    else if ( toolType == "polygon" )
    {
        Okular::LineAnnotation * la = new Okular::LineAnnotation();
        la->setLinePoints( QLinkedList<Okular::NormalizedPoint>() <<
                                Okular::NormalizedPoint(0,0) <<
                                Okular::NormalizedPoint(1,0) <<
                                Okular::NormalizedPoint(1,1) );
        la->setLineClosed( true );
        la->style().setColor( QColor( 0x00, 0x7e, 0xee ) );
        m_stubann = la;
    }
    else if ( toolType == "text-markup" )
    {
        m_stubann = new Okular::HighlightAnnotation();
        m_stubann->style().setColor( Qt::yellow );
    }
    else if ( toolType == "geometrical-shape" )
    {
        Okular::GeomAnnotation * ga = new Okular::GeomAnnotation();
        ga->setGeometricalType( Okular::GeomAnnotation::InscribedCircle );
        ga->style().setWidth( 5.0 );
        ga->style().setColor( Qt::cyan );
        m_stubann = ga;
    }
    else if ( toolType == "stamp" )
    {
        Okular::StampAnnotation * sa = new Okular::StampAnnotation();
        sa->setStampIconName( "okular" );
        m_stubann = sa;
    }

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
    QDomDocument doc = toolXml();
    QDomElement toolElement = doc.documentElement();
    m_name->setPlaceholderText( PageViewAnnotator::defaultToolName( toolElement ) );
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
