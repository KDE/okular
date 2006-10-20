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
#include <qfontdatabase.h>
#include <qimage.h>
#include <qlist.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qthread.h>
#include <kglobal.h>
#include <kimageeffect.h>
#include <klocale.h>

#include "core/page.h"
#include "generator_xps.h"

OKULAR_EXPORT_PLUGIN(XpsGenerator)

// From Qt4
static int hex2int(char hex)
{
    QChar hexchar = QLatin1Char(hex);
    int v;
    if (hexchar.isDigit())
        v = hexchar.digitValue();
    else if (hexchar >= QLatin1Char('A') && hexchar <= QLatin1Char('F'))
        v = hexchar.cell() - 'A' + 10;
    else if (hexchar >= QLatin1Char('a') && hexchar <= QLatin1Char('f'))
        v = hexchar.cell() - 'a' + 10;
    else
        v = -1;
    return v;
}

// Modified from Qt4
static QColor hexToRgba(const char *name)
{
    if(name[0] != '#')
        return QColor();
    name++; // eat the leading '#'
    int len = qstrlen(name);
    int r, g, b;
    int a = 255;
    if (len == 6) {
        r = (hex2int(name[0]) << 4) + hex2int(name[1]);
        g = (hex2int(name[2]) << 4) + hex2int(name[3]);
        b = (hex2int(name[4]) << 4) + hex2int(name[5]);
    } else if (len == 8) {
        a = (hex2int(name[0]) << 4) + hex2int(name[1]);
        r = (hex2int(name[2]) << 4) + hex2int(name[3]);
        g = (hex2int(name[4]) << 4) + hex2int(name[5]);
        b = (hex2int(name[6]) << 4) + hex2int(name[7]);
    } else {
        r = g = b = -1;
    }
    if ((uint)r > 255 || (uint)g > 255 || (uint)b > 255) {
        return QColor();
    }
    return QColor(r,g,b,a);
}

static QRectF stringToRectF( const QString &data )
{
    QStringList numbers = data.split(',');
    QPointF origin( numbers.at(0).toDouble(), numbers.at(1).toDouble() );
    QSizeF size( numbers.at(2).toDouble(), numbers.at(3).toDouble() );
    return QRectF( origin, size );
}

static QPointF getPointFromString( const QString &data, int *curPos )
{
    // find the first ',' after the current position
    int endOfNumberPos = data.indexOf(',', *curPos);
    QString numberPart = data.mid(*curPos, endOfNumberPos - *curPos);
    // kDebug() << "Number part 1: " << numberPart << endl;
    double firstVal = numberPart.toDouble();
    *curPos = endOfNumberPos + 1; //eat the number and the following comma

    endOfNumberPos = data.indexOf(' ', *curPos);
    numberPart = data.mid (*curPos, endOfNumberPos - *curPos);
    // kDebug() << "Number part 2: " << numberPart << endl;
    double secondVal = numberPart.toDouble();
    *curPos = endOfNumberPos;
    // kDebug() << "firstVal: " << firstVal << endl;
    // kDebug() << "secondVal: " << secondVal << endl;
    return QPointF( firstVal, secondVal );
}

