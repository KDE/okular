/***************************************************************************
 *   Copyright (C) 2015 by Laurent Montel <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "widgetdrawingtools.h"

#include "editdrawingtooldialog.h"

#include <QDebug>
#include <QDomElement>
#include <QListWidgetItem>
#include <QPainter>

// Used to store tools' XML description in m_list's items
static const int ToolXmlRole = Qt::UserRole;

static QPixmap colorDecorationFromToolDescription( const QString &toolDescription )
{
    QDomDocument doc;
    doc.setContent( toolDescription, true );
    const QDomElement toolElement = doc.documentElement();
    const QDomElement engineElement = toolElement.elementsByTagName( QStringLiteral( "engine" ) ).at( 0 ).toElement();
    const QDomElement annotationElement = engineElement.elementsByTagName( QStringLiteral( "annotation" ) ).at( 0 ).toElement();

    QPixmap pm( 50, 20 );
    pm.fill( QColor( annotationElement.attribute( QStringLiteral( "color" ) ) ) );

    QPainter p( &pm );
    p.setPen( Qt::black );
    p.drawRect( QRect( 0, 0, pm.width() - 1, pm.height() - 1 ) );

    return pm;
};

WidgetDrawingTools::WidgetDrawingTools( QWidget *parent )
    : WidgetConfigurationToolsBase( parent )
{
}

WidgetDrawingTools::~WidgetDrawingTools()
{

}

QStringList WidgetDrawingTools::tools() const
{
    QStringList res;

    const int count = m_list->count();
    for ( int i = 0; i < count; ++i )
    {
        QListWidgetItem * listEntry = m_list->item( i );

        // Parse associated DOM data
        QDomDocument doc;
        doc.setContent( listEntry->data( ToolXmlRole ).value<QString>() );

        // Append to output
        res << doc.toString( -1 );
    }

    return res;
}

void WidgetDrawingTools::setTools( const QStringList &items )
{
    m_list->clear();

    // Parse each string and populate the list widget
    foreach ( const QString &toolXml, items )
    {
        QDomDocument entryParser;
        if ( !entryParser.setContent( toolXml ) )
        {
            qWarning() << "Skipping malformed tool XML string";
            break;
        }

        const QDomElement toolElement = entryParser.documentElement();
        if ( toolElement.tagName() == QLatin1String("tool") )
        {
            QListWidgetItem * listEntry = new QListWidgetItem( m_list );
            listEntry->setData( ToolXmlRole, qVariantFromValue( toolXml ) );
            listEntry->setData( Qt::DecorationRole, colorDecorationFromToolDescription( toolXml ) );
        }
    }

    updateButtons();
}

void WidgetDrawingTools::slotAdd()
{
    EditDrawingToolDialog dlg( QDomElement(), this );

    if ( dlg.exec() != QDialog::Accepted )
        return;

    const QDomDocument rootDoc = dlg.toolXml();
    const QDomElement toolElement = rootDoc.documentElement();

    // Create list entry and attach XML string as data
    const QString toolXml = rootDoc.toString( -1 );
    QListWidgetItem * listEntry = new QListWidgetItem( m_list );
    listEntry->setData( ToolXmlRole, qVariantFromValue( toolXml ) );
    listEntry->setData( Qt::DecorationRole, colorDecorationFromToolDescription( toolXml ) );

    // Select and scroll
    m_list->setCurrentItem( listEntry );
    m_list->scrollToItem( listEntry );
    updateButtons();
    emit changed();
}

void WidgetDrawingTools::slotEdit()
{
    QListWidgetItem *listEntry = m_list->currentItem();

    QDomDocument doc;
    doc.setContent( listEntry->data( ToolXmlRole ).value<QString>() );
    const QDomElement toolElement = doc.documentElement();

    EditDrawingToolDialog dlg( toolElement, this );

    if ( dlg.exec() != QDialog::Accepted )
        return;

    doc = dlg.toolXml();

    // Edit list entry and attach XML string as data
    const QString toolXml = doc.toString( -1 );
    listEntry->setData( ToolXmlRole, qVariantFromValue( toolXml ) );
    listEntry->setData( Qt::DecorationRole, colorDecorationFromToolDescription( toolXml ) );

    // Select and scroll
    m_list->setCurrentItem( listEntry );
    m_list->scrollToItem( listEntry );
    updateButtons();
    emit changed();
}
