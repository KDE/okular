/***************************************************************************
 *   Copyright (C) 2015 by Laurent Montel <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "drawingtoolselectaction.h"

#include "debug_ui.h"
#include "settings.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QPainter>
#include <QToolButton>

class ColorButton : public QToolButton
{
public:
    explicit ColorButton( QWidget *parent = Q_NULLPTR )
        : QToolButton( parent )
    {
    }

    void setColor( const QColor &color )
    {
        QPixmap pm( 25, 25 );
        pm.fill( color );

        QIcon icon;

        icon.addPixmap( pm, QIcon::Normal, QIcon::Off );

        QPixmap pmSel( pm );
        QPainter p( &pmSel );
        QFont font = p.font();
        font.setPixelSize( pmSel.height() * 0.9 );
        p.setFont( font );

        // draw check mark
        const int lightness = ((color.red() * 299) + (color.green() * 587) + (color.blue() * 114)) / 1000;
        p.setPen( lightness < 128 ? Qt::white : Qt::black );
        p.drawText( QRect( QPoint( 0, 0 ), pmSel.size() ), Qt::AlignCenter, "\u2713" );

        icon.addPixmap( pmSel, QIcon::Normal, QIcon::On );

        setIcon( icon );
    }
};

DrawingToolSelectAction::DrawingToolSelectAction( QObject *parent )
    : QWidgetAction( parent )
{
    QWidget *mainWidget = new QWidget;
    m_layout = new QHBoxLayout( mainWidget );
    m_layout->setContentsMargins( 0, 0, 0, 0 );

    loadTools();

    setDefaultWidget( mainWidget );
}


DrawingToolSelectAction::~DrawingToolSelectAction()
{
}

void DrawingToolSelectAction::toolButtonClicked()
{
    QAbstractButton *button = qobject_cast<QAbstractButton*>( sender() );

    if ( button ) {
        if ( button->isChecked() ) {
            Q_FOREACH ( QAbstractButton *btn, m_buttons )
            {
                if ( button != btn ) {
                    btn->setChecked( false );
                }
            }

            emit changeEngine( button->property( "__document" ).value<QDomElement>() );
        } else {
            emit changeEngine( QDomElement() );
        }
    }
}

void DrawingToolSelectAction::loadTools()
{
    const QStringList drawingTools = Okular::Settings::drawingTools();

    QDomDocument doc;
    QDomElement drawingDefinition = doc.createElement( "drawingTools" );
    foreach ( const QString &drawingXml, drawingTools )
    {
        QDomDocument entryParser;
        if ( entryParser.setContent( drawingXml ) )
            drawingDefinition.appendChild( doc.importNode( entryParser.documentElement(), true ) );
        else
            qCWarning(OkularUiDebug) << "Skipping malformed quick selection XML in QuickSelectionTools setting";
    }

    int shortcutCounter = 0;

    // Create the AnnotationToolItems from the XML dom tree
    QDomNode drawingDescription = drawingDefinition.firstChild();
    while ( drawingDescription.isElement() )
    {
        const QDomElement toolElement = drawingDescription.toElement();
        if ( toolElement.tagName() == "tool" )
        {
            QString width;
            QString colorStr;
            QString opacity;

            const QDomNodeList engineNodeList = toolElement.elementsByTagName( "engine" );
            if ( engineNodeList.size() > 0 )
            {
                const QDomElement engineEl = engineNodeList.item( 0 ).toElement();
                if ( engineEl.hasAttribute( QStringLiteral("color") ) )
                {
                    colorStr = engineEl.attribute( QStringLiteral("color") );
                }

                const QDomNodeList annotationList = engineEl.elementsByTagName( "annotation" );
                if ( annotationList.size() > 0 )
                {
                    const QDomElement annotationEl = annotationList.item( 0 ).toElement();
                    if ( annotationEl.hasAttribute( QStringLiteral("width") ) )
                    {
                        width = annotationEl.attribute( QStringLiteral("width") );
                        opacity = annotationEl.attribute( QStringLiteral("opacity"), QStringLiteral("1.0") );
                    }
                }
            }

            QDomDocument doc( QStringLiteral("engine") );
            QDomElement root = doc.createElement( QStringLiteral("engine") );
            root.setAttribute( QStringLiteral("color"), colorStr );
            doc.appendChild( root );
            QDomElement annElem = doc.createElement( QStringLiteral("annotation") );
            root.appendChild( annElem );
            annElem.setAttribute( QStringLiteral("type"), QStringLiteral("Ink") );
            annElem.setAttribute( QStringLiteral("color"), colorStr );
            annElem.setAttribute( QStringLiteral("width"), width );
            annElem.setAttribute( QStringLiteral("opacity"), opacity );

            const QString description = i18n("Toggle Drawing Tool:\n  color: %1\n  pen width: %2\n  opacity: %3%", colorStr, width, opacity.toDouble() * 100);

            shortcutCounter++;
            const QString shortcut = (shortcutCounter < 10 ? i18n( "Ctrl+%1", shortcutCounter ) :
                                      shortcutCounter == 10 ? i18n( "Ctrl+0" ) :
                                      QString());

            createToolButton( description, colorStr, root, shortcut );
        }

        drawingDescription = drawingDescription.nextSibling();
    }
}

void DrawingToolSelectAction::createToolButton( const QString &description, const QString &colorName, const QDomElement &root, const QString &shortcut )
{
    ColorButton *button = new ColorButton;
    button->setToolTip( description );
    button->setCheckable( true );
    button->setColor( QColor( colorName ) );

    if ( !shortcut.isEmpty() )
        button->setShortcut( QKeySequence( shortcut ) );

    button->setProperty( "__document", QVariant::fromValue<QDomElement>( root ) );

    m_buttons.append( button );
    m_layout->addWidget( button );

    connect( button, SIGNAL(clicked()), SLOT(toolButtonClicked()) );
}
