/***************************************************************************
 *   Copyright (C) 2004-5 by Enrico Ros <eros.kde@email.it>                *
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_H_
#define _OKULAR_GENERATOR_H_

#include "okular_export.h"

#define OKULAR_EXPORT_PLUGIN( classname ) \
    extern "C" { \
         OKULAR_EXPORT Okular::Generator* create_plugin(Okular::Document* doc) { return new classname(doc); } \
    }


#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <kmimetype.h>

#include "document.h"
#include "textpage.h"

class KConfigDialog;
class KPrinter;

namespace Okular {

class ExportEntry;
class Link;
class Page;
class PixmapRequest;

/* Note: on contents generation and asyncronous queries.
 * Many observers may want to request data syncronously or asyncronously.
 * - Sync requests. These should be done in-place.
 * - Async request must be done in real background. That usually means a
 *   thread, such as QThread derived classes.
 * Once contents are available, they must be immediately stored in the
 * Page they refer to, and a signal is emitted as soon as storing
 * (even for sync or async queries) has been done.
 */

/**
 * @short [Abstract Class] The information generator.
 *
 * Most of class members are pure virtuals and they must be implemented to
 * provide a class that builds contents (graphics and text).
 *
 * Generation/query is requested by the 'Document' class only, and that
 * class stores the resulting data into 'Page's. The data will then be
 * displayed by the GUI components (pageView, thumbnailList, etc..).
 */
class OKULAR_EXPORT Generator : public QObject
{
    Q_OBJECT

    public:
        /** constructor: takes the Document as a parameter **/
        Generator( Document * doc ) : m_document( doc ) {};

        /** virtual methods to reimplement **/
        // load a document and fill up the pagesVector
        virtual bool loadDocument( const QString & fileName, QVector< Page * > & pagesVector ) = 0;
        // the current document is no more useful, close it
        virtual bool closeDocument() = 0;

        // page contents generation
        virtual bool canGeneratePixmap( bool async ) = 0;
        virtual void generatePixmap( PixmapRequest * request ) = 0;

        // can generate a TextPage
        virtual bool canGenerateTextPage() { return false; };
        virtual void generateSyncTextPage( Page * /*page*/ ) {;};

        // Document description and Table of contents
        virtual const DocumentInfo * generateDocumentInfo() { return 0L; }
        virtual const DocumentSynopsis * generateDocumentSynopsis() { return 0L; }
        virtual const DocumentFonts * generateDocumentFonts() { return 0L; }
        virtual const QList<EmbeddedFile*> * embeddedFiles() { return 0L; }

        // DRM handling
        virtual bool isAllowed( int /*Document::Permisison(s)*/ ) { return true; }

        // gui stuff
        virtual QString getXMLFile() { return QString::null; } ;
        virtual void setupGUI(KActionCollection  * /*ac*/ , QToolBox * /*tBox*/ ) {;};
        virtual void freeGUI( ) {;};

        // search capabilities
        virtual bool supportsSearching() { return false; };
        virtual bool prefersInternalSearching() { return false; };
        virtual RegularAreaRect * findText( const QString & /*text*/, SearchDir /*dir*/, const bool /*strictCase*/,
                    const RegularAreaRect * /*lastRect*/, Page * /*page*/) { return 0L; };
        virtual QString getText( const RegularAreaRect * /*area*/, Page * /*page*/ ) { return QString(); }

        // rotation
        virtual bool supportsRotation() { return false; };
        virtual void setOrientation(QVector<Page*> & /*pagesVector*/, int /*orientation*/) { ; };

        // paper size
        virtual bool supportsPaperSizes () { return false; }
        virtual QStringList paperSizes ()  { return QStringList(); }
        virtual void setPaperSize (QVector<Page*> & /*pagesVector*/, int /*newsize*/) { ; }

        // may come useful later
        //virtual bool hasFonts() const = 0;

        // return true if wanting to configure the printer yourself in backend
        virtual bool canConfigurePrinter( ) { return false; }
        // print the document (using a printer configured or not depending on the above function)
        // note, if we are only asking for a preview this will be preconfigured
        virtual bool print( KPrinter& /*printer*/ ) { return false; }
        // access meta data of the generator
        virtual QString getMetaData( const QString &/*key*/, const QString &/*option*/ ) { return QString(); }
        // tell generator to re-parse configuration and return true if something changed
        virtual bool reparseConfig() { return false; }

        // add support for settings
        virtual void addPages( KConfigDialog* /*dlg*/ ) {;};
//         virtual void setConfigurationPointer( KConfigDialog* /*dlg*/) { ; } ;

        // support for exporting to text and to other formats
        virtual bool canExportToText() { return false; };
        virtual bool exportToText( const QString & /*fileName*/ ) { return false; };
        virtual QList<ExportEntry*> exportFormats() { return QList<ExportEntry*>(); };
        virtual bool exportTo( const QString & /*fileName*/, const KMimeType::Ptr & /*mime*/ ) { return false; };

        // capture events
        // return false if you don't wish okular to use its event handlers
        // in the pageview after your handling (use with caution)
        virtual bool handleEvent (QEvent * /*event*/ ) { return true; } ;

        void setDocument( Document * doc ) { m_document=doc; };

    signals:
        void error(const QString & string, int duration);
        void warning(const QString & string, int duration);
        void notice(const QString & string, int duration);

    protected:
        /** 'signals' to send events the Document **/
        // tell the document that the job has been completed
        void signalRequestDone( PixmapRequest * request ) { m_document->requestDone( request ); }

        Document * m_document;
};

/**
 * @short Describes a pixmap type request.
 */
struct OKULAR_EXPORT PixmapRequest
{
    PixmapRequest( int rId, int n, int w, int h, /*double z,*/ int p, bool a )
        : id( rId ), pageNumber( n ), width( w ), height( h ), /*zoom(z),*/
        priority( p ), async( a ), page( 0 )  {};

    PixmapRequest() {;};
    // observer id
    int id;
    // page number and size
    int pageNumber;
    int width;
    int height;
//    double zoom;
    // asyncronous request priority (less is better, 0 is max)
    int priority;
    // generate the pixmap in a thread and notify observer when done
    bool async;

    // these fields are set by the Document prior passing the
    // request to the generator
    int documentRotation;
    Page * page;

};

QTextStream& operator<< (QTextStream& str, const PixmapRequest *req);

/**
 * @short Defines an entry for the export menu
 */
struct OKULAR_EXPORT ExportEntry
{
    ExportEntry( const QString & desc, const KMimeType::Ptr & _mime )
        : description( desc ), mime( _mime ) {};

    ExportEntry( const QString & _icon, const QString & desc, const KMimeType::Ptr & _mime )
        : description( desc ), mime( _mime ), icon( _icon ) {};

    // the description to be shown in the Export menu
    QString description;
    // the mime associated
    KMimeType::Ptr mime;
    // the icon to be shown in the menu item
    QString icon;
};

}

#endif
