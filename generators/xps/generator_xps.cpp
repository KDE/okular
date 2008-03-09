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

#include "generator_xps.h"

#include <qdatetime.h>
#include <qfile.h>
#include <qlist.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qthread.h>
#include <kaboutdata.h>
#include <kglobal.h>
#include <klocale.h>
#include <kurl.h>
#include <QFileInfo>
#include <QBuffer>
#include <QImageReader>

#include <okular/core/document.h>
#include <okular/core/page.h>
#include <okular/core/area.h>

const int XpsDebug = 4712;

static KAboutData createAboutData()
{
    KAboutData aboutData(
         "okular_xps",
         "okular_xps",
         ki18n( "XPS Backend" ),
         "0.1",
         ki18n( "An XPS backend" ),
         KAboutData::License_GPL,
         ki18n( "© 2006-2007 Brad Hards\n"
                "© 2007 Jiri Klement" )
    );
    aboutData.addAuthor( ki18n( "Brad Hards" ), KLocalizedString(), "bradh@frogmouth.net" );
    aboutData.addAuthor( ki18n( "Jiri Klement" ), KLocalizedString(), "jiri.klement@gmail.com" );
    return aboutData;
}

OKULAR_EXPORT_PLUGIN( XpsGenerator, createAboutData() )

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

static bool parseGUID( const QString &guidString, unsigned short guid[16]) {

    if (guidString.length() <= 35) {
        return false;
    }

    // Maps bytes to positions in guidString
    const static int indexes[] = {6, 4, 2, 0, 11, 9, 16, 14, 19, 21, 24, 26, 28, 30, 32, 34};

    for (int i = 0; i < 16; i++) {
        int hex1 = hex2int(guidString[indexes[i]].cell());
        int hex2 = hex2int(guidString[indexes[i]+1].cell());

        if ((hex1 < 0) || (hex2 < 0))
        {
            return false;
        }

        guid[i] = hex1 * 16 + hex2;
    }

    return true;

}


// Read next token of abbreviated path data
static bool nextAbbPathToken(AbbPathToken *token)
{
    int *curPos = &token->curPos;
    QString data = token->data;

    while ((*curPos < data.length()) && (data.at(*curPos).isSpace()))
    {
        (*curPos)++;
    }

    if (*curPos == data.length())
    {
        token->type = abtEOF;
        return true;
    }

    QChar ch = data.at(*curPos);

    if (ch.isNumber() || (ch == '+') || (ch == '-'))
    {
        int start = *curPos;
        while ((*curPos < data.length()) && (!data.at(*curPos).isSpace()) && (data.at(*curPos) != ',') && !data.at(*curPos).isLetter())
    {
        (*curPos)++;
    }
    token->number = data.mid(start, *curPos - start).toDouble();
    token->type = abtNumber;

    } else if (ch == ',')
    {
        token->type = abtComma;
        (*curPos)++;
    } else if (ch.isLetter())
    {
        token->type = abtCommand;
        token->command = data.at(*curPos).cell();
        (*curPos)++;
    } else
    {
        (*curPos)++;
        return false;
    }

    return true;
}

/**
    Read point (two reals delimited by comma) from abbreviated path data
*/
static QPointF getPointFromString(AbbPathToken *token, bool relative, const QPointF &currentPosition) {
    //TODO Check grammar

    QPointF result;
    result.rx() = token->number;
    nextAbbPathToken(token);
    nextAbbPathToken(token); // ,
    result.ry() = token->number;
    nextAbbPathToken(token);

    if (relative)
    {
        result += currentPosition;
    }

    return result;
}

/**
    Parse an abbreviated path "Data" description
    \param data the string containing the whitespace separated values

    \see XPS specification 4.2.3 and Appendix G
*/
static QPainterPath parseAbbreviatedPathData( const QString &data)
{
    QPainterPath path;

    AbbPathToken token;

    token.data = data;
    token.curPos = 0;

    nextAbbPathToken(&token);

    // Used by Smooth cubic curve (command s)
    char lastCommand = ' ';
    QPointF lastSecondControlPoint;

    while (true)
    {
        if (token.type != abtCommand)
        {
            if (token.type != abtEOF)
            {
                kDebug(XpsDebug) << "Error in parsing abbreviated path data";
            }
            return path;
        }

        char command = QChar(token.command).toLower().cell();
        bool isRelative = QChar(token.command).isLower();
        QPointF currPos = path.currentPosition();
        nextAbbPathToken(&token);

        switch (command) {
            case 'f':
                int rule;
                rule = (int)token.number;
                if (rule == 0)
                {
                    path.setFillRule(Qt::OddEvenFill);
                }
                else if (rule == 1)
                {
                    // In xps specs rule 1 means NonZero fill. I think it's equivalent to WindingFill but I'm not sure
                    path.setFillRule(Qt::WindingFill);
                }
                nextAbbPathToken(&token);
                break;
            case 'm': // Move
                while (token.type == abtNumber)
                {
                    QPointF point = getPointFromString(&token, isRelative, currPos);
                    path.moveTo(point);
                }
                break;
            case 'l': // Line
                while (token.type == abtNumber)
                {
                    QPointF point = getPointFromString(&token, isRelative, currPos);
                    path.lineTo(point);
                }
                break;
            case 'h': // Horizontal line
                while (token.type == abtNumber)
                {
                    double x = token.number + isRelative?path.currentPosition().x():0;
                    path.lineTo(x, path.currentPosition().y());
                    nextAbbPathToken(&token);
                }
                break;
            case 'v': // Vertical line
                while (token.type == abtNumber)
                {
                    double y = token.number + isRelative?path.currentPosition().y():0;
                    path.lineTo(path.currentPosition().x(), y);
                    nextAbbPathToken(&token);
                }
                break;
            case 'c': // Cubic bezier curve
                while (token.type == abtNumber)
                {
                    QPointF firstControl = getPointFromString(&token, isRelative, currPos);
                    QPointF secondControl = getPointFromString(&token, isRelative, currPos);
                    QPointF endPoint = getPointFromString(&token, isRelative, currPos);
                    path.cubicTo(firstControl, secondControl, endPoint);

                    lastSecondControlPoint = secondControl;
                }
                break;
            case 'q': // Quadratic bezier curve
                while (token.type == abtNumber)
                {
                    QPointF point1 = getPointFromString(&token, isRelative, currPos);
                    QPointF point2 = getPointFromString(&token, isRelative, currPos);
                    path.quadTo(point2, point2);
                }
                break;
            case 's': // Smooth cubic bezier curve
                while (token.type == abtNumber)
                {
                    QPointF firstControl;
                    if ((lastCommand == 's') || (lastCommand == 'c'))
                    {
                        firstControl = lastSecondControlPoint + (lastSecondControlPoint + path.currentPosition());
                    }
                    else
                    {
                        firstControl = path.currentPosition();
                    }
                    QPointF secondControl = getPointFromString(&token, isRelative, currPos);
                    QPointF endPoint = getPointFromString(&token, isRelative, currPos);
                    path.cubicTo(firstControl, secondControl, endPoint);
                }
                break;
            case 'a': // Arc
                //TODO Implement Arc drawing
                while (token.type == abtNumber)
                {
                    /*QPointF rp =*/ getPointFromString(&token, isRelative, currPos);
                    /*double r = token.number;*/
                    nextAbbPathToken(&token);
                    /*double fArc = token.number; */
                    nextAbbPathToken(&token);
                    /*double fSweep = token.number; */
                    nextAbbPathToken(&token);
                    /*QPointF point = */getPointFromString(&token, isRelative, currPos);
                }
                break;
            case 'z': // Close path
                path.closeSubpath();
                break;
        }

        lastCommand = command;
    }

    return path;
}