void XpsHandler::parseAbbreviatedPathData( const QString &data)
{
    // kDebug() << data << endl;

    enum OperationType { moveTo, relMoveTo, lineTo, relLineTo, cubicTo, relCubicTo };
    OperationType operation = moveTo;
    // TODO: Implement a proper parser here.
    for (int curPos = 0; curPos < data.length(); ++curPos) {
        // kDebug() << "curPos: " << curPos << endl;
        if (data.at(curPos) == 'M') {
            // kDebug() << "operation moveTo" << endl;
            operation = moveTo;
        } else if (data.at(curPos) == 'L') {
            // kDebug() << "operation lineTo" << endl;
            operation = lineTo;
        } else if (data.at(curPos) == 'C') {
            operation = cubicTo;
        } else if ( (data.at(curPos) == 'z') || (data.at(curPos) == 'Z') ){
            // kDebug() << "operation close" << endl;
            m_currentPath.closeSubpath();
        } else if (data.at(curPos).isNumber()) {
            if ( operation == moveTo ) {
                QPointF point = getPointFromString( data, &curPos );
                m_currentPath.moveTo( point );
            } else if ( operation == lineTo ) {
                QPointF point = getPointFromString( data, &curPos );
                m_currentPath.lineTo( point );
            } else if (operation == cubicTo ) {
                QPointF point1 = getPointFromString( data, &curPos );
                while ((data.at(curPos).isSpace())) { ++curPos; }
                QPointF point2 = getPointFromString( data, &curPos );
                while (data.at(curPos).isSpace()) { curPos++; }
                QPointF point3 = getPointFromString( data, &curPos );
                // kDebug() << "cubic" << point1 << " : " << point2 << " : " << point3 << endl;
                m_currentPath.cubicTo( point1, point2, point3 );
            }
        } else if (data.at(curPos) == ' ') {
            // Do nothing
            // kDebug() << "eating a space" << endl;
        } else {
            kDebug() << "Unexpected data: " << data.at(curPos) << endl;
        }
    }
}
QMatrix XpsHandler::attsToMatrix( const QString &csv )
{
    QStringList values = csv.split( ',' );
    if ( values.count() != 6 ) {
        return QMatrix(); // that is an identity matrix - no effect
    } 
    return QMatrix( values.at(0).toDouble(), values.at(1).toDouble(),
                    values.at(2).toDouble(), values.at(3).toDouble(),
                    values.at(4).toDouble(), values.at(5).toDouble() );
}

XpsHandler::XpsHandler(XpsPage *page): m_page(page)
{
}

XpsHandler::~XpsHandler()
{}

bool XpsHandler::startDocument()
{
    // kDebug() << "start document" << endl;
    m_page->m_pageImage->fill( QColor("White").rgba() ); 
    m_painter = new QPainter(m_page->m_pageImage);
    return true;
}


bool XpsHandler::startElement( const QString &nameSpace,
                               const QString &localName,
                               const QString &qname,
                               const QXmlAttributes & atts )
{
    if (localName == "Glyphs") {
        m_painter->save();
        int fontId = m_page->loadFontByName( atts.value("FontUri") );
        // kDebug() << "Font families: (" << fontId << ") " << QFontDatabase::applicationFontFamilies( fontId ).at(0) << endl;
        QString fontFamily = m_page->m_fontDatabase.applicationFontFamilies( fontId ).at(0);
        // kDebug() << "Styles: " << m_page->m_fontDatabase.styles( fontFamily ) << endl;
        QString fontStyle =  m_page->m_fontDatabase.styles( fontFamily ).at(0);
        // TODO: We may not be picking the best font size here
        QFont font = m_page->m_fontDatabase.font(fontFamily, fontStyle, qRound(atts.value("FontRenderingEmSize").toFloat()) );
        m_painter->setFont(font);
        QPointF origin( atts.value("OriginX").toDouble(), atts.value("OriginY").toDouble() );
        QColor fillColor = hexToRgba( atts.value("Fill").toLatin1() );
        kDebug() << "Indices " << atts.value("Indices") << endl;
        m_painter->setBrush(fillColor);
        m_painter->setPen(fillColor);
        m_painter->drawText( origin, atts.value("UnicodeString") );
        m_painter->restore();
        // kDebug() << "Glyphs: " << atts.value("Fill") << ", " << atts.value("FontUri") << endl;
        // kDebug() << "    Origin: " << atts.value("OriginX") << "," << atts.value("OriginY") << endl;
        // kDebug() << "    Unicode: " << atts.value("UnicodeString") << endl;
    } else if (localName == "Path") {
        // kDebug() << "Path: " << atts.value("Data") << ", " << atts.value("Fill") << endl;
        if (! atts.value("Data").isEmpty() ) {
            parseAbbreviatedPathData( atts.value("Data") );
        }
        if (! atts.value("Fill").isEmpty() ) {
            QColor fillColor;
            if ( atts.value("Fill").startsWith('#') ) {
                fillColor = hexToRgba( atts.value("Fill").toLatin1() );
            } else {
                kDebug() << "Unknown / unhandled fill color representation:" << atts.value("Fill") << ":ende" << endl;
            }
            m_currentBrush = QBrush( fillColor );
            m_currentPen = QPen ( fillColor );
        }
    } else if ( localName == "SolidColorBrush" ) {
        if (! atts.value("Color").isEmpty() ) {
            QColor fillColor;
            if (atts.value("Color").startsWith('#') ) {
                fillColor = hexToRgba( atts.value("Color").toLatin1() );
                // kDebug() << "Solid colour: " << fillColor << endl;
            } else {
                kDebug() << "Unknown / unhandled fill color representation:" << atts.value("Color") << ":ende" << endl;
            }
            m_currentBrush = QBrush( fillColor );
        }
    } else if ( localName == "ImageBrush" ) {
        kDebug() << "ImageBrush Attributes: " << atts.count() << ", " << atts.value("Transform") << ", " << atts.value("TileMode") << endl;
        m_image = m_page->loadImageFromFile( atts.value("ImageSource" ) );
        m_viewbox = stringToRectF( atts.value("Viewbox") );
        m_viewport = stringToRectF( atts.value("Viewport") );
    } else if ( localName == "Canvas" ) {
        // TODO
        m_painter->save();
    } else if ( localName == "MatrixTransform" ) {
        // kDebug() << "Matrix transform: " << atts.value("Matrix") << endl;
        m_painter->setWorldMatrix( attsToMatrix( atts.value("Matrix") ), true );
    } else if ( localName == "Path.Fill" ) {
        // this doesn't have any attributes - just other elements that we handle elsewhere
    } else {
        kDebug() << "unknown start element: " << localName << endl;
    }

    return true;
}

