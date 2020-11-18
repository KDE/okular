#ifndef CLOSEDIALOGHELPER_H
#define CLOSEDIALOGHELPER_H

#include <QDialogButtonBox>
#include <QObject>

#include "../part/part.h"

namespace TestingUtils
{
/*
 *  The CloseDialogHelper class is a helper to auto close modals opened in tests.
 */
class CloseDialogHelper : public QObject
{
    Q_OBJECT

public:
    CloseDialogHelper(Okular::Part *p, QDialogButtonBox::StandardButton b);

    CloseDialogHelper(QWidget *w, QDialogButtonBox::StandardButton b);

    // Close a modal dialog, which may not be associated to any other widget
    CloseDialogHelper(QDialogButtonBox::StandardButton b);

    ~CloseDialogHelper() override;

private slots:
    void closeDialog();

private:
    QWidget *m_widget;
    QDialogButtonBox::StandardButton m_button;
    bool m_clicked;
};

}

#endif // CLOSEDIALOGHELPER_H
