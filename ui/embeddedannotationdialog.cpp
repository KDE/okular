/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

//#include "core/document.h"
#include "embeddedannotationdialog.h"

EmbeddedAnnotationDialog::EmbeddedAnnotationDialog( QWidget * parent, Annotation * annot)
: QDialog(parent,0)
{
    m_annot=annot;
 setObjectName(QString::fromUtf8("Dialog"));
    resize(QSize(260, 220).expandedTo(minimumSizeHint()));
    setWindowIcon(QIcon());
    setSizeGripEnabled(true);
    textEdit = new QTextEdit(this);
    textEdit->setObjectName(QString::fromUtf8("textEdit"));
    textEdit->setGeometry(QRect(3, 3, 256, 201));
    if(annot)
        textEdit->setPlainText(annot->window.text);
   // textEdit->setBackgroundColor(QColor::fromRgb(255,255,0));
    QPalette pl;

    pl=palette();
    this->setPalette(QPalette(Qt::yellow));
    pl=textEdit->palette();
    pl.setColor(QPalette::Background,Qt::yellow);
    setPalette(pl);
   // pl.setColor(QPalette::Window,QColor(255,0,0));
   // pl.setColor(QPalette::Background,QColor(255,0,0));
  //  pl.setColor(QPalette::AlternateBase,QColor(255,0,0));
    pl.setColor(QPalette::Base,Qt::yellow);
    textEdit->setPalette(pl);
  //  textEdit->setTextColor(QColor(0,255,0));
   // textEdit->set
    //retranslateUi(Dialog);
setWindowTitle(QApplication::translate("Dialog", "", 0, QApplication::UnicodeUTF8));
//    Q_UNUSED(this);

//    QMetaObject::connectSlotsByName(this);
}


#include "embeddedannotationdialog.moc"