/**
   Parse a "Matrix" attribute string
   \param csv the comma separated list of values
   \return the QMatrix corresponding to the affine transform
   given in the attribute

   \see XPS specification 7.4.1
*/
static QMatrix attsToMatrix( const QString &csv )
{
    QStringList values = csv.split( ',' );
    if ( values.count() != 6 ) {
        return QMatrix(); // that is an identity matrix - no effect
    }
    return QMatrix( values.at(0).toDouble(), values.at(1).toDouble(),
                    values.at(2).toDouble(), values.at(3).toDouble(),
                    values.at(4).toDouble(), values.at(5).toDouble() );
}

/**
   \return Brush with given color or brush specified by reference to resource
*/
static QBrush parseRscRefColorForBrush( const QString &data )
{
    if (data[0] == '{') {
        //TODO
        kDebug(XpsDebug) << "Reference" << data;
        return QBrush();
    } else {
        return QBrush( hexToRgba( data.toLatin1() ) );
    }
}

/**
   \return Pen with given color or Pen specified by reference to resource
*/
static QPen parseRscRefColorForPen( const QString &data )
{
    if (data[0] == '{') {
        //TODO
        kDebug(XpsDebug) << "Reference" << data;
        return QPen();
    } else {
        return QPen( hexToRgba( data.toLatin1() ) );
    }
}

/**
   \return Matrix specified by given data or by referenced dictionary
*/
static QMatrix parseRscRefMatrix( const QString &data )
{
    if (data[0] == '{') {
        //TODO
        kDebug(XpsDebug) << "Reference" << data;
        return QMatrix();
    } else {
        return attsToMatrix( data );
    }
}

/**
   \return The path of the entry
*/
static QString entryPath( const QString &entry )
{
    const int index = entry.lastIndexOf( QChar::fromLatin1( '/' ) );
    QString ret = QString::fromLatin1( "/" ) + entry.mid( 0, index );
    if ( index > 0 ) {
        ret.append( QChar::fromLatin1( '/' ) );
    }
    return ret;
}

/**
   \return The path of the entry
*/
static QString entryPath( const KZipFileEntry* entry )
{
    return entryPath( entry->path() );
}

/**
   \return The absolute path of the \p location, according to \p path if it's non-absolute
*/
static QString absolutePath( const QString &path, const QString &location )
{
    if ( location.at( 0 ) == QLatin1Char( '/' ) ) {
        // already absolute
        return location;
    }

    KUrl url = KUrl::fromPath( path );
    url.setFileName( location );
    return url.toLocalFile();
}

/**
   Read the content of an archive entry in both the cases:
   a) single file
      + foobar
   b) directory
      + foobar/
        + [0].piece
        + [1].piece
        + ...
        + [x].last.piece

   \see XPS specification 10.1.2
*/
static QByteArray readFileOrDirectoryParts( const KArchiveEntry *entry, QString *pathOfFile = 0 )
{
    QByteArray data;
    if ( entry->isDirectory() ) {
        const KArchiveDirectory* relDir = static_cast<const KArchiveDirectory *>( entry );
        QStringList entries = relDir->entries();
        qSort( entries );
        Q_FOREACH ( const QString &entry, entries ) {
            const KArchiveEntry* relSubEntry = relDir->entry( entry );
            if ( !relSubEntry->isFile() )
                continue;

            const KZipFileEntry* relSubFile = static_cast<const KZipFileEntry *>( relSubEntry );
            data.append( relSubFile->data() );
        }
    } else {
        const KZipFileEntry* relFile = static_cast<const KZipFileEntry *>( entry );
        data.append( relFile->data() );
        if ( pathOfFile ) {
            *pathOfFile = entryPath( relFile );
        }
    }
    return data;
}

XpsHandler::XpsHandler(XpsPage *page): m_page(page)
{
    m_painter = NULL;
}

XpsHandler::~XpsHandler()
{
    delete m_painter;
}

bool XpsHandler::startDocument()
{
    kDebug(XpsDebug) << "start document" << m_page->m_fileName ;
    m_page->m_pageImage->fill( qRgba( 255, 255, 255, 255 ) );

    XpsRenderNode node;
    node.name = "document";
    m_nodes.push(node);

    return true;
}

bool XpsHandler::startElement( const QString &nameSpace,
                               const QString &localName,
                               const QString &qname,
                               const QXmlAttributes & atts )
{
    Q_UNUSED( nameSpace )
    Q_UNUSED( qname )

    XpsRenderNode node;
    node.name = localName;
    node.attributes = atts;
    node.data = NULL;
    processStartElement( node );
    m_nodes.push(node);


    return true;
}

bool XpsHandler::endElement( const QString &nameSpace,
                             const QString &localName,
                             const QString &qname)
{
    Q_UNUSED( nameSpace )
    Q_UNUSED( qname )


    XpsRenderNode node = m_nodes.pop();
    if (node.name != localName) {
        kDebug(XpsDebug) << "Name doesn't match";
    }
    processEndElement( node );
    node.children.clear();
    m_nodes.top().children.append(node);

    return true;
}

