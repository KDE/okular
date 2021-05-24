/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DLGEDITOR_H
#define DLGEDITOR_H

#include <QHash>
#include <QWidget>

class QComboBox;
class QFormLayout;
class QLineEdit;
class QStackedWidget;

class DlgEditor : public QWidget
{
    Q_OBJECT

public:
    explicit DlgEditor(QWidget *parent = nullptr);
    ~DlgEditor() override;

private Q_SLOTS:
    void editorChanged(int which);

private:
    QComboBox *m_editorChooser;
    // Two line edits, because one is connected to the config skeleton.
    QLineEdit *m_editorCommandDisplay;
    QLineEdit *m_editorCommandEditor;
    QStackedWidget *m_editorCommandStack;
    QFormLayout *m_layout;

    QHash<int, QString> m_editors;
};

#endif
