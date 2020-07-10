/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgeditor.h"

#include "core/texteditors_p.h"

#include <KLocalizedString>

#include "ui_dlgeditorbase.h"

DlgEditor::DlgEditor(QWidget *parent)
    : QWidget(parent)
{
    m_dlg = new Ui_DlgEditorBase();
    m_dlg->setupUi(this);

    m_editors = Okular::buildEditorsMap();

    connect(m_dlg->kcfg_ExternalEditor, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DlgEditor::editorChanged);

    m_dlg->kcfg_ExternalEditor->addItem(i18nc("Text editor", "Custom Text Editor"));
    m_dlg->kcfg_ExternalEditor->addItem(i18nc("Text editor", "Kate"), 1);
    m_dlg->kcfg_ExternalEditor->addItem(i18nc("Text editor", "Kile"), 2);
    m_dlg->kcfg_ExternalEditor->addItem(i18nc("Text editor", "SciTE"), 3);
    m_dlg->kcfg_ExternalEditor->addItem(i18nc("Text editor", "Emacs client"), 4);
    m_dlg->kcfg_ExternalEditor->addItem(i18nc("Text editor", "Lyx client"), 5);
    m_dlg->kcfg_ExternalEditor->addItem(i18nc("Text editor", "TeXstudio"), 6);
    m_dlg->kcfg_ExternalEditor->addItem(i18nc("Text editor", "TeXiFy IDEA"), 7);

    m_dlg->kcfg_ExternalEditorCommand->setWhatsThis(i18nc("@info:whatsthis",
                                                          "<qt>Set the command of a custom text editor to be launched.<br />\n"
                                                          "You can also put few placeholders:\n"
                                                          "<ul>\n"
                                                          "  <li>%f - the file name</li>\n"
                                                          "  <li>%l - the line of the file to be reached</li>\n"
                                                          "  <li>%c - the column of the file to be reached</li>\n"
                                                          "</ul>\n"
                                                          "If %f is not specified, then the file name is appended to the specified "
                                                          "command."));
}

DlgEditor::~DlgEditor()
{
    delete m_dlg;
}

void DlgEditor::editorChanged(int which)
{
    const int whichEditor = m_dlg->kcfg_ExternalEditor->itemData(which).toInt();
    const QHash<int, QString>::const_iterator it = m_editors.constFind(whichEditor);
    QString editor;
    if (it != m_editors.constEnd())
        editor = it.value();

    if (!editor.isEmpty()) {
        m_dlg->stackCommands->setCurrentIndex(1);
        m_dlg->leReadOnlyCommand->setText(editor);
    } else {
        m_dlg->stackCommands->setCurrentIndex(0);
    }
}

#include "moc_dlgeditor.cpp"
