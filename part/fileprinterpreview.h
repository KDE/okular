/***************************************************************************
 *   Copyright (C) 2007 by John Layt <john@layt.net>                       *
 *                                                                         *
 *   FilePrinterPreview based on KPrintPreview (originally LGPL)           *
 *   Copyright (c) 2007 Alex Merry <huntedhacker@tiscali.co.uk>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
