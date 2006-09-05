
/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include <QPainter>
#include "annotwindow.h"


MouseBox::MouseBox( QWidget * parent)
    : QWidget(parent),pointpressed(0,0)//pointpressed(QPoint(0,0))// m_parent(parent)
{
}

void MouseBox::paintEvent(QPaintEvent *e)
{
    emit paintevent(e);
}
void MouseBox::mousePressEvent(QMouseEvent *e)
{
    pointpressed=e->pos();
    //kDebug( )<<"astario:  mousebox pressed"<<endl;
    emit mousepressevent(e);
}
void MouseBox::mouseMoveEvent(QMouseEvent *e)
{
    emit mousemoveevent(e);
}
void MouseBox::mouseReleaseEvent(QMouseEvent *e)
{
    emit mousereleaseevent(e);
}

AnnotWindow::AnnotWindow( QWidget * parent, Annotation * annot)
    : QWidget(parent,Qt::SubWindow), m_annot( annot )
{
    textEdit=new QTextEdit(m_annot->window.text, this);
    connect(textEdit,SIGNAL(textChanged()),
            this,SLOT(slotsaveWindowText()));
    modTime=m_annot->modifyDate.toString(Qt::ISODate);
    
    setPalette( QPalette(m_annot->style.color));
    QPalette pl=textEdit->palette();
    pl.setColor(QPalette::Base,m_annot->style.color);
    textEdit->setPalette(pl);
    titleBox=new MouseBox(this);
    titleBox->setCursor(Qt::SizeAllCursor);
    resizerBox=new MouseBox(this);
    resizerBox->setCursor(Qt::SizeFDiagCursor);
    connect( titleBox,SIGNAL(mousemoveevent(QMouseEvent*)),
             this,SLOT(slotTitleMouseMove(QMouseEvent*)));
    connect( resizerBox,SIGNAL(mousemoveevent(QMouseEvent*)),
             this,SLOT(slotResizerMouseMove(QMouseEvent*)));
    connect( resizerBox,SIGNAL(paintevent(QPaintEvent*)),
             this,SLOT(slotResizerPaint(QPaintEvent*)));
    btnClose=new MouseBox(this);
    connect( btnClose,SIGNAL(mousereleaseevent( QMouseEvent* )),
             this,SLOT(slotCloseBtn( QMouseEvent* )));
             connect( btnClose,SIGNAL(paintevent( QPaintEvent* )),
                      this,SLOT(slotPaintCloseBtn( QPaintEvent* )));
    btnOption=new MouseBox(this);
    
    connect( btnOption,SIGNAL(mousereleaseevent( QMouseEvent* )),
             this,SLOT(slotOptionBtn( QMouseEvent* )));
 //   connect( btnOption,SIGNAL(paintevent( QPaintEvent* )),
 //            this,SLOT(slotPaintOptionBtn( QPaintEvent* )));
    

    setGeometry(10,10,300,300 );
    

    
}

void AnnotWindow::paintEvent(QPaintEvent *)
{
    QPainter annopainter(this);
    QRect rc=rect();
    QPen pen=QPen(Qt::black);
    annopainter.setPen(pen);
    annopainter.setBrush(m_annot->style.color);
    annopainter.drawRect( rc );
    annopainter.translate( rc.topLeft() );

//    QFont qft=annopainter.font();
//    qft.setPointSize( 10 );
//    annopainter.setFont( qft);
    annopainter.drawText(rc.right()-150,16,modTime);
    annopainter.drawText(2,32,m_annot->author);
    
    pen.setWidth(2);
    annopainter.setPen(pen);
    annopainter.drawText(2,16,m_annot->window.summary);

    
    //draw options button:
    pen.setWidth(1);
    annopainter.setPen(pen);
    rc=btnOption->geometry();   //(0,0,x,x)
    annopainter.drawRect(rc.left(),rc.top(),rc.width()-1,rc.height()-1);
    annopainter.drawText( rc.left(),rc.top(),rc.width(),rc.height(),Qt::AlignLeft,"options");
    
//    annopainter.drawLine( 0,10,rc.width(),10);
}


void AnnotWindow::resizeEvent ( QResizeEvent * e )
{    //size:
    QSize sz=e->size();
    btnClose->setGeometry( sz.width()-16,2,14,14);
    btnOption->setGeometry( sz.width()-80,16,75,16);
    titleBox->setGeometry(0,0,sz.width(),32);
    textEdit->setGeometry(0,32,sz.width(),sz.height()-32-14);
    resizerBox->setGeometry(sz.width()-14,sz.height()-14,14,14);
    //titlerc.setRect( 0,0,sz.width(),20);
    //sizegriprc.setRect(sz.width()-14,sz.height()-14,14,14);
}

void AnnotWindow::slotTitleMouseMove(QMouseEvent* e)
{
    if (e->buttons() != Qt::LeftButton)
        return;
    move(e->pos()-titleBox->pointpressed+pos());
}
void AnnotWindow::slotResizerMouseMove(QMouseEvent* e)
{
    if (e->buttons() != Qt::LeftButton)
        return;
    QSize sz=size();
    QPoint dpt=e->pos()-resizerBox->pointpressed;
    sz.setHeight(qMax(10,sz.height()+dpt.y()));
    sz.setWidth(qMax(20,sz.width()+dpt.x()));
    resize(sz);
}
void AnnotWindow::slotResizerPaint(QPaintEvent* )
{
    //draw Size griper:
    QPainter pnter(resizerBox);
    int w=resizerBox->rect().width(),h=resizerBox->rect().height();
    for(int i=0;i<5;i++)
    {
        pnter.drawLine( w-1-i*3,h,w,h-1-i*3);
    }
}


void AnnotWindow::slotPaintCloseBtn(QPaintEvent* )
{
    //draw close button
    QPainter pnter(btnClose);
    QRect rc=btnClose->rect();
    rc.moveTo( 0,0);
    pnter.drawRect(rc.left(),rc.top(),rc.width()-1,rc.height()-1);
    pnter.drawLine(rc.topLeft(),rc.bottomRight());
    pnter.drawLine(rc.topRight(),rc.bottomLeft());
}

void AnnotWindow::slotCloseBtn( QMouseEvent* )
{
    this->hide();
}
void AnnotWindow::slotOptionBtn( QMouseEvent* )
{
    //TODO: call context menu in pageview
    //emit sig...
}
 void AnnotWindow::slotsaveWindowText()
{
    m_annot->window.text=textEdit->toPlainText();
}
#include "annotwindow.moc"