void XpsHandler::processGlyph( XpsRenderNode &node )
{
    //TODO Currently ignored attributes: BidiLevel, CaretStops, DeviceFontName, IsSideways, Indices, StyleSimulation, Clip, OpacityMask, Name, FixedPage.NavigateURI, xml:lang, x:key
    //TODO Currently ignored child elements: Clip, OpacityMask
    //Handled separately: RenderTransform

    QString att;

    m_painter->save();

    // Get font (doesn't work well because qt doesn't allow to load font from file)
    // This works despite the fact that font size isn't specified in points as required by qt. It's because I set point size to be equal to drawing unit.
    // kDebug(XpsDebug) << "Font Rendering EmSize: " << node.attributes.value("FontRenderingEmSize").toFloat();
    QFont font = m_page->m_file->getFontByName( node.attributes.value("FontUri"),  node.attributes.value("FontRenderingEmSize").toFloat());
    m_painter->setFont(font);

    //Origin
    QPointF origin( node.attributes.value("OriginX").toDouble(), node.attributes.value("OriginY").toDouble() );

    //Fill
    QBrush brush;
    att = node.attributes.value("Fill");
    if (att.isEmpty()) {
        XpsFill * data = (XpsFill *)node.getChildData( "Glyphs.Fill" );
        if (data != NULL) {
            brush = * data;
        delete data;
        } else {
            brush = QBrush();
        }
    } else {
        brush = parseRscRefColorForBrush( att );
    }
    m_painter->setBrush( brush );
    m_painter->setPen( QPen( brush, 0 ) );

    // Opacity
    att = node.attributes.value("Opacity");
    if (! att.isEmpty()) {
        m_painter->setOpacity(att.toDouble());
    }

    //RenderTransform
    att = node.attributes.value("RenderTransform");
    if (!att.isEmpty()) {
        m_painter->setWorldMatrix( parseRscRefMatrix( att ), true);
    }

    m_painter->drawText( origin, node.attributes.value("UnicodeString") );
    // kDebug(XpsDebug) << "Glyphs: " << atts.value("Fill") << ", " << atts.value("FontUri");
    // kDebug(XpsDebug) << "    Origin: " << atts.value("OriginX") << "," << atts.value("OriginY");
    // kDebug(XpsDebug) << "    Unicode: " << atts.value("UnicodeString");

    m_painter->restore();
}

void XpsHandler::processFill( XpsRenderNode &node )
{
    //TODO Ignored child elements: LinearGradientBrush, RadialGradientBrush, VirtualBrush

    if (node.children.size() != 1) {
        kDebug(XpsDebug) << "Fill element should have exactly one child";
    } else {
        node.data = node.children[0].data;
    }
}

void XpsHandler::processImageBrush( XpsRenderNode &node )
{
    //TODO Ignored attributes: Opacity, x:key, TileMode, ViewBoxUnits, ViewPortUnits
    //TODO Check whether transformation works for non standard situations (viewbox different that whole image, Transform different that simple move & scale, Viewport different than [0, 0, 1, 1]

    QString att;
    QBrush brush;

    QRectF viewport = stringToRectF( node.attributes.value( "Viewport" ) );
    QRectF viewbox = stringToRectF( node.attributes.value( "Viewbox" ) );
    QImage image = m_page->loadImageFromFile( node.attributes.value( "ImageSource" ) );

    // Matrix which can transform [0, 0, 1, 1] rectangle to given viewbox
    QMatrix viewboxMatrix = QMatrix( viewbox.width() * image.physicalDpiX() / 96, 0, 0, viewbox.height() * image.physicalDpiY() / 96, viewbox.x(), viewbox.y() );

    // Matrix which can transform [0, 0, 1, 1] rectangle to given viewport
    //TODO Take ViewPort into account
    QMatrix viewportMatrix;
    att = node.attributes.value( "Transform" );
    if ( att.isEmpty() ) {
        XpsMatrixTransform * data = (XpsMatrixTransform *)node.getChildData( "ImageBrush.Transform" );
        if (data != NULL) {
            viewportMatrix = *data;
            delete data;
        } else {
            viewportMatrix = QMatrix();
        }
    } else {
        viewportMatrix = parseRscRefMatrix( att );
    }
    viewportMatrix = viewportMatrix * QMatrix( viewport.width(), 0, 0, viewport.height(), viewport.x(), viewbox.y() );


    // TODO Brush should work also for QImage, not only QPixmap. But for some images it doesn't work
    brush = QBrush( QPixmap::fromImage( image) );
    brush.setMatrix( viewboxMatrix.inverted() * viewportMatrix );

    node.data = new QBrush( brush );
}

void XpsHandler::processPath( XpsRenderNode &node )
{
    //TODO Ignored attributes: Clip, OpacityMask, StrokeDashArray, StrokeDashCap, StrokeDashOffset, StrokeEndLineCap, StorkeStartLineCap, StrokeLineJoin, StrokeMiterLimit, Name, FixedPage.NavigateURI, xml:lang, x:key, AutomationProperties.Name, AutomationProperties.HelpText, SnapsToDevicePixels
    //TODO Ignored child elements: RenderTransform, Clip, OpacityMask, Stroke, Data
    // Handled separately: RenderTransform
    m_painter->save();

    QString att;
    QPainterPath path;

    // Get path
    att = node.attributes.value( "Data" );
    if (! att.isEmpty() ) {
        path = parseAbbreviatedPathData( att );
    } else {
        path = QPainterPath(); //TODO
    }

    // Set Fill
    att = node.attributes.value( "Fill" );
    QBrush brush;
    if (! att.isEmpty() ) {
        brush = parseRscRefColorForBrush( att );
    } else {
        XpsFill * data = (XpsFill *)node.getChildData( "Path.Fill" );
        if (data != NULL) {
            brush = *data;
        delete data;
        } else {
            brush = QBrush();
        }
    }
    m_painter->setBrush( brush );

    // Stroke (pen)
    // We don't handle the child elements (Path.Stroke) yet.
    att = node.attributes.value( "Stroke" );
    QPen pen( Qt::transparent );
    if  (! att.isEmpty() ) {
        pen = parseRscRefColorForPen( att );
    }
    att = node.attributes.value( "StrokeThickness" );
    if  (! att.isEmpty() ) {
        bool ok = false;
        int thickness = att.toInt( &ok );
        if (ok)
            pen.setWidth( thickness );
    }
    m_painter->setPen( pen );

    // Opacity
    att = node.attributes.value("Opacity");
    if (! att.isEmpty()) {
        m_painter->setOpacity(att.toDouble());
    }

    // RenderTransform
    att = node.attributes.value( "RenderTransform" );
    if (! att.isEmpty() ) {
        m_painter->setWorldMatrix( parseRscRefMatrix( att ), true );
    }

    m_painter->drawPath( path ); //TODO Valgrind sometimes say that path drawing depends on uninitialized value in blend_texture

    m_painter->restore();
}

