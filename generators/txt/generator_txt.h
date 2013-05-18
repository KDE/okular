/***************************************************************************
 *   Copyright (C) 2013 by Azat Khuzhin <a3at.mail@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#ifndef _TXT_GENERATOR_H_
#define _TXT_GENERATOR_H_


#include <core/textdocumentgenerator.h>

class TxtGenerator : public Okular::TextDocumentGenerator
{
    public:
        TxtGenerator( QObject *parent, const QVariantList &args );
        ~TxtGenerator() {}

        // [INHERITED] reparse configuration
        void addPages( KConfigDialog* dlg );
};

#endif