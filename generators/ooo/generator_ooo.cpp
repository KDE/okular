/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "converter.h"
#include "generator_ooo.h"

OKULAR_EXPORT_PLUGIN(KOOOGenerator)

KOOOGenerator::KOOOGenerator()
  : Okular::TextDocumentGenerator( new OOO::Converter )
{
}
