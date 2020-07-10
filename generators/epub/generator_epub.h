/***************************************************************************
 *   Copyright (C) 2008 by Ely Levy <elylevy@cs.huji.ac.il>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_EPUB_H_
#define _OKULAR_GENERATOR_EPUB_H_
#include <core/textdocumentgenerator.h>

class EPubGenerator : public Okular::TextDocumentGenerator
{
    Q_OBJECT
    Q_INTERFACES(Okular::Generator)
public:
    EPubGenerator(QObject *parent, const QVariantList &args);
    ~EPubGenerator() override;

    // [INHERITED] reparse configuration
    void addPages(KConfigDialog *dlg) override;
};

#endif
