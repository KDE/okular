/***************************************************************************
 *   Copyright (C) 2006-2008 by Albert Astals Cid <aacid@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef POPPLEREMBEDDEDFILE_H
#define POPPLEREMBEDDEDFILE_H

#include <poppler-qt4.h>

#include <core/document.h>

class PDFEmbeddedFile : public Okular::EmbeddedFile
{
    public:
        PDFEmbeddedFile(Poppler::EmbeddedFile *f) : ef(f)
        {
        }
        
        QString name() const
        {
            return ef->name();
        }
        
        QString description() const
        {
            return ef->description();
        }
        
        QByteArray data() const
        {
            return ef->data();
        }
        
        int size() const
        {
            int s = ef->size();
            return s <= 0 ? -1 : s;
        }
        
        QDateTime modificationDate() const
        {
            return ef->modDate();
        }
        
        QDateTime creationDate() const
        {
            return ef->createDate();
        }
    
    private:
        Poppler::EmbeddedFile *ef;
};

#endif

