#include "closedialoghelper.h"

#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QTest>

namespace TestingUtils
{
CloseDialogHelper::CloseDialogHelper(Okular::Part *p, QDialogButtonBox::StandardButton b)
    : m_widget(p->widget())
    , m_button(b)
    , m_clicked(false)
{
    QTimer::singleShot(0, this, &CloseDialogHelper::closeDialog);
}

CloseDialogHelper::CloseDialogHelper(QWidget *w, QDialogButtonBox::StandardButton b)
    : m_widget(w)
    , m_button(b)
    , m_clicked(false)
{
    QTimer::singleShot(0, this, &CloseDialogHelper::closeDialog);
}

CloseDialogHelper::CloseDialogHelper(QDialogButtonBox::StandardButton b)
    : m_widget(nullptr)
    , m_button(b)
    , m_clicked(false)
{
    QTimer::singleShot(0, this, &CloseDialogHelper::closeDialog);
}

CloseDialogHelper::~CloseDialogHelper()
{
    QVERIFY(m_clicked);
}

void CloseDialogHelper::closeDialog()
{
    QWidget *dialog = (m_widget) ? m_widget->findChild<QDialog *>() : qApp->activeModalWidget();
    if (!dialog) {
        QTimer::singleShot(0, this, &CloseDialogHelper::closeDialog);
        return;
    }
    QDialogButtonBox *buttonBox = dialog->findChild<QDialogButtonBox *>();
    buttonBox->button(m_button)->click();
    m_clicked = true;
}
}
