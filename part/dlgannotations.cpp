/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <toscano.pino@tiscali.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dlgannotations.h"

#include "widgetannottools.h"

#include <KLocalizedString>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

DlgAnnotations::DlgAnnotations(QWidget *parent)
    : QWidget(parent)
{
    QFormLayout *layout = new QFormLayout(this);

    // BEGIN Author row: Line edit to set the annotation’s default author value.
    QLineEdit *authorLineEdit = new QLineEdit(this);
    authorLineEdit->setObjectName(QStringLiteral("kcfg_IdentityAuthor"));
    layout->addRow(i18nc("@label:textbox Config dialog, annotations page", "Author:"), authorLineEdit);

    QLabel *authorInfoLabel = new QLabel(this);
    authorInfoLabel->setText(
        i18nc("@info Config dialog, annotations page", "<b>Note:</b> the information here is used only for annotations. The information is saved in annotated documents, and so will be transmitted together with the document."));
    authorInfoLabel->setWordWrap(true);
    layout->addRow(authorInfoLabel);
    // END Author row

    // Silly 1Em spacer:
    layout->addRow(new QLabel(this));

    // BEGIN Quick annotation tools section: WidgetAnnotTools manages tools.
    QLabel *toolsLabel = new QLabel(this);
    toolsLabel->setText(i18nc("@label Config dialog, annotations page, heading line for Quick Annotations tool manager", "<h3>Quick Annotation Tools</h3>"));
    layout->addRow(toolsLabel);

    WidgetAnnotTools *kcfg_QuickAnnotationTools = new WidgetAnnotTools(this);
    kcfg_QuickAnnotationTools->setObjectName(QStringLiteral("kcfg_QuickAnnotationTools"));
    layout->addRow(kcfg_QuickAnnotationTools);
    // END Quick annotation tools section
}
