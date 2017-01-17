/***************************************************************************
 *   Copyright (C) 2017 by Gilbert Assaf <gassaf@gmx.de>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef GENERATOR_PDFIUM_H
#define GENERATOR_PDFIUM_H

#include <fpdfview.h>

#include <core/document.h>
#include <core/generator.h>

class PDFiumGenerator : public Okular::Generator
{
    Q_OBJECT
    Q_INTERFACES ( Okular::Generator )
public:
    PDFiumGenerator ( QObject* parent, const QVariantList& args );
    bool loadDocument ( const QString& fileName, QVector< Okular::Page* >& pagesVector ) Q_DECL_OVERRIDE;
    Okular::DocumentInfo generateDocumentInfo ( const QSet<Okular::DocumentInfo::Key>& keys ) const Q_DECL_OVERRIDE;
    QImage image ( Okular::PixmapRequest* page ) Q_DECL_OVERRIDE;
    QVariant metaData ( const QString& key, const QVariant& option ) const Q_DECL_OVERRIDE;
    
protected:
    bool doCloseDocument() Q_DECL_OVERRIDE;
    Okular::TextPage* textPage( Okular::Page *page ) Q_DECL_OVERRIDE;
    
private:
    QString getMetaText ( const QString& tag ) const;
    QString getPageLabel ( int page_index ) const;
    
    FPDF_DOCUMENT pdfdoc;
};

#endif // GENERATOR_PDFIUM_H
