/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _ANNOTWINDOW_H_
#define _ANNOTWINDOW_H_

#include <qframe.h>

class QTextEdit;
class Annotation;
class MovableTitle;

class AnnotWindow : public QFrame
{
    Q_OBJECT
    public:
        AnnotWindow( QWidget * parent, Annotation * annot);
        
    private:
        MovableTitle * m_title;
        QTextEdit *textEdit;
    public:
        Annotation* m_annot;

    private slots:
        void slotOptionBtn();
        void slotsaveWindowText();
};
#endif
