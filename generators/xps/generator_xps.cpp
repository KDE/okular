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
  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#include <qdatetime.h>
#include <qfile.h>
#include <qimage.h>
#include <qlist.h>
#include <qpixmap.h>
#include <qthread.h>
#include <kglobal.h>
#include <kimageeffect.h>
#include <klocale.h>

#include "core/page.h"
#include "generator_xps.h"

OKULAR_EXPORT_PLUGIN(XpsGenerator);


XpsPage::XpsPage(KZip *archive, const QString &fileName)
{
    kDebug() << "page file name: " << fileName << endl;

    KZipFileEntry* pageFile = static_cast<const KZipFileEntry *>(archive->directory()->entry( fileName ));

    QIODevice* pageDevice = pageFile->device();

    QString errMsg;
    int errLine, errCol;
    if ( m_dom.setContent( pageDevice, true, &errMsg, &errLine, &errCol ) == false ) {
        // parse error
        kDebug() << "Could not parse XPS page: " << errMsg << " : "
                 << errLine << " : " << errCol << endl;
    }

    QDomNode node = m_dom.documentElement();
    QDomElement element = node.toElement();
    m_pageSize.setWidth( element.attribute("Width").toInt() );
    m_pageSize.setHeight( element.attribute("Height").toInt() );

    delete pageDevice;
}

QSize XpsPage::size() const
{
    return m_pageSize;
}

XpsDocument::XpsDocument(KZip *archive, const QString &fileName)
{
    KZipFileEntry* documentFile = static_cast<const KZipFileEntry *>(archive->directory()->entry( fileName ));

    QIODevice* documentDevice = documentFile->device();

    QDomDocument documentDom;
    QString errMsg;
    int errLine, errCol;
    if ( documentDom.setContent( documentDevice, true, &errMsg, &errLine, &errCol ) == false ) {
        // parse error
        kDebug() << "Could not parse XPS document: " << errMsg << " : "
                 << errLine << " : " << errCol << endl;
    }

    QDomNode node = documentDom.documentElement().firstChild();

    while( !node.isNull() ) {
        QDomElement element = node.toElement();
        if( !element.isNull() ) {
            if (element.tagName() == "PageContent") {
                XpsPage *page = new XpsPage( archive, element.attribute("Source") );
                m_pages.append(page);
            } else {
                kDebug() << "Unhandled entry in FixedDocument" << element.tagName() << endl;
            }
        }
        node = node.nextSibling();
    }

    delete documentDevice;
}

int XpsDocument::numPages() const
{
    return m_pages.size();
}

XpsPage* XpsDocument::page(int pageNum) const
{
    return m_pages.at(pageNum);
}

XpsFile::XpsFile()
{
};


XpsFile::~XpsFile()
{
};


