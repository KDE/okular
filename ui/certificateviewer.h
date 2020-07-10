/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_CERTIFICATEVIEWER_H
#define OKULAR_CERTIFICATEVIEWER_H

#include <KPageDialog>
#include <QAbstractTableModel>
#include <QVector>

#include "core/signatureutils.h"

class QTextEdit;

namespace Okular
{
class CertificateInfo;
}

class CertificateModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CertificateModel(const Okular::CertificateInfo &certInfo, QObject *parent = nullptr);

    enum { PropertyKeyRole = Qt::UserRole, PropertyVisibleValueRole };

    enum Property { Version, SerialNumber, Issuer, IssuedOn, ExpiresOn, Subject, PublicKey, KeyUsage };
    Q_ENUM(Property)

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<Property> m_certificateProperties;
    const Okular::CertificateInfo &m_certificateInfo;
};

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
