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

#include <okular/core/generator.h>
#include <okular/core/textpage.h>

#include <QDomDocument>
#include <QXmlStreamReader>
#include <QXmlDefaultHandler>
#include <QStack>

#include <kzip.h>

typedef enum {abtCommand, abtNumber, abtComma, abtEOF} AbbPathTokenType;

class AbbPathToken {
public:
    QString data;
    int curPos;

    AbbPathTokenType type;
    char command;
    double number;
};

/**
    Holds information about xml element during SAX parsing of page
*/
class XpsRenderNode
{
public:
    QString name;
    QVector<XpsRenderNode> children;
    QXmlAttributes attributes;
    void * data;

    XpsRenderNode * findChild( const QString &name );
    void * getRequiredChildData( const QString &name );
    void * getChildData( const QString &name );
};

/**
    Types of data in XpsRenderNode::data. Name of each type consist of Xps and
    name of xml element which data it holds
*/
typedef QMatrix XpsMatrixTransform;
typedef QMatrix XpsRenderTransform;
typedef QBrush XpsFill;

class XpsPage;
class XpsFile;

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

protected:
    XpsPage *m_page;

    void processStartElement( XpsRenderNode &node );
    void processEndElement( XpsRenderNode &node );

    // Methods for processing of diferent xml elements
    void processGlyph( XpsRenderNode &node );
    void processPath( XpsRenderNode &node );
    void processFill( XpsRenderNode &node );
    void processImageBrush (XpsRenderNode &node );

    QPainter *m_painter;

    QImage m_image;

    QStack<XpsRenderNode> m_nodes;

    friend class XpsPage;
};

class XpsPage
{
public:
    XpsPage(XpsFile *file, const QString &fileName);
    ~XpsPage();

    QSize size() const;
    bool renderToImage( QImage *p );
    Okular::TextPage* textPage();

    QImage loadImageFromFile( const QString &filename );

private:
    XpsFile *m_file;
    const QString m_fileName;

    QSize m_pageSize;


    QString m_thumbnailFileName;
    bool m_thumbnailMightBeAvailable;
    QImage m_thumbnail;
    bool m_thumbnailIsLoaded;

    QImage *m_pageImage;
    bool m_pageIsRendered;

    friend class XpsHandler;
    friend class XpsTextExtractionHandler;
};

/**
   Represents one of the (perhaps the only) documents in an XpsFile
*/
class XpsDocument
{
public:
    XpsDocument(XpsFile *file, const QString &fileName);
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

    /**
      whether this document has a Document Structure
    */
    bool hasDocumentStructure();

    /**
      the document structure for this document, if available
    */
    const Okular::DocumentSynopsis * documentStructure();

private:
    void parseDocumentStructure( const QString &documentStructureFileName );

    QList<XpsPage*> m_pages;
    XpsFile * m_file;
    bool m_haveDocumentStructure;
    Okular::DocumentSynopsis *m_docStructure;
    QMap<QString,int> m_docStructurePageMap;
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

    QFont getFontByName( const QString &fontName, float size );

    KZip* xpsArchive();


private:
    int loadFontByName( const QString &fontName );

    QList<XpsDocument*> m_documents;
    QList<XpsPage*> m_pages;

    QString m_thumbnailFileName;
    bool m_thumbnailMightBeAvailable;
    QImage m_thumbnail;
    bool m_thumbnailIsLoaded;

    QString m_corePropertiesFileName;
    Okular::DocumentInfo * m_docInfo;

    QString m_signatureOrigin;

    KZip * m_xpsArchive;

    QMap<QString, int> m_fontCache;
    QFontDatabase m_fontDatabase;
};


class XpsGenerator : public Okular::Generator
{
    Q_OBJECT
    public:
        XpsGenerator();
        virtual ~XpsGenerator();

        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
        bool closeDocument();

        const Okular::DocumentInfo * generateDocumentInfo();
        const Okular::DocumentSynopsis * generateDocumentSynopsis();

        Okular::ExportFormat::List exportFormats() const;
        bool exportTo( const QString &fileName, const Okular::ExportFormat &format );
    protected:
        QImage image( Okular::PixmapRequest *page );
        Okular::TextPage* textPage( Okular::Page * page );

    private:
        XpsFile *m_xpsFile;
};

#endif
