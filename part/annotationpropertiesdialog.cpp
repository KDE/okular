/*
    SPDX-FileCopyrightText: 2006 Chu Xiaodong <xiaodongchu@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "annotationpropertiesdialog.h"

// qt/kde includes
#include <KLineEdit>
#include <KLocalizedString>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTextEdit>

// local includes
#include "annotationwidgets.h"
#include "core/annotations.h"
#include "core/document.h"
#include "core/page.h"

AnnotsPropertiesDialog::AnnotsPropertiesDialog(QWidget *parent, Okular::Document *document, int docpage, Okular::Annotation *ann)
    : KPageDialog(parent)
    , m_document(document)
    , m_page(docpage)
    , modified(false)
{
    setFaceType(Tabbed);
    m_annot = ann;
    const bool canEditAnnotations = m_document->canModifyPageAnnotation(ann);
    setCaptionTextbyAnnotType();
    if (canEditAnnotations) {
        setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
        button(QDialogButtonBox::Apply)->setEnabled(false);
        connect(button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &AnnotsPropertiesDialog::slotapply);
        connect(button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &AnnotsPropertiesDialog::slotapply);
    } else {
        setStandardButtons(QDialogButtonBox::Close);
        button(QDialogButtonBox::Close)->setDefault(true);
    }

    m_annotWidget = AnnotationWidgetFactory::widgetFor(ann);

    QLabel *tmplabel;
    // 1. Appearance
    // BEGIN tab1
    QWidget *appearanceWidget = m_annotWidget->appearanceWidget();
    appearanceWidget->setEnabled(canEditAnnotations);
    addPage(appearanceWidget, i18n("&Appearance"));
    // END tab1

    // BEGIN tab 2
    QFrame *page = new QFrame(this);
    addPage(page, i18n("&General"));
    //    m_tabitem[1]->setIcon( QIcon::fromTheme( "fonts" ) );
    QFormLayout *gridlayout = new QFormLayout(page);
    AuthorEdit = new KLineEdit(ann->author(), page);
    AuthorEdit->setEnabled(canEditAnnotations);
    gridlayout->addRow(i18n("&Author:"), AuthorEdit);

    tmplabel = new QLabel(page);
    tmplabel->setText(QLocale().toString(ann->creationDate(), QLocale::LongFormat));
    tmplabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridlayout->addRow(i18n("Created:"), tmplabel);

    m_modifyDateLabel = new QLabel(page);
    m_modifyDateLabel->setText(QLocale().toString(ann->modificationDate(), QLocale::LongFormat));
    m_modifyDateLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    gridlayout->addRow(i18n("Modified:"), m_modifyDateLabel);

    // END tab 2

    QWidget *extraWidget = m_annotWidget->extraWidget();
    if (extraWidget) {
        addPage(extraWidget, extraWidget->windowTitle());
    }

    // BEGIN connections
    connect(AuthorEdit, &QLineEdit::textChanged, this, &AnnotsPropertiesDialog::setModified);
    connect(m_annotWidget, &AnnotationWidget::dataChanged, this, &AnnotsPropertiesDialog::setModified);
    // END

#if 0
    qCDebug(OkularUiDebug) << "Annotation details:";
    qCDebug(OkularUiDebug).nospace() << " => unique name: '" << ann->uniqueName() << "'";
    qCDebug(OkularUiDebug) << " => flags:" << QString::number( m_annot->flags(), 2 );
#endif

    resize(sizeHint());
}
AnnotsPropertiesDialog::~AnnotsPropertiesDialog()
{
    delete m_annotWidget;
}

void AnnotsPropertiesDialog::setCaptionTextbyAnnotType()
{
    Okular::Annotation::SubType type = m_annot->subType();
    QString captiontext;
    switch (type) {
    case Okular::Annotation::AText:
        if (((Okular::TextAnnotation *)m_annot)->textType() == Okular::TextAnnotation::Linked)
            captiontext = i18n("Pop-up Note Properties");
        else {
            if (((Okular::TextAnnotation *)m_annot)->inplaceIntent() == Okular::TextAnnotation::TypeWriter)
                captiontext = i18n("Typewriter Properties");
            else
                captiontext = i18n("Inline Note Properties");
        }
        break;
    case Okular::Annotation::ALine:
        if (((Okular::LineAnnotation *)m_annot)->linePoints().count() == 2)
            captiontext = i18n("Straight Line Properties");
        else
            captiontext = i18n("Polygon Properties");
        break;
    case Okular::Annotation::AGeom:
        captiontext = i18n("Geometry Properties");
        break;
    case Okular::Annotation::AHighlight:
        captiontext = i18n("Text Markup Properties");
        break;
    case Okular::Annotation::AStamp:
        captiontext = i18n("Stamp Properties");
        break;
    case Okular::Annotation::AInk:
        captiontext = i18n("Freehand Line Properties");
        break;
    case Okular::Annotation::ACaret:
        captiontext = i18n("Caret Properties");
        break;
    case Okular::Annotation::AFileAttachment:
        captiontext = i18n("File Attachment Properties");
        break;
    case Okular::Annotation::ASound:
        captiontext = i18n("Sound Properties");
        break;
    case Okular::Annotation::AMovie:
        captiontext = i18n("Movie Properties");
        break;
    default:
        captiontext = i18n("Annotation Properties");
        break;
    }
    setWindowTitle(captiontext);
}

void AnnotsPropertiesDialog::setModified()
{
    modified = true;
    button(QDialogButtonBox::Apply)->setEnabled(true);
}

void AnnotsPropertiesDialog::slotapply()
{
    if (!modified)
        return;

    m_document->prepareToModifyAnnotationProperties(m_annot);
    m_annot->setAuthor(AuthorEdit->text());
    m_annot->setModificationDate(QDateTime::currentDateTime());

    m_annotWidget->applyChanges();

    m_document->modifyPageAnnotationProperties(m_page, m_annot);

    m_modifyDateLabel->setText(i18n("Modified: %1", QLocale().toString(m_annot->modificationDate(), QLocale::LongFormat)));

    modified = false;
    button(QDialogButtonBox::Apply)->setEnabled(false);
}

#include "moc_annotationpropertiesdialog.cpp"
