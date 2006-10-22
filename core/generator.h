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
         OKULAR_EXPORT Okular::Generator* create_plugin() { return new classname(); } \
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
class kdbgstream;

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
        /**
         * Creates a new generator.
         */
        Generator();


        /**
         * Destroys the generator.
         */
        virtual ~Generator();

        /**
         * Sets the current document the generator should work on.
         */
        void setDocument( Document * document );

        /**
         * Loads the document with the given @p fileName and fills the
         * @p pagesVector with the parsed pages.
         *
         * @returns true on success, false otherwise.
         */
        virtual bool loadDocument( const QString & fileName, QVector< Page * > & pagesVector ) = 0;

        /**
         * This method is called when the document is closed and not used
         * any longer.
         *
         * @returns true on success, false otherwise.
         */
        virtual bool closeDocument() = 0;

        /**
         * This method returns whether the generator can create pixmaps for
         * each page in a synchronous or asynchronous way, depending on @p async.
         */
        virtual bool canGeneratePixmap( bool async ) const = 0;

        /**
         * This method is called to create a pixmap for a page. The page number,
         * width and height is encapsulated in the page @p request.
         */
        virtual void generatePixmap( PixmapRequest * request ) = 0;

        /**
         * This method returns whether the generator can create text pages,
         * which are used for search and text extraction.
         *
         * @returns false as default.
         */
        virtual bool canGenerateTextPage() const;

        /**
         * This method is called to create a so called 'text page' for the given @p page.
         *
         * A text page is an abstract description of the readable text of the page.
         * It's used for search and text extraction.
         */
        virtual void generateSyncTextPage( Page *page );

        /**
         * Returns the general information object of the document or 0 if
         * no information are available.
         */
        virtual const DocumentInfo * generateDocumentInfo();

        /**
         * Returns the 'table of content' object of the document or 0 if
         * no table of content is available.
         */
        virtual const DocumentSynopsis * generateDocumentSynopsis();

        /**
         * Returns the 'list of embedded fonts' object of the document or 0 if
         * no list of embedded fonts is available.
         */
        virtual const DocumentFonts * generateDocumentFonts();

        /**
         * Returns the 'list of embedded files' object of the document or 0 if
         * no list of embedded files is available.
         */
        virtual const QList<EmbeddedFile*> * embeddedFiles() const;

        /**
         * This enum identifies the metric of the page size.
         *
         * @li None - page size is not defined in a physical metric
         * @li Points - page size is given in 1/72 inches
         */
        enum PageSizeMetric
        {
          None,
          Points
        };

        /**
         * This method returns the metric of the page size. Default is @see None.
         */
        virtual PageSizeMetric pagesSizeMetric() const;

        /**
         * This method returns whether given action (@see Document::Permission) is
         * allowed in this document.
         */
        virtual bool isAllowed( int action ) const;

        // gui stuff
        virtual QString getXMLFile() const;
        virtual void setupGUI( KActionCollection*, QToolBox* );
        virtual void freeGUI();

        /**
         * This method returns whether the generator supports searching. Default is false.
         */
        virtual bool supportsSearching() const;

        /**
         * This method returns whether the generator prefers internal searching. Default is false.
         */
        virtual bool prefersInternalSearching() const;

        /**
         * This method returns the rectangular of the area where the given @p text can be found
         * on the given @p page.
         *
         * The search can be influenced by the parameters @p direction, @p caseSensitive and
         * @p lastRect
         *
         * If no match is found, 0 is returned.
         */
        virtual RegularAreaRect * findText( const QString &text, SearchDir direction, const bool caseSensitive,
                                            const RegularAreaRect *lastRect, Page *page ) const;

        /**
         * This method returns the text which is enclosed by the given @p area on the given @p page.
         */
        virtual QString getText( const RegularAreaRect *area, Page *page ) const;

        /**
         * Returns whether the generator supports rotation of the pages. Default is false.
         */
        virtual bool supportsRotation() const;

        /**
         * This method is called when the orientation has been changed by the user.
         */
        virtual void rotationChanged( int orientation, int oldOrientation );

        /**
         * Returns whether the generator supports paper sizes. Default is false.
         */
        virtual bool supportsPaperSizes() const;

        /**
         * Returns the list of supported paper sizes.
         */
        virtual QStringList paperSizes() const;

        /**
         * This method is called to set the paper size of the given @p pages
         * to the given @p paperSize.
         */
        virtual void setPaperSize( QVector<Page*> &pages, int paperSize );

        /**
         * Returns true if the generator configures the printer itself, false otherwise.
         */
        virtual bool canConfigurePrinter() const;

        /**
         * This method is called to print the document to the given @p printer.
         */
        virtual bool print( KPrinter &printer );

        /**
         * This method returns the meta data of the given @p key with the given @p option
         * of the document.
         */
        virtual QString metaData( const QString &key, const QString &option ) const;

        /**
         * This method is called to tell the generator to re-parse its configuration.
         *
         * Returns true if something has changed.
         */
        virtual bool reparseConfig();

        /**
         * This method allows the generator to add custom configuration pages to the
         * config @p dialog of okular.
         */
        virtual void addPages( KConfigDialog *dialog );

        /**
         * Returns whether the generator can export the document to text format.
         */
        virtual bool canExportToText() const;

        /**
         * This method is called to export the document as text format and save it
         * under the given @p fileName.
         */
        virtual bool exportToText( const QString &fileName );

        /**
         * Returns the list of additional supported export formats.
         */
        virtual QList<ExportEntry*> exportFormats() const;

        /**
         * This method is called to export the document in the given @p mimeType and save it
         * under the given @p fileName. The mime type must be one of the supported export formats.
         */
        virtual bool exportTo( const QString &fileName, const KMimeType::Ptr &mimeType );

        // TODO: remove
        virtual bool handleEvent (QEvent * /*event*/ );

    Q_SIGNALS:
        /**
         * This signal should be emitted whenever an error occured in the generator.
         *
         * @param message The message which should be shown to the user.
         * @param duration The time that the message should be shown to the user.
         */
        void error( const QString &message, int duration );

        /**
         * This signal should be emitted whenever the user should be warned.
         *
         * @param message The message which should be shown to the user.
         * @param duration The time that the message should be shown to the user.
         */
        void warning( const QString &message, int duration );

        /**
         * This signal should be emitted whenever the user should be noticed.
         *
         * @param message The message which should be shown to the user.
         * @param duration The time that the message should be shown to the user.
         */
        void notice( const QString &message, int duration );

    protected:
        /**
         * This method must be called when the pixmap request triggered by @see generatePixmap()
         * has been finished.
         */
        void signalRequestDone( PixmapRequest * request );

        /**
         * Returns a pointer to the document.
         */
        Document * document() const;

    private:
        /**
         * The internal pointer to the document.
         */
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

kdbgstream& operator<<( kdbgstream &str, const Okular::PixmapRequest &req );

#endif
