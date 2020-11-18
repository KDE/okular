/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _ANNOTATIONPROPERTIESDIALOG_H_
#define _ANNOTATIONPROPERTIESDIALOG_H_

#include <KPageDialog>

class QLabel;
class QLineEdit;
class AnnotationWidget;

namespace Okular
{
class Annotation;
class Document;
}

class AnnotsPropertiesDialog : public KPageDialog
{
    Q_OBJECT
public:
    AnnotsPropertiesDialog(QWidget *parent, Okular::Document *document, int docpage, Okular::Annotation *ann);
    ~AnnotsPropertiesDialog() override;

private:
    Okular::Document *m_document;
    int m_page;
    bool modified;
    Okular::Annotation *m_annot; // source annotation
    // dialog widgets:
    QLineEdit *AuthorEdit;
    AnnotationWidget *m_annotWidget;
    QLabel *m_modifyDateLabel;

    void setCaptionTextbyAnnotType();

private Q_SLOTS:
    void setModified();
    void slotapply();
};

#endif
