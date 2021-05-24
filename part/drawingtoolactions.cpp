/*
    SPDX-FileCopyrightText: 2015 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2015 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "drawingtoolactions.h"

#include "debug_ui.h"
#include "settings.h"

#include <KActionCollection>
#include <KLocalizedString>

#include <QAction>
#include <QIconEngine>
#include <QPainter>

class ColorAction : public QAction
{
    Q_OBJECT

public:
    explicit ColorAction(KActionCollection *parent)
        : QAction(parent)
    {
    }

    void setColor(const QColor &color)
    {
        setIcon(QIcon(new ColorActionIconEngine(color)));
    }

protected:
    class ColorActionIconEngine : public QIconEngine
    {
    public:
        explicit ColorActionIconEngine(const QColor &color)
            : m_color(color)
        {
        }

        ColorActionIconEngine(const ColorActionIconEngine &) = delete;
        ColorActionIconEngine &operator=(const ColorActionIconEngine &) = delete;

        // No one needs clone(), but it’s pure virtual
        QIconEngine *clone() const override
        {
            return nullptr;
        }

        QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
        {
            QPixmap pixmap(size);
            pixmap.fill(Qt::transparent);
            Q_ASSERT(pixmap.hasAlphaChannel());

            QPainter painter(&pixmap);
            paint(&painter, QRect(QPoint(0, 0), size), mode, state);
            return pixmap;
        }

        void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
        {
            Q_UNUSED(mode)

            // Assume that rect is square and at position (0, 0)
            int squareSize = rect.height() * 0.8;
            int squareOffset = (rect.height() - squareSize) / 2;

            painter->fillRect(squareOffset, squareOffset, squareSize, squareSize, m_color);

            if (state == QIcon::On) {
                QFont checkmarkFont = painter->font();
                checkmarkFont.setPixelSize(squareSize * 0.9);
                painter->setFont(checkmarkFont);

                const int lightness = ((m_color.red() * 299) + (m_color.green() * 587) + (m_color.blue() * 114)) / 1000;
                painter->setPen(lightness < 128 ? Qt::white : Qt::black);

                painter->drawText(QRect(squareOffset, squareOffset, squareSize, squareSize), Qt::AlignCenter, QStringLiteral("\u2713"));
            }
        }

    protected:
        QColor m_color;
    };
};

DrawingToolActions::DrawingToolActions(KActionCollection *parent)
    : QObject(parent)
{
    loadTools();
}

DrawingToolActions::~DrawingToolActions()
{
}

QList<QAction *> DrawingToolActions::actions() const
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
    QAction *action = qobject_cast<QAction *>(sender());

    if (action) {
        if (action->isChecked()) {
            for (QAction *btn : qAsConst(m_actions)) {
                if (action != btn) {
                    btn->setChecked(false);
                }
            }

            emit changeEngine(action->property("__document").value<QDomElement>());
        } else {
            emit changeEngine(QDomElement());
        }
    }
}

void DrawingToolActions::loadTools()
{
    const QStringList drawingTools = Okular::Settings::drawingTools();

    QDomDocument doc;
    QDomElement drawingDefinition = doc.createElement(QStringLiteral("drawingTools"));
    for (const QString &drawingXml : drawingTools) {
        QDomDocument entryParser;
        if (entryParser.setContent(drawingXml))
            drawingDefinition.appendChild(doc.importNode(entryParser.documentElement(), true));
        else
            qCWarning(OkularUiDebug) << "Skipping malformed quick selection XML in QuickSelectionTools setting";
    }

    // Create the AnnotationToolItems from the XML dom tree
    QDomNode drawingDescription = drawingDefinition.firstChild();
    while (drawingDescription.isElement()) {
        const QDomElement toolElement = drawingDescription.toElement();
        if (toolElement.tagName() == QLatin1String("tool")) {
            QString tooltip;
            QString width;
            QString colorStr;
            QString opacity;

            const QString name = toolElement.attribute(QStringLiteral("name"));
            if (toolElement.attribute(QStringLiteral("default"), QStringLiteral("false")) == QLatin1String("true")) {
                tooltip = i18n(name.toLatin1().constData());
            } else {
                tooltip = name;
            }

            const QDomNodeList engineNodeList = toolElement.elementsByTagName(QStringLiteral("engine"));
            if (engineNodeList.size() > 0) {
                const QDomElement engineEl = engineNodeList.item(0).toElement();
                if (engineEl.hasAttribute(QStringLiteral("color"))) {
                    colorStr = engineEl.attribute(QStringLiteral("color"));
                }

                const QDomNodeList annotationList = engineEl.elementsByTagName(QStringLiteral("annotation"));
                if (annotationList.size() > 0) {
                    const QDomElement annotationEl = annotationList.item(0).toElement();
                    if (annotationEl.hasAttribute(QStringLiteral("width"))) {
                        width = annotationEl.attribute(QStringLiteral("width"));
                        opacity = annotationEl.attribute(QStringLiteral("opacity"), QStringLiteral("1.0"));
                    }
                }
            }

            QDomDocument doc(QStringLiteral("engine"));
            QDomElement root = doc.createElement(QStringLiteral("engine"));
            root.setAttribute(QStringLiteral("color"), colorStr);
            doc.appendChild(root);
            QDomElement annElem = doc.createElement(QStringLiteral("annotation"));
            root.appendChild(annElem);
            annElem.setAttribute(QStringLiteral("type"), QStringLiteral("Ink"));
            annElem.setAttribute(QStringLiteral("color"), colorStr);
            annElem.setAttribute(QStringLiteral("width"), width);
            annElem.setAttribute(QStringLiteral("opacity"), opacity);

            const QString text = i18n("Drawing Tool: %1", tooltip);
            createToolAction(text, tooltip, colorStr, root);
        }

        drawingDescription = drawingDescription.nextSibling();
    }

    // add erasure action
    {
        QDomDocument doc(QStringLiteral("engine"));
        QDomElement root = doc.createElement(QStringLiteral("engine"));
        root.setAttribute(QStringLiteral("color"), QStringLiteral("transparent"));
        root.setAttribute(QStringLiteral("compositionMode"), QStringLiteral("clear"));
        doc.appendChild(root);
        QDomElement annElem = doc.createElement(QStringLiteral("annotation"));
        root.appendChild(annElem);
        annElem.setAttribute(QStringLiteral("type"), QStringLiteral("Ink"));
        annElem.setAttribute(QStringLiteral("color"), QStringLiteral("transparent"));
        annElem.setAttribute(QStringLiteral("width"), 20);

        KActionCollection *ac = static_cast<KActionCollection *>(parent());
        QAction *action = new QAction(ac);
        action->setText(i18n("Eraser"));
        action->setToolTip(i18n("Eraser"));
        action->setCheckable(true);
        action->setIcon(QIcon::fromTheme(QStringLiteral("draw-eraser")));
        action->setProperty("__document", QVariant::fromValue<QDomElement>(root));

        m_actions.append(action);

        ac->addAction(QStringLiteral("presentation_drawing_eraser"), action);

        connect(action, &QAction::triggered, this, &DrawingToolActions::actionTriggered);
    }
}

void DrawingToolActions::createToolAction(const QString &text, const QString &toolName, const QString &colorName, const QDomElement &root)
{
    KActionCollection *ac = static_cast<KActionCollection *>(parent());
    ColorAction *action = new ColorAction(ac);
    action->setText(text);
    action->setToolTip(toolName);
    action->setCheckable(true);
    action->setColor(QColor(colorName));
    action->setEnabled(false);

    action->setProperty("__document", QVariant::fromValue<QDomElement>(root));

    m_actions.append(action);

    ac->addAction(QStringLiteral("presentation_drawing_%1").arg(toolName), action);

    connect(action, &QAction::triggered, this, &DrawingToolActions::actionTriggered);
}

#include "drawingtoolactions.moc"
