/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
//annotwindow.h
#ifndef _ANNOTWINDOW_H_
#define _ANNOTWINDOW_H_

#include <QtGui/qwidget.h>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QTextEdit>

#include "core/annotations.h"
//this widget is for the titlebar and size griper
class MouseBox : public QWidget
{
    Q_OBJECT
    public:
        MouseBox( QWidget * parent);
    signals:
        void paintevent(QPaintEvent *e);
        void mousepressevent(QMouseEvent *e);
        void mousemoveevent(QMouseEvent *e);
        void mousereleaseevent(QMouseEvent *e);
    private:
        //QWidget * m_parent;
    public:
        QPoint pointpressed;
    protected:
        void paintEvent(QPaintEvent *e);
        void mousePressEvent(QMouseEvent *e);
        void mouseMoveEvent(QMouseEvent *e);
        void mouseReleaseEvent(QMouseEvent *e);

};

class AnnotWindow : public QWidget
{
    Q_OBJECT
    public:
        AnnotWindow( QWidget * parent, Annotation * annot);
        
    private:
        QFrame *frame;
        //QLabel *labelAnnotName;
        //QLabel *labelTimeDate;
        //QRect rcCloseBtn;
        //QRect rcOptionBtn;
        //QSizeGrip *resizer;
        MouseBox* titleBox;
        MouseBox* resizerBox;
        MouseBox* btnClose;
        MouseBox* btnOption;
        QString modTime;
        
       // QRect titlerc;
       // QRect sizegriprc;
        
        QTextEdit *textEdit;
    public:
        Annotation* m_annot;
    protected:
        void paintEvent(QPaintEvent * e);
        void resizeEvent ( QResizeEvent * e );
//        void mousePressEvent(QMouseEvent * e);
//        void mouseMoveEvent(QMouseEvent * e);
    private slots:
        void slotTitleMouseMove(QMouseEvent* e);
        void slotResizerMouseMove(QMouseEvent* e);
        void slotResizerPaint(QPaintEvent* e);
        void slotPaintCloseBtn(QPaintEvent* e);
 //       void slotPaintOptionBtn(QPaintEvent* e);
        void slotCloseBtn( QMouseEvent* e);
        void slotOptionBtn( QMouseEvent* e);
        void slotsaveWindowText();

};
#endif
