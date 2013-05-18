/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_OOO_H_
#define _OKULAR_GENERATOR_OOO_H_

#include <core/textdocumentgenerator.h>

class KOOOGenerator : public Okular::TextDocumentGenerator
{
  public:
    KOOOGenerator( QObject *parent, const QVariantList &args );

    // [INHERITED] reparse configuration
    void addPages( KConfigDialog* dlg );
};

#endif