bool XpsHandler::endElement( const QString &nameSpace,
                             const QString &localName,
                             const QString &qname)
{
    if ( localName == "Path" ) {
        m_painter->save();
        m_painter->setBrush( m_currentBrush );
        m_painter->setPen( m_currentPen );
        m_painter->drawPath( m_currentPath );

        if (! m_image.isNull() ) {
            m_painter->drawImage( m_viewport, m_image, m_viewbox );
        }

        m_painter->restore();
        m_currentPath = QPainterPath();
        m_currentBrush = QBrush();
        m_currentPen = QPen();
        m_image = QImage();
    } else if ( localName == "Canvas" ) {
        m_painter->restore();
    }
    return true;
}

XpsPage::XpsPage(KZip *archive, const QString &fileName): m_archive( archive ),
    m_fileName( fileName ), m_pageIsRendered(false)
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

    QDomElement element = m_dom.documentElement().toElement();
    m_pageSize.setWidth( element.attribute("Width").toInt() );
    m_pageSize.setHeight( element.attribute("Height").toInt() );
    m_pageImage = new QImage( m_pageSize, QImage::Format_ARGB32 );
    delete pageDevice;
}

bool XpsPage::renderToImage( QImage *p )
{
    if (! m_pageIsRendered) {
        XpsHandler *handler = new XpsHandler( this );
        QXmlSimpleReader *parser = new QXmlSimpleReader();
        parser->setContentHandler( handler );
        parser->setErrorHandler( handler );
        QXmlInputSource *source = new QXmlInputSource();
        source->setData( m_dom.toString() );
        bool ok = parser->parse( source );
        kDebug() << "Parse result: " << ok << endl;
        delete source;
        delete parser;
        delete handler;
        m_pageIsRendered = true;
    }

    if ( size() == p->size() )
        *p = *m_pageImage;
    else
        *p = m_pageImage->scaled( p->size(), Qt::KeepAspectRatio );

    return true;
}
QSize XpsPage::size() const
{
    return m_pageSize;
}

