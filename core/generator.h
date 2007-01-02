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
#include <QtCore/QVariant>
#include <QtCore/QVector>

#include <kmimetype.h>

class KConfigDialog;
class KIcon;
class KPrinter;
class kdbgstream;

namespace Okular {

class Document;
class DocumentFonts;
class DocumentInfo;
class DocumentSynopsis;
class EmbeddedFile;
class Link;
class Page;
class PixmapRequest;
class TextPage;

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
 * @short Defines an entry for the export menu
 *
 * This class encapsulates information about an export format.
 * Every Generator can support 0 ore more export formats which can be
 * queried with @see Generator::exportFormats().
 */
class OKULAR_EXPORT ExportFormat
{
    public:
        typedef QList<ExportFormat> List;

        /**
         * Creates an empty export format.
         */
        ExportFormat();

        /**
         * Creates a new export format.
         *
         * @param description The i18n'ed description of the format.
         * @param mimeType The supported mime type of the format.
         */
        ExportFormat( const QString &description, const KMimeType::Ptr &mimeType );

        /**
         * Creates a new export format.
         *
         * @param icon The icon used in the GUI for this format.
         * @param description The i18n'ed description of the format.
         * @param mimeType The supported mime type of the format.
         */
        ExportFormat( const KIcon &icon, const QString &description, const KMimeType::Ptr &mimeType );

        /**
         * Destroys the export format.
         */
        ~ExportFormat();

        ExportFormat( const ExportFormat &other );
        ExportFormat& operator=( const ExportFormat &other );

        /**
         * Returns the description of the format.
         */
        QString description() const;

        /**
         * Returns the mime type of the format.
         */
        KMimeType::Ptr mimeType() const;

        /**
         * Returns the icon for GUI representations of the format.
         */
        KIcon icon() const;

    private:
        class Private;
        Private* const d;
};

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

        /**
         * This method returns whether the generator supports searching. Default is false.
         */
        virtual bool supportsSearching() const;

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
         * This method is called to print the document to the given @p printer.
         */
        virtual bool print( KPrinter &printer );

        /**
         * This method returns the meta data of the given @p key with the given @p option
         * of the document.
         */
        virtual QVariant metaData( const QString &key, const QVariant &option ) const;

        /**
         * Returns the list of additional supported export formats.
         */
        virtual ExportFormat::List exportFormats() const;

        /**
         * This method is called to export the document in the given @p format and save it
         * under the given @p fileName. The format must be one of the supported export formats.
         */
        virtual bool exportTo( const QString &fileName, const ExportFormat &format );

    Q_SIGNALS:
        /**
         * This signal should be emitted whenever an error occurred in the generator.
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
        class Private;
        Private* const d;
};

/**
 * @short Describes a pixmap type request.
 */
class OKULAR_EXPORT PixmapRequest
{
    friend class Document;

    public:
        /**
         * Creates a new pixmap request.
         *
         * @param id The observer id.
         * @param pageNumber The page number.
         * @param width The width of the page.
         * @param height The height of the page.
         * @param priority The priority of the request.
         * @param asynchronous The mode of generation.
         */
        PixmapRequest( int id, int pageNumber, int width, int height, int priority, bool asynchronous );

        /**
         * Destroys the pixmap request.
         */
        ~PixmapRequest();

        /**
         * Returns the observer id of the request.
         */
        int id() const;

        /**
         * Returns the page number of the request.
         */
        int pageNumber() const;

        /**
         * Returns the page width of the requested pixmap.
         */
        int width() const;

        /**
         * Returns the page height of the requested pixmap.
         */
        int height() const;

        /**
         * Returns the priority (less it better, 0 is maximum) of the
         * request.
         */
        int priority() const;

        /**
         * Returns whether the generation should be done synchronous or
         * asynchronous.
         *
         * If asynchronous, the pixmap is created in a thread and the observer
         * is notified when the job is done.
         */
        bool asynchronous() const;

        /**
         * Returns a pointer to the page where the pixmap shall be generated for.
         */
        Page *page() const;

    protected:
        /**
         * Internal usage.
         */
        void setPriority( int priority );

        /**
         * Internal usage.
         */
        void setAsynchronous( bool asynchronous );

        /**
         * Internal usage.
         */
        void setPage( Page *page );

        /**
         * Internal usage.
         */
        void swap();

    private:
        Q_DISABLE_COPY( PixmapRequest )

        class Private;
        Private* const d;
};

/**
 * @short Defines a source reference
 *
 * A source reference is a reference to one of the source(s) of the loaded
 * document.
 */
class OKULAR_EXPORT SourceReference
{
    public:
        /**
         * Creates a reference to the row @p row and column @p column of the
         * source @p fileName
         */
        SourceReference( const QString &fileName, int row, int column = 0 );

        ~SourceReference();

        /**
         * Returns the filename of the source.
         */
        QString fileName() const;

        /**
         * Returns the row of the position in the source file.
         */
        int row() const;

        /**
         * Returns the column of the position in the source file.
         */
        int column() const;

    private:
        class Private;
        Private* const d;
};

}

kdbgstream& operator<<( kdbgstream &str, const Okular::PixmapRequest &req );

#endif
