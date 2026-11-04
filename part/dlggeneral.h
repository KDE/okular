/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <toscano.pino@tiscali.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _DLGGENERAL_H
#define _DLGGENERAL_H

#include <QCheckBox>
#include <QLabel>
#include <QWidget>

#include "part.h"

class DlgGeneral : public QWidget
{
    Q_OBJECT

public:
    explicit DlgGeneral(QWidget *parent, Okular::EmbedMode embedMode);
};

class WrappableQCheckBox : public QWidget
{
    Q_OBJECT
public:
    explicit WrappableQCheckBox(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags {});

    void setWordWrap(bool wrap);
    void setText(const QString &text);
    bool isChecked() const;
    void setInnerObjectName(QAnyStringView name)
    {
        m_checkbox->setObjectName(name);
    }

Q_SIGNALS:
    void toggled(bool checked);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QLabel *m_text = nullptr;
    QCheckBox *m_checkbox = nullptr;
    qint64 m_mouseDown = 0;
};

#endif
