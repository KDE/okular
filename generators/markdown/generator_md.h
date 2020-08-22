/***************************************************************************
 *   Copyright (C) 2017 by Julian Wolff <wolff@julianwolff.de>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_MD_H_
#define _OKULAR_GENERATOR_MD_H_

#include <core/textdocumentgenerator.h>

class MarkdownGenerator : public Okular::TextDocumentGenerator
{
    Q_OBJECT
    Q_INTERFACES(Okular::Generator)

public:
    MarkdownGenerator(QObject *parent, const QVariantList &args);

    // [INHERITED] reparse configuration
    bool reparseConfig() override;
    void addPages(KConfigDialog *dlg) override;

private:
    bool m_isFancyPantsConfigEnabled = true;
    bool m_wasFancyPantsConfigEnabled = true;
};

#endif
