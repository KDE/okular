/***************************************************************************
 *   Copyright (C) 2015 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2015 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "drawingtoolactions.h"

#include "debug_ui.h"
#include "settings.h"

#include <KActionCollection>
#include <KLocalizedString>

#include <QAction>
#include <QPainter>

class ColorAction : public QAction
{
    Q_OBJECT

public:
    explicit ColorAction( KActionCollection *parent )
        : QAction( parent )
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
        p.drawText( QRect( QPoint( 0, 0 ), pmSel.size() ), Qt::AlignCenter, QStringLiteral("\u2713") );

        icon.addPixmap( pmSel, QIcon::Normal, QIcon::On );

        setIcon( icon );
    }
};

DrawingToolActions::DrawingToolActions( KActionCollection *parent )
    : QObject( parent )
{
    loadTools();
}


DrawingToolActions::~DrawingToolActions()
{
}

QList<QAction*> DrawingToolActions::actions() const
{
    return m_actions;
}

void DrawingToolActions::reparseConfig()
{
    qDeleteAll(m_actions);
    m_actions.clear();
    loadTools();
}

void DrawingToolActions::actionTriggered()
{
    QAction *action = qobject_cast<QAction*>( sender() );

    if ( action ) {
        if ( action->isChecked() ) {
            Q_FOREACH ( QAction *btn, m_actions )
            {
                if ( action != btn ) {
                    btn->setChecked( false );
                }
            }

            emit changeEngine( action->property( "__document" ).value<QDomElement>() );
        } else {
            emit changeEngine( QDomElement() );
        }
    }
}

void DrawingToolActions::loadTools()
{
    const QStringList drawingTools = Okular::Settings::drawingTools();

    QDomDocument doc;
    QDomElement drawingDefinition = doc.createElement( QStringLiteral("drawingTools") );
    foreach ( const QString &drawingXml, drawingTools )
    {
        QDomDocument entryParser;
        if ( entryParser.setContent( drawingXml ) )
            drawingDefinition.appendChild( doc.importNode( entryParser.documentElement(), true ) );
        else
            qCWarning(OkularUiDebug) << "Skipping malformed quick selection XML in QuickSelectionTools setting";
    }

    // Create the AnnotationToolItems from the XML dom tree
    QDomNode drawingDescription = drawingDefinition.firstChild();
    while ( drawingDescription.isElement() )
    {
        const QDomElement toolElement = drawingDescription.toElement();
        if ( toolElement.tagName() == QLatin1String("tool") )
        {
            QString tooltip;
            QString width;
            QString colorStr;
            QString opacity;

            const QString name = toolElement.attribute( QStringLiteral("name") );
            if ( toolElement.attribute( QStringLiteral("default"), QStringLiteral("false") ) == QLatin1String("true") )
            {
                tooltip = i18n( name.toLatin1().constData() );   
            }
            else
            {
                tooltip = name;
            }

            const QDomNodeList engineNodeList = toolElement.elementsByTagName( QStringLiteral("engine") );
            if ( engineNodeList.size() > 0 )
            {
                const QDomElement engineEl = engineNodeList.item( 0 ).toElement();
                if ( engineEl.hasAttribute( QStringLiteral("color") ) )
                {
                    colorStr = engineEl.attribute( QStringLiteral("color") );
                }

                const QDomNodeList annotationList = engineEl.elementsByTagName( QStringLiteral("annotation") );
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

            const QString text = i18n("Drawing Tool: %1", tooltip);
            createToolAction( text, tooltip, colorStr, root );
        }

        drawingDescription = drawingDescription.nextSibling();
    }

    // add erasure action
    {
        QDomDocument doc( QStringLiteral("engine") );
        QDomElement root = doc.createElement( QStringLiteral("engine") );
        root.setAttribute( QStringLiteral("color"), QStringLiteral("transparent") );
        root.setAttribute( QStringLiteral("compositionMode"), QStringLiteral("clear") );
        doc.appendChild( root );
        QDomElement annElem = doc.createElement( QStringLiteral("annotation") );
        root.appendChild( annElem );
        annElem.setAttribute( QStringLiteral("type"), QStringLiteral("Ink") );
        annElem.setAttribute( QStringLiteral("color"), QStringLiteral("transparent") );
        annElem.setAttribute( QStringLiteral("width"), 20 );

        KActionCollection *ac = static_cast<KActionCollection*>( parent() );
        QAction *action = new QAction( ac );
        action->setText( i18n("Eraser") );
        action->setToolTip( i18n("Eraser") );
        action->setCheckable( true );
        action->setIcon( QIcon::fromTheme( QStringLiteral("draw-eraser") ) );
        action->setProperty( "__document", QVariant::fromValue<QDomElement>( root ) );

        m_actions.append( action );

        ac->addAction( QStringLiteral("presentation_drawing_eraser"), action );

        connect( action, &QAction::triggered, this, &DrawingToolActions::actionTriggered );
    }
}

void DrawingToolActions::createToolAction( const QString &text, const QString &toolName, const QString &colorName, const QDomElement &root )
{
    KActionCollection *ac = static_cast<KActionCollection*>( parent() );
    ColorAction *action = new ColorAction( ac );
    action->setText( text );
    action->setToolTip( toolName );
    action->setCheckable( true );
    action->setColor( QColor( colorName ) );
    action->setEnabled( false );

    action->setProperty( "__document", QVariant::fromValue<QDomElement>( root ) );

    m_actions.append( action );

    ac->addAction( QStringLiteral("presentation_drawing_%1").arg( toolName ), action );

    connect( action, &QAction::triggered, this, &DrawingToolActions::actionTriggered );
}

#include "drawingtoolactions.moc"
