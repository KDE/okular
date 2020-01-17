/***************************************************************************
 *   Copyright (C) 2012 by Bubli                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _CERTIFICATETOOLS_H_
#define _CERTIFICATETOOLS_H_

#include <QWidget>
class QListWidget;
class QPushButton;

class CertificateTools : public QWidget
{
    Q_OBJECT

    Q_PROPERTY( QStringList certificates READ certificates WRITE setCertificates NOTIFY changed USER true )

    public:
        explicit CertificateTools( QWidget * parent = nullptr );
        ~CertificateTools() override;

        QStringList certificates() const;
        void setCertificates(const QStringList& items);

    Q_SIGNALS:
        void changed();

    protected:
        QListWidget *m_list;
    private:
        QPushButton *m_btnAdd;
        QPushButton *m_btnEdit;
        QPushButton *m_btnRemove;
        QPushButton *m_btnMoveUp;
        QPushButton *m_btnMoveDown;

    protected Q_SLOTS:
        void slotAdd();
        void slotEdit();
        void updateButtons();
        void slotRemove();
        void slotMoveUp();
        void slotMoveDown();
};

#endif
