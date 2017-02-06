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

#include <qbitarray.h>
#include <QLinkedList>

#include <fpdfview.h>

#include <core/document.h>
#include <core/generator.h>

class PDFiumGenerator : public Okular::Generator
{
    Q_OBJECT
    Q_INTERFACES ( Okular::Generator )
public:
    PDFiumGenerator ( QObject* parent, const QVariantList& args );
    Okular::Document::OpenResult loadDocumentWithPassword( const QString & fileName, QVector<Okular::Page*> & pagesVector, const QString & password ) Q_DECL_OVERRIDE;
    Okular::DocumentInfo generateDocumentInfo ( const QSet<Okular::DocumentInfo::Key>& keys ) const Q_DECL_OVERRIDE;
    const Okular::DocumentSynopsis * generateDocumentSynopsis() Q_DECL_OVERRIDE;
    QImage image ( Okular::PixmapRequest* page ) Q_DECL_OVERRIDE;
    QVariant metaData ( const QString& key, const QVariant& option ) const Q_DECL_OVERRIDE;
    
protected:
    bool doCloseDocument() Q_DECL_OVERRIDE;
    Okular::TextPage* textPage( Okular::Page *page ) Q_DECL_OVERRIDE;
    
private:
    QString getMetaText ( const QString& tag ) const;
    QString getPageLabel ( int page_index ) const;
    QLinkedList<Okular::ObjectRect*> generateObjectRects ( int page_index );
    Okular::NormalizedRect generateRectangle ( double x1, double y1, double x2, double y2, double pageWidth, double pageHeight );
    Okular::Action* createAction ( FPDF_ACTION action, unsigned long type );
    void addSynopsisChildren(FPDF_BOOKMARK bookmark, QDomNode * parentDestination);
    
    FPDF_DOCUMENT pdfdoc;
    QBitArray rectsGenerated;
    Okular::DocumentSynopsis docSyn;
};

#endif // GENERATOR_PDFIUM_H
