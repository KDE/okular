/*
    SPDX-FileCopyrightText: 2015 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "widgetdrawingtools.h"

#include "editdrawingtooldialog.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QDebug>
#include <QDomElement>
#include <QListWidgetItem>
#include <QPainter>

// Used to store tools' XML description in m_list's items
static const int ToolXmlRole = Qt::UserRole;

static QPixmap colorDecorationFromToolDescription(const QString &toolDescription)
{
    QDomDocument doc;
    doc.setContent(toolDescription, true);
    const QDomElement toolElement = doc.documentElement();
    const QDomElement engineElement = toolElement.elementsByTagName(QStringLiteral("engine")).at(0).toElement();
    const QDomElement annotationElement = engineElement.elementsByTagName(QStringLiteral("annotation")).at(0).toElement();

    QPixmap pm(50, 20);
    pm.fill(QColor(annotationElement.attribute(QStringLiteral("color"))));

    QPainter p(&pm);
    p.setPen(Qt::black);
    p.drawRect(QRect(0, 0, pm.width() - 1, pm.height() - 1));

    return pm;
}

WidgetDrawingTools::WidgetDrawingTools(QWidget *parent)
    : WidgetConfigurationToolsBase(parent)
{
}

WidgetDrawingTools::~WidgetDrawingTools()
{
}

QStringList WidgetDrawingTools::tools() const
{
    QStringList res;

    const int count = m_list->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem *listEntry = m_list->item(i);

        // Parse associated DOM data
        QDomDocument doc;
        doc.setContent(listEntry->data(ToolXmlRole).value<QString>());

        // Append to output
        res << doc.toString(-1);
    }

    return res;
}

void WidgetDrawingTools::setTools(const QStringList &items)
{
    m_list->clear();

    // Parse each string and populate the list widget
    for (const QString &toolXml : items) {
        QDomDocument entryParser;
        if (!entryParser.setContent(toolXml)) {
            qWarning() << "Skipping malformed tool XML string";
            break;
        }

        const QDomElement toolElement = entryParser.documentElement();
        if (toolElement.tagName() == QLatin1String("tool")) {
            const QString name = toolElement.attribute(QStringLiteral("name"));
            QString itemText;
            if (toolElement.attribute(QStringLiteral("default"), QStringLiteral("false")) == QLatin1String("true")) {
                itemText = i18n(name.toLatin1().constData());
            } else {
                itemText = name;
            }

            QListWidgetItem *listEntry = new QListWidgetItem(itemText, m_list);
            listEntry->setData(ToolXmlRole, QVariant::fromValue(toolXml));
            listEntry->setData(Qt::DecorationRole, colorDecorationFromToolDescription(toolXml));
        }
    }

    updateButtons();
}

QString WidgetDrawingTools::defaultName() const
{
    int nameIndex = 1;
    bool freeNameFound = false;
    QString candidateName;
    while (!freeNameFound) {
        candidateName = i18n("Default Drawing Tool #%1", nameIndex);
        int i = 0;
        for (; i < m_list->count(); ++i) {
            QListWidgetItem *listEntry = m_list->item(i);
            if (candidateName == listEntry->text()) {
                break;
            }
        }
        freeNameFound = i == m_list->count();
        ++nameIndex;
    }
    return candidateName;
}

void WidgetDrawingTools::slotAdd()
{
    EditDrawingToolDialog dlg(QDomElement(), this);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    const QDomDocument rootDoc = dlg.toolXml();
    QDomElement toolElement = rootDoc.documentElement();

    QString itemText = dlg.name().trimmed();

    if (itemText.isEmpty()) {
        itemText = defaultName();
    }

    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *listEntry = m_list->item(i);
        if (itemText == listEntry->text()) {
            KMessageBox::information(this, i18n("There's already a tool with that name. Using a default one"), i18n("Duplicated Name"));
            itemText = defaultName();
            break;
        }
    }

    // Store name attribute only if the user specified a customized name
    toolElement.setAttribute(QStringLiteral("name"), itemText);

    // Create list entry and attach XML string as data
    const QString toolXml = rootDoc.toString(-1);
    QListWidgetItem *listEntry = new QListWidgetItem(itemText, m_list);
    listEntry->setData(ToolXmlRole, QVariant::fromValue(toolXml));
    listEntry->setData(Qt::DecorationRole, colorDecorationFromToolDescription(toolXml));

    // Select and scroll
    m_list->setCurrentItem(listEntry);
    m_list->scrollToItem(listEntry);
    updateButtons();
    Q_EMIT changed();
}

void WidgetDrawingTools::slotEdit()
{
    QListWidgetItem *listEntry = m_list->currentItem();

    QDomDocument doc;
    doc.setContent(listEntry->data(ToolXmlRole).value<QString>());
    QDomElement toolElement = doc.documentElement();

    EditDrawingToolDialog dlg(toolElement, this);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    doc = dlg.toolXml();
    toolElement = doc.documentElement();

    QString itemText = dlg.name();

    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *auxListEntry = m_list->item(i);
        if (itemText == auxListEntry->text() && auxListEntry != listEntry) {
            KMessageBox::information(this, i18n("There's already a tool with that name. Using a default one"), i18n("Duplicated Name"));
            itemText = defaultName();
            break;
        }
    }

    // Store name attribute only if the user specified a customized name
    toolElement.setAttribute(QStringLiteral("name"), itemText);

    // Edit list entry and attach XML string as data
    const QString toolXml = doc.toString(-1);
    listEntry->setText(itemText);
    listEntry->setData(ToolXmlRole, QVariant::fromValue(toolXml));
    listEntry->setData(Qt::DecorationRole, colorDecorationFromToolDescription(toolXml));

    // Select and scroll
    m_list->setCurrentItem(listEntry);
    m_list->scrollToItem(listEntry);
    updateButtons();
    Q_EMIT changed();
}