void XpsHandler::processStartElement( XpsRenderNode &node )
{
    if (node.name == "Canvas") {
        m_painter->save();
    }
}

void XpsHandler::processEndElement( XpsRenderNode &node )
{
    if (node.name == "Glyphs") {
        processGlyph( node );
    } else if (node.name == "Path") {
        processPath( node );
    } else if (node.name == "MatrixTransform") {
        //TODO Ignoring x:key
        node.data = new QMatrix ( attsToMatrix( node.attributes.value( "Matrix" ) ) );
    } else if ((node.name == "Canvas.RenderTransform") || (node.name == "Glyphs.RenderTransform") || (node.name == "Path.RenderTransform"))  {
        XpsMatrixTransform * data = (XpsMatrixTransform *)node.getRequiredChildData( "MatrixTransform" );
        if (data != NULL) {
            m_painter->setWorldMatrix( *data, true );
        delete data;
        }
    } else if (node.name == "Canvas") {
        m_painter->restore();
    } else if ((node.name == "Path.Fill") || (node.name == "Glyphs.Fill")) {
        processFill( node );
    } else if (node.name == "SolidColorBrush") {
        //TODO Ignoring opacity, x:key
        node.data = new QBrush( QColor (hexToRgba( node.attributes.value( "Color" ).toLatin1() ) ) );
    } else if (node.name == "ImageBrush") {
        processImageBrush( node );
    } else if (node.name == "ImageBrush.Transform") {
        node.data = node.getRequiredChildData( "MatrixTransform" );
    } else {
        //kDebug(XpsDebug) << "Unknown element: " << node->name;
    }
}

XpsPage::XpsPage(XpsFile *file, const QString &fileName): m_file( file ),
    m_fileName( fileName ), m_pageIsRendered(false)
{
    m_pageImage = NULL;

    // kDebug(XpsDebug) << "page file name: " << fileName;

    const KZipFileEntry* pageFile = static_cast<const KZipFileEntry *>(m_file->xpsArchive()->directory()->entry( fileName ));

    QXmlStreamReader xml;
    xml.addData( pageFile->data() );
    while ( !xml.atEnd() )
    {
        xml.readNext();
        if ( xml.isStartElement() && ( xml.name() == "FixedPage" ) )
        {
            QXmlStreamAttributes attributes = xml.attributes();
            m_pageSize.setWidth( attributes.value( "Width" ).toString().toInt() );
            m_pageSize.setHeight( attributes.value( "Height" ).toString().toInt() );
            break;
        }
    }
    if ( xml.error() )
    {
        kDebug(XpsDebug) << "Could not parse XPS page:" << xml.errorString();
    }
}

XpsPage::~XpsPage()
{
    delete m_pageImage;
}

bool XpsPage::renderToImage( QImage *p )
{

    if ((m_pageImage == NULL) || (m_pageImage->size() != p->size())) {
        delete m_pageImage;
        m_pageImage = new QImage( p->size(), QImage::Format_ARGB32 );
        // Set one point = one drawing unit. Useful for fonts, because xps specifies font size using drawing units, not points as usual
        m_pageImage->setDotsPerMeterX( 2835 );
        m_pageImage->setDotsPerMeterY( 2835 );

        m_pageIsRendered = false;
    }
    if (! m_pageIsRendered) {
        XpsHandler *handler = new XpsHandler( this );
        handler->m_painter = new QPainter( m_pageImage );
        handler->m_painter->setWorldMatrix(QMatrix().scale((qreal)p->size().width() / size().width(), (qreal)p->size().height() / size().height()));
        QXmlSimpleReader *parser = new QXmlSimpleReader();
        parser->setContentHandler( handler );
        parser->setErrorHandler( handler );
        const KZipFileEntry* pageFile = static_cast<const KZipFileEntry *>(m_file->xpsArchive()->directory()->entry( m_fileName ));
        QByteArray data = pageFile->data();
        QBuffer * buffer = new QBuffer(&data);
        QXmlInputSource *source = new QXmlInputSource(buffer);
        bool ok = parser->parse( source );
        kDebug(XpsDebug) << "Parse result: " << ok;
        delete source;
        delete parser;
        delete handler;
        delete buffer;
        m_pageIsRendered = true;
    }

    *p = *m_pageImage;

    return true;
}

QSize XpsPage::size() const
{
    return m_pageSize;
}

QFont XpsFile::getFontByName( const QString &fileName, float size )
{
    // kDebug(XpsDebug) << "trying to get font: " << fileName << ", size: " << size;

    int index = m_fontCache.value(fileName, -1);
    if (index == -1)
    {
        index = loadFontByName(fileName);
        m_fontCache[fileName] = index;
    }

    QString fontFamily = m_fontDatabase.applicationFontFamilies( index ).at(0);
    QString fontStyle =  m_fontDatabase.styles( fontFamily ).at(0);
    QFont font = m_fontDatabase.font(fontFamily, fontStyle, qRound(size) );

    return font;
}

