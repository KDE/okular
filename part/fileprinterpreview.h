/*
    SPDX-FileCopyrightText: 2007 John Layt <john@layt.net>

    FilePrinterPreview based on KPrintPreview (originally LGPL)
    SPDX-FileCopyrightText: 2007 Alex Merry <huntedhacker@tiscali.co.uk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef FILEPRINTERPREVIEW_H
#define FILEPRINTERPREVIEW_H

#include <QDialog>

namespace Okular
{
// This code copied from KPrintPreview by Alex Merry, adapted to do PS files instead of PDF

class FilePrinterPreviewPrivate;

class FilePrinterPreview : public QDialog
{
    Q_OBJECT

public:
    /**
     * Create a Print Preview dialog for a given file.
     *
     * @param filename file to print preview
     * @param parent  pointer to the parent widget for the dialog
     */
    explicit FilePrinterPreview(const QString &filename, QWidget *parent = nullptr);
    ~FilePrinterPreview() override;

    QSize sizeHint() const override;

protected:
    void showEvent(QShowEvent *event) override;

private:
    FilePrinterPreviewPrivate *const d;
};

}

#endif // FILEPRINTER_H
