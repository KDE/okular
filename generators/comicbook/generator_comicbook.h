/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef GENERATOR_COMICBOOK_H
#define GENERATOR_COMICBOOK_H

#include <core/generator.h>

#include "document.h"

class ComicBookGenerator : public Okular::Generator
{
    Q_OBJECT
    Q_INTERFACES(Okular::Generator)

public:
    ComicBookGenerator(QObject *parent, const QVariantList &args);
    ~ComicBookGenerator() override;

    // [INHERITED] load a document and fill up the pagesVector
    bool loadDocument(const QString &fileName, QVector<Okular::Page *> &pagesVector) override;

    // [INHERITED] print document using already configured kprinter
    bool print(QPrinter &printer) override;

protected:
    bool doCloseDocument() override;
    QImage image(Okular::PixmapRequest *request) override;

private:
    ComicBook::Document mDocument;
};

#endif