int XpsFile::loadFontByName( const QString &fileName )
{
    // kDebug(XpsDebug) << "font file name: " << fileName;

    const KZipFileEntry* fontFile = static_cast<const KZipFileEntry *>(m_xpsArchive->directory()->entry( fileName ));

    QByteArray fontData = fontFile->data(); // once per file, according to the docs

    int result = m_fontDatabase.addApplicationFontFromData( fontData );
    if (-1 == result) {
        // Try to deobfuscate font
       // TODO Use deobfuscation depending on font content type, don't do it always when standard loading fails

        QFileInfo* fileInfo = new QFileInfo(fileName);
        QString baseName = fileInfo->baseName();
        delete fileInfo;

        unsigned short guid[16];
        if (!parseGUID(baseName, guid))
        {
            kDebug(XpsDebug) << "File to load font - file name isn't a GUID";
        }
        else
        {
        if (fontData.length() < 32)
            {
                kDebug(XpsDebug) << "Font file is too small";
            } else {
                // Obfuscation - xor bytes in font binary with bytes from guid (font's filename)
                const static int mapping[] = {15, 14, 13, 12, 11, 10, 9, 8, 6, 7, 4, 5, 0, 1, 2, 3};
                for (int i = 0; i < 16; i++) {
                    fontData[i] = fontData[i] ^ guid[mapping[i]];
                    fontData[i+16] = fontData[i+16] ^ guid[mapping[i]];
                }
                result = m_fontDatabase.addApplicationFontFromData( fontData );
            }
        }
    }


    // kDebug(XpsDebug) << "Loaded font: " << m_fontDatabase.applicationFontFamilies( result );

    return result; // a font ID
}

KZip * XpsFile::xpsArchive() {
    return m_xpsArchive;
}

QImage XpsPage::loadImageFromFile( const QString &fileName )
{
    // kDebug(XpsDebug) << "image file name: " << fileName;

    if ( fileName.at( 0 ) == QLatin1Char( '{' ) ) {
        // for example: '{ColorConvertedBitmap /Resources/bla.wdp /Resources/foobar.icc}'
        // TODO: properly read a ColorConvertedBitmap
        return QImage();
    }

    const QString absoluteFileName = absolutePath( entryPath( m_fileName ), fileName );
    const KZipFileEntry* imageFile = static_cast<const KZipFileEntry *>(m_file->xpsArchive()->directory()->entry( absoluteFileName ));

    /* WORKAROUND:
        XPS standard requires to use 96dpi for images which doesn't have dpi specified (in file). When Qt loads such an image,
        it sets its dpi to qt_defaultDpi and doesn't allow to find out that it happend.

        To workaround this I used this procedure: load image, set its dpi to 96, load image again. When dpi isn't set in file,
        dpi set by me stays unchanged.

        I've posted a bug to qt.

    */

    QImage image;
    QByteArray data = imageFile->data();

    QBuffer buffer(&data);
    buffer.open(QBuffer::ReadOnly);

    QImageReader reader(&buffer);
    image = reader.read();

    image.setDotsPerMeterX(qRound(96 / 0.0254));
    image.setDotsPerMeterY(qRound(96 / 0.0254));
  
    buffer.seek(0);
    reader.setDevice(&buffer);
    reader.read(&image);

    return image;
}

Okular::TextPage* XpsPage::textPage()
{
    // kDebug(XpsDebug) << "Parsing XpsPage, text extraction";

    Okular::TextPage* textPage = new Okular::TextPage();

    const KZipFileEntry* pageFile = static_cast<const KZipFileEntry *>(m_file->xpsArchive()->directory()->entry( m_fileName ));
    QXmlStreamReader xml;
    xml.addData( pageFile->data() );

    QMatrix matrix = QMatrix();
    QStack<QMatrix> matrices;
    matrices.push( QMatrix() );
    bool useMatrix = false;
    QXmlStreamAttributes glyphsAtts;

    while ( ! xml.atEnd() ) {
        xml.readNext();
        if ( xml.isStartElement() ) {
            if ( xml.name() == "Canvas") {
                matrices.push(matrix);

                QString att = xml.attributes().value( "RenderTransform" ).toString();
                if (!att.isEmpty()) {
                    matrix = parseRscRefMatrix( att ) * matrix;
                }
            } else if ((xml.name() == "Canvas.RenderTransform") || (xml.name() == "Glyphs.RenderTransform")) {
                useMatrix = true;
            } else if (xml.name() == "MatrixTransform") {
                if (useMatrix) {
                    matrix = attsToMatrix( xml.attributes().value("Matrix").toString() ) * matrix;
                }
            } else if (xml.name() == "Glyphs") {
                matrices.push( matrix );
                glyphsAtts = xml.attributes();
            } else if ( (xml.name() == "Path") || (xml.name() == "Path.Fill") || (xml.name() == "SolidColorBrush")
                        || (xml.name() == "ImageBrush") ||  (xml.name() == "ImageBrush.Transform")
                        || (xml.name() == "Path.OpacityMask") ){
                // those are only graphical - no use in text handling
            } else if (xml.name() == "FixedPage")  {
                // not useful for text extraction
            } else {
                kDebug(XpsDebug) << "Unhandled element in Text Extraction start: " << xml.name().toString();
            }
        } else if (xml.isEndElement() ) {
            if (xml.name() == "Canvas") {
                matrix = matrices.pop();
            } else if ((xml.name() == "Canvas.RenderTransform") || (xml.name() == "Glyphs.RenderTransform")) {
                useMatrix = false;
            } else if (xml.name() == "MatrixTransform") {
                // not clear if we need to do anything here yet.
            } else if (xml.name() == "Glyphs") {
                QString att = glyphsAtts.value( "RenderTransform" ).toString();
                if (!att.isEmpty()) {
                    matrix = parseRscRefMatrix( att ) * matrix;
                }
                QString text =  glyphsAtts.value( "UnicodeString" ).toString();

                // Get font (doesn't work well because qt doesn't allow to load font from file)
                QFont font = m_file->getFontByName( glyphsAtts.value( "FontUri" ).toString(),
                                                    glyphsAtts.value("FontRenderingEmSize").toString().toFloat() * 72 / 96 );
                QFontMetrics metrics = QFontMetrics( font );
                // Origin
                QPointF origin( glyphsAtts.value("OriginX").toString().toDouble(),
                                glyphsAtts.value("OriginY").toString().toDouble() );


                int lastWidth = 0;
                for (int i = 0; i < text.length(); i++) {
                    int width = metrics.width( text, i + 1 );

                    Okular::NormalizedRect * rect = new Okular::NormalizedRect( (origin.x() + lastWidth) / m_pageSize.width(),
                                                                                (origin.y() - metrics.height()) / m_pageSize.height(),
                                                                                (origin.x() + width) / m_pageSize.width(),
                                                                                origin.y() / m_pageSize.height() );
                    rect->transform( matrix );
                    textPage->append( text.mid(i, 1), rect );

                    lastWidth = width;
                }

                matrix = matrices.pop();
            } else if ( (xml.name() == "Path") || (xml.name() == "Path.Fill") || (xml.name() == "SolidColorBrush")
                        || (xml.name() == "ImageBrush") ||  (xml.name() == "ImageBrush.Transform")
                        || (xml.name() == "Path.OpacityMask") ){
                // those are only graphical - no use in text handling
            } else if (xml.name() == "FixedPage")  {
                // not useful for text extraction
            } else {
                kDebug(XpsDebug) << "Unhandled element in Text Extraction end: " << xml.name().toString();
            }
        }
    }
    if ( xml.error() ) {
        kDebug(XpsDebug) << "Error parsing XpsPage text: " << xml.errorString();
    }
    return textPage;
}

