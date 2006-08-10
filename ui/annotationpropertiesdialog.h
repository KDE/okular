/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _ANNOTSPROPERTIESDIALOG_H_
#define _ANNOTSPROPERTIESDIALOG_H_

#include <qabstractitemmodel.h>
#include <QTextStream>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
#include <QtGui/QTabWidget>
#include <QtGui/QWidget>

#include <kpagedialog.h>

class KPDFDocument;
class AnnotsPropertiesDialog : public KPageDialog
{
    Q_OBJECT
public:
    AnnotsPropertiesDialog( QWidget *parent,Annotation* ann );
    ~AnnotsPropertiesDialog();
    bool modified;

private:
    
    Annotation* m_annot;    //source annotation
    //dialog widgets:
    QFrame *m_page[3];
    KPageWidgetItem *m_tabitem[3];
    QGridLayout * m_layout[3];
    QLineEdit *opacityEdit;
    QLineEdit *AuthorEdit;
    QLineEdit *uniqueNameEdit;
    QLineEdit *contentsEdit,
        *flagsEdit,
        *boundaryEdit;
    QPushButton* colorBn;
    QSlider *opacitySlider;
    QColor m_selcol;
    
    void setCaptionTextbyAnnotType();

public slots:
    void slotChooseColor();
    void slotapply();
};


#endif
