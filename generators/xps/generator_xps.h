/*
  Copyright (C)  2006  Brad Hards <bradh@frogmouth.net>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#ifndef _OKULAR_GENERATOR_XPS_H_
#define _OKULAR_GENERATOR_XPS_H_

#include "core/generator.h"
#include <QDomDocument>
#include <QXmlDefaultHandler>

#include <kzip.h>

class XpsPage;

class XpsHandler: public QXmlDefaultHandler
{
public:
    XpsHandler( XpsPage *page );
    ~XpsHandler();

    bool startElement( const QString & nameSpace,
                       const QString & localName,
                       const QString & qname,
                       const QXmlAttributes & atts );
    bool endElement( const QString & nameSpace,
                     const QString & localName,
                     const QString & qname );
    bool startDocument();

private:
    /**
        Parse an abbreviated path "Data" description
        \param data the string containing the whitespace separated values
        
        \note sets the m_currentPath and possibly other private variables
    
        \see XPS specification 4.2.3 and Appendix G
    */
    void parseAbbreviatedPathData( const QString &data);
    
    /**
        Parse a "Matrix" attribute string
        \param csv the comma separated list of values
        \return the QMatrix corresponding to the affine transform
        given in the attribute
    
        \see XPS specification 7.4.1
    */
    QMatrix attsToMatrix( const QString &csv );
    
    XpsPage *m_page;
    QPainter *m_painter;
    
    QBrush m_currentBrush;
    QPen m_currentPen;
    QPainterPath m_currentPath;
    
    QImage m_image;
    QRectF m_viewbox;
    QRectF m_viewport;
};

class XpsPage
{
public:
    XpsPage(KZip *archive, const QString &fileName);
    ~XpsPage();

    QSize size() const;
    bool renderToImage( QImage *p );
    
    QImage loadImageFromFile( const QString &filename );
    int loadFontByName( const QString &fontName );
    
private:
    KZip *m_archive;
    const QString m_fileName;
    QDomDocument m_dom;

    QSize m_pageSize;
    
    QFontDatabase m_fontDatabase;

    QString m_thumbnailFileName;
    bool m_thumbnailMightBeAvailable;
    QImage m_thumbnail;
    bool m_thumbnailIsLoaded;

    QImage *m_pageImage;
    bool m_pageIsRendered;
    
    friend class XpsHandler;
};

/**
   Represents one of the (perhaps the only) documents in an XpsFile
*/
class XpsDocument
{
public:
    XpsDocument(KZip *archive, const QString &fileName);
    ~XpsDocument();

    /**
       the total number of pages in this document
    */
    int numPages() const;

    /**
       obtain a certain page from this document

       \param pageNum the number of the page to return

       \note page numbers are zero based - they run from 0 to 
       numPages() - 1
    */
    XpsPage* page(int pageNum) const;

private:
    QList<XpsPage*> m_pages;
};

/**
   Represents the contents of a Microsoft XML Paper Specification
   format document.
*/
class XpsFile
{
public:
    XpsFile();
    ~XpsFile();
    
    bool loadDocument( const QString & fileName );
    bool closeDocument();

    const Okular::DocumentInfo * generateDocumentInfo();

    QImage thumbnail();

    /**
       the total number of XpsDocuments with this file
    */
    int numDocuments() const;

    /**
       the total number of pages in all the XpsDocuments within this
       file
    */
    int numPages() const;

    /**
       a page from the file

        \param pageNum the page number of the page to return

        \note page numbers are zero based - they run from 0 to 
        numPages() - 1
    */
    XpsPage* page(int pageNum) const;

    /**
       obtain a certain document from this file

       \param documentNum the number of the document to return

       \note document numbers are zero based - they run from 0 to 
       numDocuments() - 1
    */
    XpsDocument* document(int documentNum) const;
private:
    QList<XpsDocument*> m_documents;
    QList<XpsPage*> m_pages;

    QString m_thumbnailFileName;
    bool m_thumbnailMightBeAvailable;
    QImage m_thumbnail;
    bool m_thumbnailIsLoaded;

    QString m_corePropertiesFileName;
    Okular::DocumentInfo * m_docInfo;

    KZip *xpsArchive;
};


class XpsGenerator : public Okular::Generator
{
    Q_OBJECT
    public:
        XpsGenerator();
        virtual ~XpsGenerator();

        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
        bool closeDocument();

        bool canGeneratePixmap( bool async ) const;
        void generatePixmap( Okular::PixmapRequest * request );

        const Okular::DocumentInfo * generateDocumentInfo();

        bool supportsRotation() const { return true; };

    private slots:

    private:
        XpsFile *m_xpsFile;
};

#endif
