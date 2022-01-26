/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_CERTIFICATEVIEWER_H
#define OKULAR_CERTIFICATEVIEWER_H

#include <KPageDialog>

#include "core/signatureutils.h"

class CertificateModel;

class QTextEdit;

namespace Okular
{
class CertificateInfo;
}

class CertificateViewer : public KPageDialog
{
    Q_OBJECT

public:
    CertificateViewer(const Okular::CertificateInfo &certInfo, QWidget *parent);

private Q_SLOTS:
    void updateText(const QModelIndex &index);
    void exportCertificate();

private:
    QTextEdit *m_propertyText;
    CertificateModel *m_certificateModel;
    const Okular::CertificateInfo &m_certificateInfo;
};

#endif
