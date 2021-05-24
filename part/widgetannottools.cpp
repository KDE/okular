/*
    SPDX-FileCopyrightText: 2012 Fabio D 'Urso <fabiodurso@hotmail.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "widgetannottools.h"
#include "editannottooldialog.h"

#include <KLocalizedString>
#include <QIcon>

#include <KConfigGroup>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QDomElement>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

#include "pageviewannotator.h"

// Used to store tools' XML description in m_list's items
static const int ToolXmlRole = Qt::UserRole;

WidgetAnnotTools::WidgetAnnotTools(QWidget *parent)
    : WidgetConfigurationToolsBase(parent)
{
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
    for (int i = 0; i < count; ++i) {
        QListWidgetItem *listEntry = m_list->item(i);

        // Parse associated DOM data
        QDomDocument doc;
        doc.setContent(listEntry->data(ToolXmlRole).value<QString>());

        // Set id
        QDomElement toolElement = doc.documentElement();
        toolElement.setAttribute(QStringLiteral("id"), i + 1);

        // Remove old shortcut, if any
        QDomNode oldShortcut = toolElement.elementsByTagName(QStringLiteral("shortcut")).item(0);
        if (oldShortcut.isElement())
            toolElement.removeChild(oldShortcut);

        // Create new shortcut element (only the first 9 tools are assigned a shortcut key)
        if (i < 9) {
            QDomElement newShortcut = doc.createElement(QStringLiteral("shortcut"));
            newShortcut.appendChild(doc.createTextNode(QString::number(i + 1)));
            toolElement.appendChild(newShortcut);
        }

        // Append to output
        res << doc.toString(-1);
    }

    return res;
}

void WidgetAnnotTools::setTools(const QStringList &items)
{
    m_list->clear();

    // Parse each string and populate the list widget
    for (const QString &toolXml : items) {
        QDomDocument entryParser;
        if (!entryParser.setContent(toolXml)) {
            qWarning() << "Skipping malformed tool XML string";
            break;
        }

        QDomElement toolElement = entryParser.documentElement();
        if (toolElement.tagName() == QLatin1String("tool")) {
            // Create list item and attach the source XML string as data
            QString itemText = toolElement.attribute(QStringLiteral("name"));
            if (itemText.isEmpty())
                itemText = PageViewAnnotator::defaultToolName(toolElement);
            QListWidgetItem *listEntry = new QListWidgetItem(itemText, m_list);
            listEntry->setData(ToolXmlRole, QVariant::fromValue(toolXml));
            listEntry->setIcon(PageViewAnnotator::makeToolPixmap(toolElement));
        }
    }

    updateButtons();
}

void WidgetAnnotTools::slotEdit()
{
    QListWidgetItem *listEntry = m_list->currentItem();

    QDomDocument doc;
    doc.setContent(listEntry->data(ToolXmlRole).value<QString>());
    QDomElement toolElement = doc.documentElement();

    EditAnnotToolDialog t(this, toolElement);

    if (t.exec() != QDialog::Accepted)
        return;

    doc = t.toolXml();
    toolElement = doc.documentElement();

    QString itemText = t.name();

    // Store name attribute only if the user specified a customized name
    if (!itemText.isEmpty())
        toolElement.setAttribute(QStringLiteral("name"), itemText);
    else
        itemText = PageViewAnnotator::defaultToolName(toolElement);

    // Edit list entry and attach XML string as data
    listEntry->setText(itemText);
    listEntry->setData(ToolXmlRole, QVariant::fromValue(doc.toString(-1)));
    listEntry->setIcon(PageViewAnnotator::makeToolPixmap(toolElement));

    // Select and scroll
    m_list->setCurrentItem(listEntry);
    m_list->scrollToItem(listEntry);
    updateButtons();
    emit changed();
}

void WidgetAnnotTools::slotAdd()
{
    EditAnnotToolDialog t(this);

    if (t.exec() != QDialog::Accepted)
        return;

    QDomDocument rootDoc = t.toolXml();
    QDomElement toolElement = rootDoc.documentElement();

    QString itemText = t.name();

    // Store name attribute only if the user specified a customized name
    if (!itemText.isEmpty())
        toolElement.setAttribute(QStringLiteral("name"), itemText);
    else
        itemText = PageViewAnnotator::defaultToolName(toolElement);

    // Create list entry and attach XML string as data
    QListWidgetItem *listEntry = new QListWidgetItem(itemText, m_list);
    listEntry->setData(ToolXmlRole, QVariant::fromValue(rootDoc.toString(-1)));
    listEntry->setIcon(PageViewAnnotator::makeToolPixmap(toolElement));

    // Select and scroll
    m_list->setCurrentItem(listEntry);
    m_list->scrollToItem(listEntry);
    updateButtons();
    emit changed();
}
