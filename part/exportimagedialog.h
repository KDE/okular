/*
    SPDX-FileCopyrightText: 2024 Pratham Gandhi <ppg.1382@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_EXPORTIMAGEDIALOG_H_
#define _OKULAR_EXPORTIMAGEDIALOG_H_

#include "core/document.h"
#include "core/generator.h"
#include "core/observer.h"

#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMutex>
#include <QPixmap>
#include <QProgressDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

#include <atomic>

class ExportImageDocumentObserver : public QObject, public Okular::DocumentObserver
{
    Q_OBJECT
public:
    void notifyPageChanged(int page, int flags) override;
    void doExport(const QList<Okular::PixmapRequest *> &pixmapRequestList, Okular::Document *document, const QString &dirPath, QWidget *parent);

private Q_SLOTS:
    void progressDialogCanceled();

private:
    void getPixmapAndSave(int page);

    Okular::Document *m_document;
    QString m_dirPath;
    QProgressDialog *m_progressDialog;
    int m_progressValue;
    bool m_progressCanceled;
};

class ExportImageDialog : public QDialog
{
    Q_OBJECT
public:
    enum DialogCloseCode { Accepted, Canceled, InvalidOptions };

    ExportImageDialog(Okular::Document *document, QWidget *parent = nullptr);

    QString dirPath() const
    {
        return m_dirPathLineEdit->text();
    }

    QList<Okular::PixmapRequest *> pixmapRequestList() const
    {
        return m_pixmapRequestList;
    }

private:
    Okular::Document *m_document;

    QLabel *m_imageTypeLabel;
    QLabel *m_PNGTypeLabel;

    QLabel *m_dirPathLabel;
    QLineEdit *m_dirPathLineEdit;

    QGroupBox *m_exportRangeGroupBox;
    QRadioButton *m_allPagesRadioButton;
    QRadioButton *m_pageRangeRadioButton;
    QRadioButton *m_customPageRadioButton;
    QSpinBox *m_pageStartSpinBox;
    QSpinBox *m_pageEndSpinBox;
    QLabel *m_toLabel;
    QLineEdit *m_customPageRangeLineEdit;

    QPushButton *m_dirPathBrowseButton;

    QList<Okular::PixmapRequest *> m_pixmapRequestList;

private Q_SLOTS:
    void browseClicked();
    void okClicked();
};

#endif // _OKULAR_EXPORTIMAGEDIALOG_H_
