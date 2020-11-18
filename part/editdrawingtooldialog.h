/***************************************************************************
 *   Copyright (C) 2015 by Laurent Montel <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
