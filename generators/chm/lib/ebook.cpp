/*
 *  Kchmviewer - a CHM and EPUB file viewer with broad language support
 *  Copyright (C) 2004-2014 George Yunaev, gyunaev@ulduzsoft.com
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ebook.h"
#include "ebook_chm.h"
#include "ebook_epub.h"

EBook::EBook()
{
}

EBook::~EBook()
{
}

EBook *EBook::loadFile(const QString &archiveName)
{
    EBook_CHM *cbook = new EBook_CHM();

    if (cbook->load(archiveName))
        return cbook;

    delete cbook;

    EBook_EPUB *ebook = new EBook_EPUB();

    if (ebook->load(archiveName))
        return ebook;

    delete ebook;
    return nullptr;
}