bool XpsFile::loadDocument(const QString &filename)
{
    xpsArchive = new KZip( filename );
    if ( xpsArchive->open( IO_ReadOnly ) == true ) {
        kDebug() << "Successful open of " << xpsArchive->fileName() << endl;
    } else {
        kDebug() << "Could not open XPS archive: " << xpsArchive->fileName() << endl;
        delete xpsArchive;
        return false;
    }

    // The only fixed entry in XPS is _rels/.rels
    KZipFileEntry* relFile = static_cast<const KZipFileEntry *>(xpsArchive->directory()->entry("_rels/.rels"));

    if ( !relFile ) {
        // this might occur if we can't read the zip directory, or it doesn't have the relationships entry
        return false;
    }

    QIODevice* relDevice = relFile->device();
    QDomDocument relDom;
    QString errMsg;
    int errLine, errCol;
    if ( relDom.setContent( relDevice, true, &errMsg, &errLine, &errCol ) == false ) {
        // parse error
        kDebug() << "Could not parse relationship document: " << errMsg << " : "
                     << errLine << " : " << errCol << endl;
        return false;
    }

    QString fixedRepresentationFileName;
    // We work through the relationships document and pull out each element.
    QDomNode n = relDom.documentElement().firstChild();
    while( !n.isNull() ) {
        QDomElement e = n.toElement();
        if( !e.isNull() ) {
            if ("http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail" == e.attribute("Type") ) {
                m_thumbnailFileName = e.attribute("Target");
            } else if ("http://schemas.microsoft.com/xps/2005/06/fixedrepresentation" == e.attribute("Type") ) {
                fixedRepresentationFileName = e.attribute("Target");
            } else if ("http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" == e.attribute("Type") ) {
                m_corePropertiesFileName = e.attribute("Target");
            } else {
                kDebug() << "Unknown relationships element: " << e.attribute("Type") << " : " << e.attribute("Target") << endl;
            }
        }
        n = n.nextSibling();
    }

       if ( fixedRepresentationFileName.isEmpty() ) {
        // FixedRepresentation is a required part of the XPS document
        return false;
    }

    delete relDevice;


    KZipFileEntry* fixedRepFile = static_cast<const KZipFileEntry *>(xpsArchive->directory()->entry( fixedRepresentationFileName ));

    QIODevice* fixedRepDevice = fixedRepFile->device();

    QDomDocument fixedRepDom;
    if ( fixedRepDom.setContent( fixedRepDevice, true, &errMsg, &errLine, &errCol ) == false ) {
        // parse error
        kDebug() << "Could not parse Fixed Representation document: " << errMsg << " : "
                     << errLine << " : " << errCol << endl;
        return false;
    }

    n = fixedRepDom.documentElement().firstChild();
    while( !n.isNull() ) {
        QDomElement e = n.toElement();
        if( !e.isNull() ) {
            if (e.tagName() == "DocumentReference") {
                XpsDocument *doc = new XpsDocument( xpsArchive, e.attribute("Source") );
                m_documents.append(doc);
            } else {
                kDebug() << "Unhandled entry in FixedDocumentSequence" << e.tagName() << endl;
            }
        }
        n = n.nextSibling();
    }

    delete fixedRepDevice;

    kDebug() << "Parsed stage 1" << endl;
    return true;
}

const DocumentInfo * XpsFile::generateDocumentInfo()
{
    if ( m_docInfo )
        return m_docInfo;

    m_docInfo = new DocumentInfo();

    m_docInfo->set( "mimeType", "application/vnd.ms-xpsdocument" );

    if ( ! m_corePropertiesFileName.isEmpty() ) {
        KZipFileEntry* corepropsFile = static_cast<const KZipFileEntry *>(xpsArchive->directory()->entry(m_corePropertiesFileName));

        QDomDocument corePropertiesDocumentDom;
        QString errMsg;
        int errLine, errCol;

        QIODevice *corepropsDevice = corepropsFile->device();

        if ( corePropertiesDocumentDom.setContent( corepropsDevice, true, &errMsg, &errLine, &errCol ) == false ) {
            // parse error
            kDebug() << "Could not parse core properties (metadata) document: " << errMsg << " : "
                     << errLine << " : " << errCol << endl;
            // return whatever we have
            return m_docInfo;
        }

        QDomNode n = corePropertiesDocumentDom.documentElement().firstChild(); // the <coreProperties> level
        while( !n.isNull() ) {
            QDomElement e = n.toElement();
            if( !e.isNull() ) {
                if (e.tagName() == "title") {
                    m_docInfo->set( "title", e.text(), i18n("Title") );
                } else if (e.tagName() == "subject") {
                    m_docInfo->set( "subject", e.text(), i18n("Subject") );
                } else if (e.tagName() == "description") {
                    m_docInfo->set( "description", e.text(), i18n("Description") );
                } else if (e.tagName() == "creator") {
                    m_docInfo->set( "creator", e.text(), i18n("Author") );
                } else if (e.tagName() == "created") {
                    QDateTime createdDate = QDateTime::fromString( e.text(), "yyyy-MM-ddThh:mm:ssZ" );
                    m_docInfo->set( "creationDate", KGlobal::locale()->formatDateTime( createdDate, false, true ), i18n("Created" ) );
                } else if (e.tagName() == "modified") {
                    QDateTime modifiedDate = QDateTime::fromString( e.text(), "yyyy-MM-ddThh:mm:ssZ" );
                    m_docInfo->set( "modifiedDate", KGlobal::locale()->formatDateTime( modifiedDate, false, true ), i18n("Modified" ) );
                } else if (e.tagName() == "keywords") {
                    m_docInfo->set( "keywords", e.text(), i18n("Keywords") );
                } else {
                    kDebug() << "unhandled metadata tag: " << e.tagName() << " : " << e.text() << endl;
                }
            }
            n = n.nextSibling();
        }

        delete corepropsDevice;
    } else {
        kDebug() << "No core properties filename" << endl;
    }

    m_docInfo->set( "pages", QString::number(numPages()), i18n("Pages") );

    return m_docInfo;
}