void XpsDocument::parseDocumentStructure( const QString &documentStructureFileName )
{
    kDebug(XpsDebug) << "document structure file name: " << documentStructureFileName;
    m_haveDocumentStructure = false;

    const KZipFileEntry* documentStructureFile = static_cast<const KZipFileEntry *>(m_file->xpsArchive()->directory()->entry( documentStructureFileName ));

    QXmlStreamReader xml;
    xml.addData( documentStructureFile->data() );

    while ( !xml.atEnd() )
    {
        xml.readNext();

        if ( xml.isStartElement() ) {
            if ( xml.name() == "DocumentStructure" ) {
                // just a container for optional outline and story elements - nothing to do here
            } else if ( xml.name() == "DocumentStructure.Outline" ) {
                kDebug(XpsDebug) << "found DocumentStructure.Outline";
            } else if ( xml.name() == "DocumentOutline" ) {
                kDebug(XpsDebug) << "found DocumentOutline";
                m_docStructure = new Okular::DocumentSynopsis;
            } else if ( xml.name() == "OutlineEntry" ) {
                m_haveDocumentStructure = true;
                QXmlStreamAttributes attributes = xml.attributes();
                int outlineLevel = attributes.value( "OutlineLevel").toString().toInt();
                QString description = attributes.value("Description").toString();
                QDomElement synopsisElement = m_docStructure->createElement( description );
                synopsisElement.setAttribute( "OutlineLevel",  outlineLevel );
                QString target = attributes.value("OutlineTarget").toString();
                int hashPosition = target.lastIndexOf( '#' );
                target = target.mid( hashPosition + 1 );
                // kDebug(XpsDebug) << "target: " << target;
                Okular::DocumentViewport viewport;
                viewport.pageNumber = m_docStructurePageMap.value( target );
                synopsisElement.setAttribute( "Viewport",  viewport.toString() );
                if ( outlineLevel == 1 ) {
                    // kDebug(XpsDebug) << "Description: "
                    // << outlineEntryElement.attribute( "Description" ) << endl;
                    m_docStructure->appendChild( synopsisElement );
                } else {
                    // find the last next highest element (so it this is level 3, we need
                    // to find the most recent level 2 node)
                    QDomNode maybeParentNode = m_docStructure->lastChild();
                    while ( !maybeParentNode.isNull() )
                    {
                        if ( maybeParentNode.toElement().attribute( "OutlineLevel" ).toInt() == ( outlineLevel - 1 ) )  {
                            // we have the right parent
                            maybeParentNode.appendChild( synopsisElement );
                            break;
                        }
                        maybeParentNode = maybeParentNode.lastChild();
                    }
                }
            } else if ( xml.name() == "Story" ) {
                // we need to handle Story here, but I have no idea what to do with it.
            } else if ( xml.name() == "StoryFragment" ) {
                // we need to handle StoryFragment here, but I have no idea what to do with it.
            } else if ( xml.name() == "StoryFragmentReference" ) {
                // we need to handle StoryFragmentReference here, but I have no idea what to do with it.
            } else {
                kDebug(XpsDebug) << "Unhandled entry in DocumentStructure: " << xml.name().toString();
            }
        }
    }
}

const Okular::DocumentSynopsis * XpsDocument::documentStructure()
{
    return m_docStructure;
}

bool XpsDocument::hasDocumentStructure()
{
    return m_haveDocumentStructure;
}

XpsDocument::XpsDocument(XpsFile *file, const QString &fileName): m_file(file), m_haveDocumentStructure( false ), m_docStructure( 0 )
{
    kDebug(XpsDebug) << "document file name: " << fileName;

    const KArchiveEntry* documentEntry = file->xpsArchive()->directory()->entry( fileName );
    QString documentFilePath = fileName;

    QXmlStreamReader docXml;
    docXml.addData( readFileOrDirectoryParts( documentEntry, &documentFilePath ) );
    while( !docXml.atEnd() ) {
        docXml.readNext();
        if ( docXml.isStartElement() ) {
            if ( docXml.name() == "PageContent" ) {
                QString pagePath = docXml.attributes().value("Source").toString();
                kDebug(XpsDebug) << "Page Path: " << pagePath;
                XpsPage *page = new XpsPage( file, absolutePath( documentFilePath, pagePath ) );
                m_pages.append(page);
            } else if ( docXml.name() == "PageContent.LinkTargets" ) {
                // do nothing - wait for the real LinkTarget elements
            } else if ( docXml.name() == "LinkTarget" ) {
                QString targetName = docXml.attributes().value( "Name" ).toString();
                if ( ! targetName.isEmpty() ) {
                    m_docStructurePageMap[ targetName ] = m_pages.count() - 1;
                }
            } else if ( docXml.name() == "FixedDocument" ) {
                // we just ignore this - it is just a container
            } else {
                kDebug(XpsDebug) << "Unhandled entry in FixedDocument: " << docXml.name().toString();
            }
        }
    }
    if ( docXml.error() ) {
        kDebug(XpsDebug) << "Could not parse main XPS document file: " << docXml.errorString();
    }

    // There might be a relationships entry for this document - typically used to tell us where to find the
    // content structure description

    // We should be able to find this using a reference from some other part of the document, but I can't see it.
    QString maybeDocumentRelationshipPath = fileName;
    // trim off the old filename
    int slashPosition = maybeDocumentRelationshipPath.lastIndexOf( '/' );
    maybeDocumentRelationshipPath.truncate( slashPosition );
    // add in the path to the document relationships
    maybeDocumentRelationshipPath.append( "/_rels/FixedDoc.fdoc.rels" );

    const KZipFileEntry* relFile = static_cast<const KZipFileEntry *>(file->xpsArchive()->directory()->entry(maybeDocumentRelationshipPath));

    QString documentStructureFile;
    if ( relFile ) {
        QXmlStreamReader xml;
        xml.addData( relFile->data() );
        while ( !xml.atEnd() )
        {
            xml.readNext();
            if ( xml.isStartElement() && ( xml.name() == "Relationship" ) ) {
                QXmlStreamAttributes attributes = xml.attributes();
                if ( attributes.value( "Type" ).toString() == "http://schemas.microsoft.com/xps/2005/06/documentstructure" ) {
                    documentStructureFile  = attributes.value( "Target" ).toString();
                } else {
                    kDebug(XpsDebug) << "Unknown document relationships element: "
                                     << attributes.value( "Type" ).toString() << " : "
                                     << attributes.value( "Target" ).toString() << endl;
                }
            }
        }
        if ( xml.error() ) {
            kDebug(XpsDebug) << "Could not parse XPS page relationships file ( "
                             << maybeDocumentRelationshipPath
                             << " ) - " << xml.errorString() << endl;
        }
    } else { // the page relationship file didn't exist in the zipfile
        // this isn't fatal
        kDebug(XpsDebug) << "Could not open Document relationship file from " << maybeDocumentRelationshipPath;
    }

    if ( ! documentStructureFile.isEmpty() )
    {
        // kDebug(XpsDebug) << "Document structure filename: " << documentStructureFile;
        // make the document path absolute
        if ( documentStructureFile.startsWith( '/' ) )
        {
            // it is already absolute, don't do anything
        } else
        {
            // we reuse the relationship string
            maybeDocumentRelationshipPath.truncate( slashPosition );
            documentStructureFile.prepend( maybeDocumentRelationshipPath + '/' );
        }
        // kDebug(XpsDebug) << "Document structure absolute path: " << documentStructureFile;
        parseDocumentStructure( documentStructureFile );
    }

}

