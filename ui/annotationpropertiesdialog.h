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
    AnnotsPropertiesDialog( QWidget *parent, KPDFDocument *doc, Annotation* ann );

private:
    QLineEdit *opacityEdit;
    QSlider *opacitySlider;
    QLineEdit *AuthorEdit;
    Annotation* m_annot;

public slots:
    void slotChooseColor();
};


#endif
