/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SIGNATUREWIDGETS_H
#define OKULAR_SIGNATUREWIDGETS_H

#include <QDialog>
#include <QVector>
#include <QAbstractTableModel>

#include "core/signatureutils.h"

class QTextEdit;

namespace Okular {
    class SignatureInfo;
}

class CertificateViewerModel : public QAbstractTableModel
{
    Q_OBJECT

    public:
        explicit CertificateViewerModel( Okular::SignatureInfo *sigInfo, QObject * parent = nullptr );

        enum {
            PropertyValueRole = Qt::UserRole
        };

        int columnCount( const QModelIndex &parent = QModelIndex() ) const override;
        int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
        QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
        QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;

    private:
        QVector< QPair<QString, QString> > m_sigProperties;
};

class CertificateViewer : public QDialog
{
    Q_OBJECT

    public:
        CertificateViewer( Okular::SignatureInfo *sigInfo, QWidget *parent );

    private Q_SLOTS:
        void updateText( const QModelIndex &index );

    private:
        CertificateViewerModel  *m_sigPropModel;
        QTextEdit *m_sigPropText;
        Okular::SignatureInfo *m_sigInfo;
};

class SignaturePropertiesDialog : public QDialog
{
    Q_OBJECT

    public:
        SignaturePropertiesDialog( Okular::SignatureInfo *sigInfo, QWidget *parent );

    private Q_SLOTS:
        void viewSignedVersion();
        void viewCertificateProperties();

    private:
        Okular::SignatureInfo *m_sigInfo;
};

#endif
