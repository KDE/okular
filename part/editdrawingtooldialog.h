/*
    SPDX-FileCopyrightText: 2015 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef EDITDRAWINGTOOLDIALOG_H
#define EDITDRAWINGTOOLDIALOG_H

#include <QDialog>
#include <QDomElement>

class KColorButton;
class KLineEdit;

class QSpinBox;

class EditDrawingToolDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EditDrawingToolDialog(const QDomElement &initialState = QDomElement(), QWidget *parent = nullptr);
    ~EditDrawingToolDialog() override;

    QDomDocument toolXml() const;

    QString name() const;

private:
    void loadTool(const QDomElement &toolElement);

    KLineEdit *m_name;
    KColorButton *m_colorBn;
    QSpinBox *m_penWidth;
    QSpinBox *m_opacity;
};

#endif // EDITDRAWINGTOOLDIALOG_H
