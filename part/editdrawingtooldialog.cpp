/*
    SPDX-FileCopyrightText: 2015 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "editdrawingtooldialog.h"

#include <KColorButton>
#include <KLineEdit>
#include <KLocalizedString>

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

EditDrawingToolDialog::EditDrawingToolDialog(const QDomElement &initialState, QWidget *parent)
    : QDialog(parent)
{
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->setObjectName(QStringLiteral("buttonbox"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return); // NOLINT(bugprone-suspicious-enum-usage)
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditDrawingToolDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &EditDrawingToolDialog::reject);
    okButton->setDefault(true);

    QWidget *widget = new QWidget(this);
    QGridLayout *widgetLayout = new QGridLayout(widget);

    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);

    m_name = new KLineEdit(widget);
    m_name->setObjectName(QStringLiteral("name"));
    mainLayout->addWidget(m_name);

    QLabel *tmplabel = new QLabel(i18n("&Name:"), widget);
    mainLayout->addWidget(tmplabel);
    tmplabel->setBuddy(m_name);

    widgetLayout->addWidget(tmplabel, 0, 0, Qt::AlignRight);
    widgetLayout->addWidget(m_name, 0, 1);

    tmplabel = new QLabel(i18n("Color:"), widget);
    widgetLayout->addWidget(tmplabel, 1, 0, Qt::AlignRight);

    m_colorBn = new KColorButton(this);
    m_colorBn->setObjectName(QStringLiteral("colorbutton"));
    widgetLayout->addWidget(m_colorBn, 1, 1, Qt::AlignRight);

    tmplabel = new QLabel(i18n("&Pen Width:"), widget);
    widgetLayout->addWidget(tmplabel, 2, 0, Qt::AlignRight);

    m_penWidth = new QSpinBox(widget);
    m_penWidth->setObjectName(QStringLiteral("penWidth"));
    m_penWidth->setRange(0, 50);
    m_penWidth->setSuffix(i18nc("Suffix for the pen width, eg '10 px'", " px"));
    tmplabel->setBuddy(m_penWidth);
    widgetLayout->addWidget(m_penWidth, 2, 1);

    tmplabel = new QLabel(i18n("&Opacity:"), widget);
    widgetLayout->addWidget(tmplabel, 3, 0, Qt::AlignRight);

    m_opacity = new QSpinBox(widget);
    m_opacity->setObjectName(QStringLiteral("opacity"));
    m_opacity->setRange(0, 100);
    m_opacity->setSuffix(i18nc("Suffix for the opacity level, eg '80 %'", " %"));
    tmplabel->setBuddy(m_opacity);
    widgetLayout->addWidget(m_opacity, 3, 1);

    if (initialState.isNull()) {
        setWindowTitle(i18n("Create drawing tool"));
        m_colorBn->setColor(Qt::black);
        m_penWidth->setValue(2);
        m_opacity->setValue(100);
    } else {
        setWindowTitle(i18n("Edit drawing tool"));
        loadTool(initialState);
    }

    m_name->setFocus();
}

EditDrawingToolDialog::~EditDrawingToolDialog()
{
}

QString EditDrawingToolDialog::name() const
{
    return m_name->text();
}

QDomDocument EditDrawingToolDialog::toolXml() const
{
    QDomDocument doc;
    QDomElement toolElement = doc.createElement(QStringLiteral("tool"));
    QDomElement engineElement = doc.createElement(QStringLiteral("engine"));
    QDomElement annotationElement = doc.createElement(QStringLiteral("annotation"));
    doc.appendChild(toolElement);
    toolElement.appendChild(engineElement);
    engineElement.appendChild(annotationElement);

    const QString color = m_colorBn->color().name();
    const double opacity = m_opacity->value() / 100.0;

    engineElement.setAttribute(QStringLiteral("color"), color);

    annotationElement.setAttribute(QStringLiteral("type"), QStringLiteral("Ink"));
    annotationElement.setAttribute(QStringLiteral("color"), color);
    annotationElement.setAttribute(QStringLiteral("width"), QString::number(m_penWidth->value()));

    if (opacity != 1.0)
        annotationElement.setAttribute(QStringLiteral("opacity"), QString::number(opacity));

    return doc;
}

void EditDrawingToolDialog::loadTool(const QDomElement &toolElement)
{
    const QDomElement engineElement = toolElement.elementsByTagName(QStringLiteral("engine")).item(0).toElement();
    const QDomElement annotationElement = engineElement.elementsByTagName(QStringLiteral("annotation")).item(0).toElement();

    if (annotationElement.hasAttribute(QStringLiteral("color")))
        m_colorBn->setColor(QColor(annotationElement.attribute(QStringLiteral("color"))));

    m_penWidth->setValue(annotationElement.attribute(QStringLiteral("width"), QStringLiteral("2")).toInt());
    m_opacity->setValue(annotationElement.attribute(QStringLiteral("opacity"), QStringLiteral("1.0")).toDouble() * 100);

    if (toolElement.attribute(QStringLiteral("default"), QStringLiteral("false")) == QLatin1String("true"))
        m_name->setText(i18n(toolElement.attribute(QStringLiteral("name")).toLatin1().constData()));
    else
        m_name->setText(toolElement.attribute(QStringLiteral("name")));
}
