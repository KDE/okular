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

#include <QVector>
#include <QAbstractTableModel>
#include <KPageDialog>

#include "core/signatureutils.h"

class QTextEdit;

namespace Okular {
    class CertificateInfo;
}

class CertificateModel : public QAbstractTableModel
{
    Q_OBJECT

    public:
        explicit CertificateModel( Okular::CertificateInfo *certInfo, QObject * parent = nullptr );

        enum {
            PropertyKeyRole = Qt::UserRole,
            PropertyValueRole,
            PublicKeyRole
        };

        int columnCount( const QModelIndex &parent = QModelIndex() ) const override;
        int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
        QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
        QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;

    private:
        QVector< QPair<QString, QString> > m_certificateProperties;
        Okular::CertificateInfo *m_certificateInfo;
};

class CertificateViewer : public KPageDialog
{
    Q_OBJECT

    public:
        CertificateViewer( Okular::CertificateInfo *certInfo, QWidget *parent );

    private Q_SLOTS:
        void updateText( const QModelIndex &index );
        void exportCertificate();

    private:
        QTextEdit *m_propertyText;
        CertificateModel  *m_certificateModel;
        Okular::CertificateInfo *m_certificateInfo;
};

#endif