int XpsPage::loadFontByName( const QString &fileName )
{
    // kDebug() << "font file name: " << fileName << endl;

    KZipFileEntry* fontFile = static_cast<const KZipFileEntry *>(m_archive->directory()->entry( fileName ));

    QByteArray fontData = fontFile->data(); // once per file, according to the docs

    int result = m_fontDatabase.addApplicationFontFromData( fontData );
    if (-1 == result) {
        kDebug() << "Failed to load application font from data" << endl;
    }
    
    // kDebug() << "Loaded font: " << m_fontDatabase.applicationFontFamilies( result ) << endl;
    
    return result; // a font ID
}

QImage XpsPage::loadImageFromFile( const QString &fileName )
{
    kDebug() << "image file name: " << fileName << endl;

    KZipFileEntry* imageFile = static_cast<const KZipFileEntry *>(m_archive->directory()->entry( fileName ));

    QByteArray imageData = imageFile->data(); // once per file, according to the docs

    QImage image;
    bool result = image.loadFromData( imageData );
    kDebug() << "Image load result: " << result << ", " << image.size() << endl;
    return image;
}

XpsDocument::XpsDocument(KZip *archive, const QString &fileName)
{
    kDebug() << "document file name: " << fileName << endl;

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
                QString pagePath = element.attribute("Source");
                if (pagePath.startsWith('/') == false ) {
                    int offset = fileName.lastIndexOf('/');
                    QString thisDir = fileName.mid(0,  offset) + '/';
                    pagePath.prepend(thisDir);
                }
                XpsPage *page = new XpsPage( archive, pagePath );
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

XpsFile::XpsFile() : m_docInfo( 0 )
{
}


XpsFile::~XpsFile()
{
}


bool XpsFile::loadDocument(const QString &filename)
{
    xpsArchive = new KZip( filename );
    if ( xpsArchive->open( QIODevice::ReadOnly ) == true ) {
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
                for (int lv = 0; lv < doc->numPages(); ++lv) {
                    // our own copy of the pages list
                    m_pages.append( doc->page( lv ) );
                }
                m_documents.append(doc);
            } else {
                kDebug() << "Unhandled entry in FixedDocumentSequence" << e.tagName() << endl;
            }
        }
        n = n.nextSibling();
    }

    delete fixedRepDevice;

    return true;
}

const Okular::DocumentInfo * XpsFile::generateDocumentInfo()
{
    if ( m_docInfo )
        return m_docInfo;

    m_docInfo = new Okular::DocumentInfo();

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
    return m_pages.size();
}

int XpsFile::numDocuments() const
{
    return m_documents.size();
}

XpsDocument* XpsFile::document(int documentNum) const
{
    return m_documents.at( documentNum );
}

XpsPage* XpsFile::page(int pageNum) const
{
    return m_pages.at( pageNum );
}

XpsGenerator::XpsGenerator( Okular::Document * document )
  : Okular::Generator( document )
{
    m_xpsFile = new XpsFile;
}

XpsGenerator::~XpsGenerator()
{
    delete m_xpsFile;
}

bool XpsGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
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
            pagesVector[pagesVectorOffset] = new Okular::Page( pagesVectorOffset, pageSize.width(), pageSize.height(), 0 );
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

bool XpsGenerator::canGeneratePixmap( bool /*async*/ ) const
{
    return true;
}

void XpsGenerator::generatePixmap( Okular::PixmapRequest * request )
{
    QSize size( (int)request->page->width(), (int)request->page->height() );
    QPixmap * p = new QPixmap( size );
    QImage image( size, QImage::Format_RGB32 );
    XpsPage *pageToRender = m_xpsFile->page( request->page->number() );
    pageToRender->renderToImage( &image );
    *p = QPixmap::fromImage( image );
    request->page->setPixmap( request->id, p );
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

const Okular::DocumentInfo * XpsGenerator::generateDocumentInfo()
{
    kDebug() << "generating document metadata" << endl;

    return m_xpsFile->generateDocumentInfo();
}


#include "generator_xps.moc"

