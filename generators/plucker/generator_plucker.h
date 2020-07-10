/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_PLUCKER_H_
#define _OKULAR_GENERATOR_PLUCKER_H_

#include <core/document.h>
#include <core/generator.h>

#include <QTextBlock>

#include "qunpluck.h"

class QTextDocument;

class PluckerGenerator : public Okular::Generator
{
    Q_OBJECT
    Q_INTERFACES(Okular::Generator)

public:
    PluckerGenerator(QObject *parent, const QVariantList &args);
    ~PluckerGenerator() override;

    // [INHERITED] load a document and fill up the pagesVector
    bool loadDocument(const QString &fileName, QVector<Okular::Page *> &pagesVector) override;

    // [INHERITED] document information
    Okular::DocumentInfo generateDocumentInfo(const QSet<Okular::DocumentInfo::Key> &keys) const override;

    // [INHERITED] perform actions on document / pages
    QImage image(Okular::PixmapRequest *request) override;

    // [INHERITED] text exporting
    Okular::ExportFormat::List exportFormats() const override;
    bool exportTo(const QString &fileName, const Okular::ExportFormat &format) override;

    // [INHERITED] print document using already configured kprinter
    bool print(QPrinter &printer) override;

protected:
    bool doCloseDocument() override;

private:
    QList<QTextDocument *> mPages;
    QSet<int> mLinkAdded;
    Link::List mLinks;
    Okular::DocumentInfo mDocumentInfo;
};

#endif
