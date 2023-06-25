/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SIGNATUREPROPERTIESDIALOG_H
#define OKULAR_SIGNATUREPROPERTIESDIALOG_H

#include <QDialog>

#include <memory>

namespace Okular
{
class Document;
class FormFieldSignature;
}

class SignaturePropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    SignaturePropertiesDialog(Okular::Document *doc, const Okular::FormFieldSignature *form, QWidget *parent = nullptr);

public Q_SLOTS:
    void viewSignedVersion();
    void viewCertificateProperties();

private:
    Okular::Document *m_doc;
    const Okular::FormFieldSignature *m_signatureForm;
    QString m_kleopatraPath;
};

#endif
