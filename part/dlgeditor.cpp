/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dlgeditor.h"

#include "core/texteditors_p.h"

#include <KLocalizedString>

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>

DlgEditor::DlgEditor(QWidget *parent)
    : QWidget(parent)
{
    // Set up the user interface
    m_layout = new QFormLayout(this);

    // BEGIN "Editor" row: Combo box with a list of preset editors
    m_editorChooser = new QComboBox(this);
    m_editorChooser->setObjectName(QStringLiteral("kcfg_ExternalEditor"));
    m_editorChooser->setWhatsThis(i18nc("@info:whatsthis Config dialog, editor page", "Choose the editor you want to launch when Okular wants to open a source file."));
    m_editorChooser->addItem(i18nc("@item:inlistbox Config dialog, editor page", "Custom Text Editor"), 0);
    m_editorChooser->addItem(i18nc("@item:inlistbox Config dialog, editor page", "Kate"), 1);
    m_editorChooser->addItem(i18nc("@item:inlistbox Config dialog, editor page", "Kile"), 2);
    m_editorChooser->addItem(i18nc("@item:inlistbox Config dialog, editor page", "SciTE"), 3);
    m_editorChooser->addItem(i18nc("@item:inlistbox Config dialog, editor page", "Emacs client"), 4);
    m_editorChooser->addItem(i18nc("@item:inlistbox Config dialog, editor page", "Lyx client"), 5);
    m_editorChooser->addItem(i18nc("@item:inlistbox Config dialog, editor page", "TeXstudio"), 6);
    m_editorChooser->addItem(i18nc("@item:inlistbox Config dialog, editor page", "TeXiFy IDEA"), 7);
    m_layout->addRow(i18nc("@label:listbox Config dialog, editor page", "Editor:"), m_editorChooser);
    // END "Editor" row

    // BEGIN "Command" row: two line edits, one to display preset commands, one to enter custom command.
    m_editorCommandStack = new QStackedWidget(this);

    m_editorCommandDisplay = new QLineEdit(this);
    m_editorCommandDisplay->setReadOnly(true);
    m_editorCommandStack->addWidget(m_editorCommandDisplay);
    // Let stack widget be layouted like a line edit:
    m_editorCommandStack->setSizePolicy(m_editorCommandDisplay->sizePolicy());

    m_editorCommandEditor = new QLineEdit(this);
    m_editorCommandEditor->setObjectName(QStringLiteral("kcfg_ExternalEditorCommand"));
    m_editorCommandEditor->setWhatsThis(i18nc("@info:whatsthis",
                                              "<qt>Set the command of a custom text editor to be launched.<br />\n"
                                              "You can also put few placeholders:\n"
                                              "<ul>\n"
                                              "  <li>%f - the file name</li>\n"
                                              "  <li>%l - the line of the file to be reached</li>\n"
                                              "  <li>%c - the column of the file to be reached</li>\n"
                                              "</ul>\n"
                                              "If %f is not specified, then the file name is appended to the specified "
                                              "command."));
    m_editorCommandStack->addWidget(m_editorCommandEditor);

    m_layout->addRow(i18nc("@label:textbox Config dialog, editor page", "Command:"), m_editorCommandStack);

    // Initialize
    editorChanged(0);
    // END "Command" row

    setLayout(m_layout);

    // Set up the logic
    m_editors = Okular::buildEditorsMap();

    connect(m_editorChooser, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DlgEditor::editorChanged);
}

DlgEditor::~DlgEditor()
{
}

void DlgEditor::editorChanged(int which)
{
    // map combobox index to editor index. Custom editor has index 0, and is not present in m_editors.
    const int whichEditor = m_editorChooser->itemData(which).toInt();
    const QHash<int, QString>::const_iterator it = m_editors.constFind(whichEditor);
    if (it != m_editors.constEnd()) {
        // Show editor command display and manually connect buddy
        m_editorCommandDisplay->setText(it.value());
        m_editorCommandStack->setCurrentIndex(0);
        if (QLabel *l = qobject_cast<QLabel *>(m_layout->labelForField(m_editorCommandStack))) {
            l->setBuddy(m_editorCommandDisplay);
        }
    } else {
        // Show editor command editor and manually connect buddy
        m_editorCommandStack->setCurrentIndex(1);
        if (QLabel *l = qobject_cast<QLabel *>(m_layout->labelForField(m_editorCommandStack))) {
            l->setBuddy(m_editorCommandEditor);
        }
    }
}
