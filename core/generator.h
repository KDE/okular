/***************************************************************************
 *   Copyright (C) 2004-5 by Enrico Ros <eros.kde@email.it>                *
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_GENERATOR_H_
#define _KPDF_GENERATOR_H_

#include "okular_export.h"

#define KPDF_EXPORT_PLUGIN( classname ) \
    extern "C" { \
         OKULAR_EXPORT Generator* create_plugin(KPDFDocument* doc) { return new classname(doc); } \
    }


#include <qobject.h>
#include <qvector.h>
#include <qstring.h>
#include <ostream>
#include "document.h"
#include "textpage.h"
class KPrinter;
class KPDFPage;
class KPDFLink;
class PixmapRequest;
class KConfigDialog;

/* Note: on contents generation and asyncronous queries.
 * Many observers may want to request data syncronously or asyncronously.
 * - Sync requests. These should be done in-place.
 * - Async request must be done in real background. That usually means a
 *   thread, such as QThread derived classes.
 * Once contents are available, they must be immediately stored in the
 * KPDFPage they refer to, and a signal is emitted as soon as storing
 * (even for sync or async queries) has been done.
 */

/**
 * @short [Abstract Class] The information generator.
 *
 * Most of class members are pure virtuals and they must be implemented to
 * provide a class that builds contents (graphics and text).
 *
 * Generation/query is requested by the 'KPDFDocument' class only, and that
 * class stores the resulting data into 'KPDFPage's. The data will then be
 * displayed by the GUI components (pageView, thumbnailList, etc..).
 */
class OKULAR_EXPORT Generator : public QObject
{
    public:
        /** virtual methods to reimplement **/
        // load a document and fill up the pagesVector
        virtual bool loadDocument( const QString & fileName, QVector< KPDFPage * > & pagesVector ) = 0;

        // page contents generation
        virtual bool canGeneratePixmap( bool async ) = 0;
        virtual void generatePixmap( PixmapRequest * request ) = 0;

        // can generate a KPDFText Page
        virtual bool canGenerateTextPage() { return false; };
        virtual void generateSyncTextPage( KPDFPage * /*page*/ ) {;};

        // Document description and Table of contents
        virtual const DocumentInfo * generateDocumentInfo() { return 0L; }
        virtual const DocumentSynopsis * generateDocumentSynopsis() { return 0L; }
        virtual const DocumentFonts * generateDocumentFonts() { return 0L; }

        // DRM handling
        virtual bool isAllowed( int /*Document::Permisison(s)*/ ) { return true; }

        // gui stuff
        virtual QString getXMLFile() { return QString::null; } ;
        virtual void setupGUI(KActionCollection  * /*ac*/ , QToolBox * /*tBox*/ ) {;};
        virtual void freeGUI( ) {;};
        // capability querying

        // provides internal search 
        virtual bool supportsSearching() { return false; };
        virtual bool prefersInternalSearching() { return false; };

        // rotation
        virtual bool supportsRotation() { return false; };
        virtual void setOrientation(QVector<KPDFPage*> & /*pagesVector*/, int /*orientation*/) { ; };
        virtual bool supportsPaperSizes () { return false; }
        virtual QStringList paperSizes ()  { return QStringList(); }
        virtual void setPaperSize (QVector<KPDFPage*> & /*pagesVector*/, int /*newsize*/) { ; }

        // internal search and gettext
        virtual RegularAreaRect * findText( const QString & /*text*/, SearchDir /*dir*/, const bool /*strictCase*/,
                    const RegularAreaRect * /*lastRect*/, KPDFPage * /*page*/) { return 0L; };
        virtual QString* getText( const RegularAreaRect * /*area*/, KPDFPage * /*page*/ ) { return 0L; }

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

        // capture events
        // return false if you don't wish kpdf to use its event handlers
        // in the pageview after your handling (use with caution)
        virtual bool handleEvent (QEvent * /*event*/ ) { return true; } ;

        void setDocument( KPDFDocument * doc ) { m_document=doc; };

        /** 'signals' to send events the KPDFDocument **/
        // tell the document that the job has been completed
        void signalRequestDone( PixmapRequest * request ) { m_document->requestDone( request ); }

        /** constructor: takes the Document as a parameter **/
        Generator( KPDFDocument * doc ) : m_document( doc ) {};

    signals:
        void error(QString & string, int duration);
        void warning(QString & string, int duration);
        void notice(QString & string, int duration);

    protected:
        KPDFDocument * m_document;
    private:
        Generator();
};

/**
 * @short Describes a pixmap type request.
 */
struct OKULAR_EXPORT PixmapRequest
{
    PixmapRequest( int rId, int n, int w, int h, /*double z,*/ int r, int p, bool a = false )
        : id( rId ), pageNumber( n ), width( w ), height( h ), /*zoom(z),*/
        rotation(r) , priority( p ), async( a ), page( 0 )  {};

    PixmapRequest() {;};
    // observer id
    int id;
    // page number and size
    int pageNumber;
    int width;
    int height;
//    double zoom;
    int rotation;
    // asyncronous request priority (less is better, 0 is max)
    int priority;
    // generate the pixmap in a thread and notify observer when done
    bool async;

    // this field is set by the Docuemnt prior passing the
    // request to the generator
    KPDFPage * page;

};

QTextStream& operator<< (QTextStream& str, const PixmapRequest *req);

#endif