XpsDocument::~XpsDocument()
{
    for (int i = 0; i < m_pages.size(); i++) {
        delete m_pages.at( i );
    }
    m_pages.clear();

    if ( m_docStructure )
        delete m_docStructure;
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
    m_fontCache.clear();
    m_fontDatabase.removeAllApplicationFonts();
}


bool XpsFile::loadDocument(const QString &filename)
{
    m_xpsArchive = new KZip( filename );
    if ( m_xpsArchive->open( QIODevice::ReadOnly ) == true ) {
        kDebug(XpsDebug) << "Successful open of " << m_xpsArchive->fileName();
    } else {
        kDebug(XpsDebug) << "Could not open XPS archive: " << m_xpsArchive->fileName();
        delete m_xpsArchive;
        return false;
    }

    // The only fixed entry in XPS is /_rels/.rels
    const KArchiveEntry* relEntry = m_xpsArchive->directory()->entry("_rels/.rels");
    if ( !relEntry ) {
        // this might occur if we can't read the zip directory, or it doesn't have the relationships entry
        return false;
    }

    QXmlStreamReader relXml;
    relXml.addData( readFileOrDirectoryParts( relEntry ) );

    QString fixedRepresentationFileName;
    // We work through the relationships document and pull out each element.
    while ( !relXml.atEnd() )
    {
        relXml.readNext();
        if ( relXml.isStartElement() ) {
            if ( relXml.name() == "Relationship" ) {
                QXmlStreamAttributes attributes = relXml.attributes();
                QString type = attributes.value( "Type" ).toString();
                QString target = attributes.value( "Target" ).toString();
                if ( "http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail" == type ) {
                    m_thumbnailFileName = target;
                } else if ( "http://schemas.microsoft.com/xps/2005/06/fixedrepresentation" == type ) {
                    fixedRepresentationFileName = target;
                } else if ("http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" == type) {
                    m_corePropertiesFileName = target;
                } else if ("http://schemas.openxmlformats.org/package/2006/relationships/digital-signature/origin" == type) {
                    m_signatureOrigin = target;
                } else {
                    kDebug(XpsDebug) << "Unknown relationships element: " << type << " : " << target;
                }
            } else if ( relXml.name() == "Relationships" ) {
                // nothing to do here - this is just the container level
            } else {
                kDebug(XpsDebug) << "unexpected element in _rels/.rels: " << relXml.name().toString();
            }
        }
    }
    if ( relXml.error() ) {
        kDebug(XpsDebug) << "Could not parse _rels/.rels: " << relXml.errorString();
        return false;
    }

    if ( fixedRepresentationFileName.isEmpty() ) {
        // FixedRepresentation is a required part of the XPS document
        return false;
    }

    const KArchiveEntry* fixedRepEntry = m_xpsArchive->directory()->entry( fixedRepresentationFileName );
    QString fixedRepresentationFilePath = fixedRepresentationFileName;

    QXmlStreamReader fixedRepXml;
    fixedRepXml.addData( readFileOrDirectoryParts( fixedRepEntry, &fixedRepresentationFileName ) );

    while ( !fixedRepXml.atEnd() )
    {
        fixedRepXml.readNext();
        if ( fixedRepXml.isStartElement() ) {
            if ( fixedRepXml.name() == "DocumentReference" ) {
                const QString source = fixedRepXml.attributes().value("Source").toString();
                XpsDocument *doc = new XpsDocument( this, absolutePath( fixedRepresentationFilePath, source ) );
                for (int lv = 0; lv < doc->numPages(); ++lv) {
                    // our own copy of the pages list
                    m_pages.append( doc->page( lv ) );
                }
                m_documents.append(doc);
            } else if ( fixedRepXml.name() == "FixedDocumentSequence") {
                // we don't do anything here - this is just a container for one or more DocumentReference elements
            } else {
                kDebug(XpsDebug) << "Unhandled entry in FixedDocumentSequence: " << fixedRepXml.name().toString();
            }
        }
    }
    if ( fixedRepXml.error() ) {
        kDebug(XpsDebug) << "Could not parse FixedRepresentation file:" << fixedRepXml.errorString();
        return false;
    }

    return true;
}

