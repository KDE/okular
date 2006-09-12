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

#include <kpagedialog.h>

class QLineEdit;
class KColorButton;
class KIntNumInput;
class Annotation;
class AnnotationWidget;
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
    QLineEdit *AuthorEdit;
    QLineEdit *uniqueNameEdit;
    QLineEdit *contentsEdit,
        *flagsEdit,
        *boundaryEdit;
    KColorButton *colorBn;
    KIntNumInput *m_opacity;
    AnnotationWidget *m_annotWidget;
    
    void setCaptionTextbyAnnotType();

public slots:
    void slotapply();
};


#endif