bool XpsFile::closeDocument()
{

    if ( m_docInfo )
        delete m_docInfo;

    m_docInfo = 0;

    m_documents.clear();

    delete xpsArchive;

    return true;
}

int XpsFile::numPages() const
{
    int pages = 0;

    foreach( const XpsDocument *doc,  m_documents ) {
        pages += doc->numPages();
    }

    return pages;
}

int XpsFile::numDocuments() const
{
    return m_documents.size();
}

XpsDocument* XpsFile::document(int documentNum) const
{
    return m_documents.at(documentNum);
}

XpsGenerator::XpsGenerator( KPDFDocument * document ) : Generator( document )
{
    m_xpsFile = new XpsFile;
}

XpsGenerator::~XpsGenerator()
{
    delete m_xpsFile;
}

bool XpsGenerator::loadDocument( const QString & fileName, QVector<KPDFPage*> & pagesVector )
{
    m_xpsFile->loadDocument( fileName );
    pagesVector.resize( m_xpsFile->numPages() );

    int pagesVectorOffset = 0;

    for (int docNum = 0; docNum < m_xpsFile->numDocuments(); ++docNum )
    {
        XpsDocument *doc = m_xpsFile->document( docNum );
        for (int pageNum = 0; pageNum < doc->numPages(); ++pageNum )
        {
            QSize pageSize = doc->page( pageNum )->size();
            pagesVector[pagesVectorOffset] = new KPDFPage( pagesVectorOffset, pageSize.width(), pageSize.height(), 0 );
            ++pagesVectorOffset;
        }
    }

    return true;
}

bool XpsGenerator::closeDocument()
{
    m_xpsFile->closeDocument();

    return true;
}

bool XpsGenerator::canGeneratePixmap( bool /*async*/ )
{
    return true; // ???
}

void XpsGenerator::generatePixmap( PixmapRequest * request )
{
    QPixmap * p = new QPixmap( request->width, request->height );

#if 0
    if ( TIFFSetDirectory( d->tiff, request->page->number() ) )
    {
        int rotation = request->documentRotation;
        uint32 width = (uint32)request->page->width();
        uint32 height = (uint32)request->page->height();
        if ( rotation % 2 == 1 )
            qSwap( width, height );

        QImage image( width, height, QImage::Format_RGB32 );
        uint32 * data = (uint32 *)image.bits();

        // read data
        if ( TIFFReadRGBAImageOriented( d->tiff, width, height, data, ORIENTATION_TOPLEFT ) != 0 )
        {
            // an image read by ReadRGBAImage is ABGR, we need ARGB, so swap red and blue
            uint32 size = width * height;
            for ( uint32 i = 0; i < size; ++i )
            {
                uint32 red = ( data[i] & 0x00FF0000 ) >> 16;
                uint32 blue = ( data[i] & 0x000000FF ) << 16;
                data[i] = ( data[i] & 0xFF00FF00 ) + red + blue;
            }

            int reqwidth = request->width;
            int reqheight = request->height;
            if ( rotation % 2 == 1 )
                qSwap( reqwidth, reqheight );
            QImage smoothImage = image.scaled( reqwidth, reqheight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
            QImage finalImage = rotation > 0
                ? KImageEffect::rotate( smoothImage, (KImageEffect::RotateDirection)( rotation - 1 ) )
                : smoothImage;
            *p = QPixmap::fromImage( finalImage );

            generated = true;
        }
    }

    if ( !generated )
    {
        p->fill();
    }

    request->page->setPixmap( request->id, p );

    ready = true;

    // signal that the request has been accomplished
    signalRequestDone( request );
#endif
}

const DocumentInfo * XpsGenerator::generateDocumentInfo()
{
    kDebug() << "generating document metadata" << endl;

    return m_xpsFile->generateDocumentInfo();
}


void XpsGenerator::setOrientation( QVector<KPDFPage*> & pagesVector, int orientation )
{
    // TODO
}

#include "generator_xps.moc"

