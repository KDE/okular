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
#include <QTreeView>
#include "core/signatureutils.h"
#include "core/observer.h"
#include "fileprinterpreview.h"
//#include "signaturemodel.h"

class QTextEdit;

namespace Okular {
    class Document;
    class FormFieldSignature;
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
        SignaturePropertiesDialog( Okular::Document *doc, Okular::FormFieldSignature *form, QWidget *parent );

    private Q_SLOTS:
        void viewSignedVersion();
        void viewCertificateProperties();

    private:
        Okular::Document *m_doc;
        Okular::FormFieldSignature *m_signatureForm;
        Okular::SignatureInfo *m_signatureInfo;
};

class RevisionViewer : public Okular::FilePrinterPreview
{
    Q_OBJECT
    public:
        RevisionViewer( const QString &filename, QWidget *parent = nullptr );
        ~RevisionViewer();
};

class TreeView1 : public QTreeView
{
  Q_OBJECT

  public:
    TreeView1( Okular::Document *document, QWidget *parent = Q_NULLPTR );
  protected:
    void paintEvent( QPaintEvent *event ) override;

  private:
    Okular::Document *m_document;
};

class SignaturePanel : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
    public:
        SignaturePanel( QWidget * parent, Okular::Document * document );
        ~SignaturePanel();

         void notifySetup( const QVector< Okular::Page * > & pages, int setupFlags ) override;

    private:
         Okular::Document *m_document;
         TreeView1 *m_view;
         //SignatureModel *m_model;
};


#endif
