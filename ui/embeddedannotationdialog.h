/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _EMBEDDEDANNOTATIONDIALOG_H
#define _EMBEDDEDANNOTATIONDIALOG_H

#include <kdialog.h>
#include <QDialog>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QTextEdit>

#include "core/annotations.h"

class EmbeddedAnnotationDialog : public QDialog
{
Q_OBJECT
public:
    EmbeddedAnnotationDialog( QWidget * parent, Annotation * annot);

    
    
public:
    QTextEdit *textEdit;

    Annotation* m_annot;
};


#endif // _EMBEDDEDANNOTATIONDIALOG_H
