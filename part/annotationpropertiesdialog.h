/*
    SPDX-FileCopyrightText: 2006 Chu Xiaodong <xiaodongchu@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