const Okular::DocumentInfo * XpsFile::generateDocumentInfo()
{
    if ( m_docInfo )
        return m_docInfo;

    m_docInfo = new Okular::DocumentInfo();

    m_docInfo->set( Okular::DocumentInfo::MimeType, "application/vnd.ms-xpsdocument" );

    if ( ! m_corePropertiesFileName.isEmpty() ) {
        const KZipFileEntry* corepropsFile = static_cast<const KZipFileEntry *>(m_xpsArchive->directory()->entry(m_corePropertiesFileName));

        QXmlStreamReader xml;
        xml.addData( corepropsFile->data() );
        while ( !xml.atEnd() )
        {
            xml.readNext();
            if ( xml.isEndElement() )
                break;
            if ( xml.isStartElement() )
            {
                if (xml.name() == "title") {
                    m_docInfo->set( Okular::DocumentInfo::Title, xml.readElementText() );
                } else if (xml.name() == "subject") {
                    m_docInfo->set( Okular::DocumentInfo::Subject, xml.readElementText() );
                } else if (xml.name() == "description") {
                    m_docInfo->set( Okular::DocumentInfo::Description, xml.readElementText() );
                } else if (xml.name() == "creator") {
                    m_docInfo->set( Okular::DocumentInfo::Creator, xml.readElementText() );
                } else if (xml.name() == "category") {
                    m_docInfo->set( Okular::DocumentInfo::Category, xml.readElementText() );
                } else if (xml.name() == "created") {
                    QDateTime createdDate = QDateTime::fromString( xml.readElementText(), "yyyy-MM-ddThh:mm:ssZ" );
                    m_docInfo->set( Okular::DocumentInfo::CreationDate, KGlobal::locale()->formatDateTime( createdDate, KLocale::LongDate, true ) );
                } else if (xml.name() == "modified") {
                    QDateTime modifiedDate = QDateTime::fromString( xml.readElementText(), "yyyy-MM-ddThh:mm:ssZ" );
                    m_docInfo->set( Okular::DocumentInfo::ModificationDate, KGlobal::locale()->formatDateTime( modifiedDate, KLocale::LongDate, true ) );
                } else if (xml.name() == "keywords") {
                    m_docInfo->set( Okular::DocumentInfo::Keywords, xml.readElementText() );
                }
            }
        }
        if ( xml.error() )
        {
            kDebug(XpsDebug) << "Could not parse XPS core properties:" << xml.errorString();
        }
    } else {
        kDebug(XpsDebug) << "No core properties filename";
    }

    m_docInfo->set( Okular::DocumentInfo::Pages, QString::number(numPages()) );

    return m_docInfo;
}

bool XpsFile::closeDocument()
{

    if ( m_docInfo )
        delete m_docInfo;

    m_docInfo = 0;

    qDeleteAll( m_documents );
    m_documents.clear();

    delete m_xpsArchive;

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

XpsGenerator::XpsGenerator( QObject *parent, const QVariantList &args )
  : Okular::Generator( parent, args ), m_xpsFile( 0 )
{
    setFeature( TextExtraction );
}

XpsGenerator::~XpsGenerator()
{
}

bool XpsGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    m_xpsFile = new XpsFile();

    m_xpsFile->loadDocument( fileName );
    pagesVector.resize( m_xpsFile->numPages() );

    int pagesVectorOffset = 0;

    for (int docNum = 0; docNum < m_xpsFile->numDocuments(); ++docNum )
    {
        XpsDocument *doc = m_xpsFile->document( docNum );
        for (int pageNum = 0; pageNum < doc->numPages(); ++pageNum )
        {
            QSize pageSize = doc->page( pageNum )->size();
            pagesVector[pagesVectorOffset] = new Okular::Page( pagesVectorOffset, pageSize.width(), pageSize.height(), Okular::Rotation0 );
            ++pagesVectorOffset;
        }
    }

    return true;
}

bool XpsGenerator::doCloseDocument()
{
    m_xpsFile->closeDocument();
    delete m_xpsFile;
    m_xpsFile = 0;

    return true;
}

QImage XpsGenerator::image( Okular::PixmapRequest * request )
{
    QSize size( (int)request->width(), (int)request->height() );
    QImage image( size, QImage::Format_RGB32 );
    XpsPage *pageToRender = m_xpsFile->page( request->page()->number() );
    pageToRender->renderToImage( &image );
    return image;
}

Okular::TextPage* XpsGenerator::textPage( Okular::Page * page )
{
    XpsPage * xpsPage = m_xpsFile->page( page->number() );
    return xpsPage->textPage();
}

const Okular::DocumentInfo * XpsGenerator::generateDocumentInfo()
{
    kDebug(XpsDebug) << "generating document metadata";

    return m_xpsFile->generateDocumentInfo();
}

const Okular::DocumentSynopsis * XpsGenerator::generateDocumentSynopsis()
{
    kDebug(XpsDebug) << "generating document synopsis";

    // we only generate the synopsis for the first file.
    if ( !m_xpsFile || !m_xpsFile->document( 0 ) )
        return NULL;

    if ( m_xpsFile->document( 0 )->hasDocumentStructure() )
        return m_xpsFile->document( 0 )->documentStructure();


    return NULL;
}

Okular::ExportFormat::List XpsGenerator::exportFormats() const
{
    static Okular::ExportFormat::List formats;
    if ( formats.isEmpty() ) {
        formats.append( Okular::ExportFormat::standardFormat( Okular::ExportFormat::PlainText ) );
    }
    return formats;
}

bool XpsGenerator::exportTo( const QString &fileName, const Okular::ExportFormat &format )
{
    if ( format.mimeType()->name() == QLatin1String( "text/plain" ) ) {
        QFile f( fileName );
        if ( !f.open( QIODevice::WriteOnly ) )
            return false;

        QTextStream ts( &f );
        for ( int i = 0; i < m_xpsFile->numPages(); ++i )
        {
            Okular::TextPage* textPage = m_xpsFile->page(i)->textPage();
            QString text = textPage->text();
            ts << text;
            ts << QChar('\n');
            delete textPage;
        }
        f.close();

        return true;
    }

    return false;
}

XpsRenderNode * XpsRenderNode::findChild( const QString &name )
{
    for (int i = 0; i < children.size(); i++) {
        if (children[i].name == name) {
            return &children[i];
        }
    }

    return NULL;
}

void * XpsRenderNode::getRequiredChildData( const QString &name )
{
    XpsRenderNode * child = findChild( name );
    if (child == NULL) {
        kDebug(XpsDebug) << "Required element " << name << " is missing in " << this->name;
        return NULL;
    }

    return child->data;
}

void * XpsRenderNode::getChildData( const QString &name )
{
    XpsRenderNode * child = findChild( name );
    if (child == NULL) {
        return NULL;
    } else {
        return child->data;
    }
}

#include "generator_xps.moc"

