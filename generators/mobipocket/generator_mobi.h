/***************************************************************************
 *   Copyright (C) 2008 by Ely Levy <elylevy@cs.huji.ac.il>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef OKULAR_GENERATOR_MOBI_H
#define OKULAR_GENERATOR_MOBI_H
#include <core/textdocumentgenerator.h>

class MobiGenerator : public Okular::TextDocumentGenerator
{
    Q_OBJECT
    Q_INTERFACES(Okular::Generator)

public:
    MobiGenerator(QObject *parent, const QVariantList &args);
    ~MobiGenerator() override
    {
    }

    // [INHERITED] reparse configuration
    void addPages(KConfigDialog *dlg) override;
};

#endif
