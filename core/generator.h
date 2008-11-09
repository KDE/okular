/***************************************************************************
 *   Copyright (C) 2004-5 by Enrico Ros <eros.kde@email.it>                *
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *   Copyright (C) 2008   by Albert Astals Cid <aacid@kde.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_H_
#define _OKULAR_GENERATOR_H_

#include <okular/core/okular_export.h>
#include <okular/core/fontinfo.h>
#include <okular/core/global.h>
#include <okular/core/pagesize.h>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QVector>

#include <kmimetype.h>
#include <kpluginfactory.h>

#define OKULAR_EXPORT_PLUGIN( classname, aboutdata ) \
    K_PLUGIN_FACTORY( classname ## Factory, registerPlugin< classname >(); ) \
    K_EXPORT_PLUGIN( classname ## Factory( aboutdata ) )

class QByteArray;
class QMutex;
class QPrinter;
class QPrintDialog;
class KIcon;

namespace Okular {

class Document;
class DocumentFonts;
class DocumentInfo;
class DocumentSynopsis;
class EmbeddedFile;
class ExportFormatPrivate;
class FontInfo;
class GeneratorPrivate;
class Page;
class PixmapRequest;
class PixmapRequestPrivate;
class TextPage;
class NormalizedRect;

/* Note: on contents generation and asynchronous queries.
 * Many observers may want to request data syncronously or asynchronously.
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
 * Every Generator can support 0 or more export formats which can be
 * queried with @ref Generator::exportFormats().
 */
class OKULAR_EXPORT ExportFormat
{
    public:
        typedef QList<ExportFormat> List;

        /**
         * Creates an empty export format.
         *
         * @see isNull()
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

        /**
         * @internal
         */
        ExportFormat( const ExportFormat &other );

        /**
         * @internal
         */
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

        /**
         * Returns whether the export format is null/valid.
         *
         * An ExportFormat is null if the mimetype is not valid or the
         * description is empty, or both.
         */
        bool isNull() const;

        /**
         * Type of standard export format.
         */
        enum StandardExportFormat
        {
            PlainText,         ///< Plain text
            PDF,               ///< PDF, aka Portable Document Format
            OpenDocumentText,  ///< OpenDocument Text format @since 0.8 (KDE 4.2)
            HTML   ///< OpenDocument Text format @since 0.8 (KDE 4.2)
        };

        /**
         * Builds a standard format for the specified @p type .
         */
        static ExportFormat standardFormat( StandardExportFormat type );

        bool operator==( const ExportFormat &other ) const;

        bool operator!=( const ExportFormat &other ) const;

    private:
        /// @cond PRIVATE
        friend class ExportFormatPrivate;
        /// @endcond
        QSharedDataPointer<ExportFormatPrivate> d;
};

/**
 * @short [Abstract Class] The information generator.
 *
 * Most of class members are virtuals and some of them pure virtual. The pure
 * virtuals provide the minimal functionalities for a Generator, that is being
 * able to generate QPixmap for the Page 's of the Document.
 *
 * Implementing the other functions will make the Generator able to provide
 * more contents and/or functionalities (like text extraction).
 *
 * Generation/query is requested by the Document class only, and that
 * class stores the resulting data into Page s. The data will then be
 * displayed by the GUI components (PageView, ThumbnailList, etc..).
 *
 * @see PrintInterface, ConfigInterface, GuiInterface
 */
class OKULAR_EXPORT Generator : public QObject
{
    /// @cond PRIVATE
    friend class PixmapGenerationThread;
    friend class TextPageGenerationThread;
    /// @endcond

    Q_OBJECT

    public:
        /**
         * Describe the possible optional features that a Generator can
         * provide.
         */
        enum GeneratorFeature
        {
            Threaded,
            TextExtraction,    ///< Whether the Generator can extract text from the document in the form of TextPage's
            ReadRawData,       ///< Whether the Generator can read a document directly from its raw data.
            FontInfo,          ///< Whether the Generator can provide information about the fonts used in the document
            PageSizes,         ///< Whether the Generator can change the size of the document pages.
            PrintNative,       ///< Whether the Generator supports native cross-platform printing (QPainter-based).
            PrintPostscript,   ///< Whether the Generator supports postscript-based file printing.
            PrintToFile        ///< Whether the Generator supports export to PDF & PS through the Print Dialog
        };

        /**
         * Creates a new generator.
         */
        Generator( QObject *parent, const QVariantList &args );

        /**
         * Destroys the generator.
         */
        virtual ~Generator();

        /**
         * Loads the document with the given @p fileName and fills the
         * @p pagesVector with the parsed pages.
         *
         * @returns true on success, false otherwise.
         */
        virtual bool loadDocument( const QString & fileName, QVector< Page * > & pagesVector ) = 0;

        /**
         * Loads the document from the raw data @p fileData and fills the
         * @p pagesVector with the parsed pages.
         *
         * @note the Generator has to have the feature @ref ReadRawData enabled
         *
         * @returns true on success, false otherwise.
         */
        virtual bool loadDocumentFromData( const QByteArray & fileData, QVector< Page * > & pagesVector );

        /**
         * This method is called when the document is closed and not used
         * any longer.
         *
         * @returns true on success, false otherwise.
         */
        bool closeDocument();

        /**
         * This method returns whether the generator is ready to
         * handle a new pixmap request.
         */
        virtual bool canGeneratePixmap() const;

        /**
         * This method can be called to trigger the generation of
         * a new pixmap as described by @p request.
         */
        virtual void generatePixmap( PixmapRequest * request );

        /**
         * This method returns whether the generator is ready to
         * handle a new text page request.
         */
        virtual bool canGenerateTextPage() const;

        /**
         * This method can be called to trigger the generation of
         * a text page for the given @p page.
         *
         * The generation is done synchronous or asynchronous, depending
         * on the @p type parameter and the capabilities of the
         * generator (e.g. multithreading).
         *
         * @see TextPage
         */
        virtual void generateTextPage( Page * page );

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
         * Returns the 'list of embedded fonts' object of the specified \page
         * of the document.
         *
         * \param page a page of the document, starting from 0 - -1 indicates all
         * the other fonts
         */
        virtual FontInfo::List fontsForPage( int page );

        /**
         * Returns the 'list of embedded files' object of the document or 0 if
         * no list of embedded files is available.
         */
        virtual const QList<EmbeddedFile*> * embeddedFiles() const;

        /**
         * This enum identifies the metric of the page size.
         */
        enum PageSizeMetric
        {
          None,   ///< The page size is not defined in a physical metric.
          Points  ///< The page size is given in 1/72 inches.
        };

        /**
         * This method returns the metric of the page size. Default is @ref None.
         */
        virtual PageSizeMetric pagesSizeMetric() const;

        /**
         * This method returns whether given @p action (@ref Permission) is
         * allowed in this document.
         */
        virtual bool isAllowed( Permission action ) const;

        /**
         * This method is called when the orientation has been changed by the user.
         */
        virtual void rotationChanged( Rotation orientation, Rotation oldOrientation );

        /**
         * Returns the list of supported page sizes.
         */
        virtual PageSize::List pageSizes() const;

        /**
         * This method is called when the page size has been changed by the user.
         */
        virtual void pageSizeChanged( const PageSize &pageSize, const PageSize &oldPageSize );

        /**
         * This method is called to print the document to the given @p printer.
         */
        virtual bool print( QPrinter &printer );

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

        /**
         * Query for the specified @p feature.
         */
        bool hasFeature( GeneratorFeature feature ) const;

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
         * This method must be called when the pixmap request triggered by generatePixmap()
         * has been finished.
         */
        void signalPixmapRequestDone( PixmapRequest * request );

        /**
         * This method must be called when a text generation has been finished.
         */
        void signalTextGenerationDone( Page *page, TextPage *textPage );

        /**
         * This method is called when the document is closed and not used
         * any longer.
         *
         * @returns true on success, false otherwise.
         */
        virtual bool doCloseDocument() = 0;

        /**
         * Returns the image of the page as specified in
         * the passed pixmap @p request.
         *
         * @warning this method may be executed in its own separated thread if the
         * @ref Threaded is enabled!
         */
        virtual QImage image( PixmapRequest *page );

        /**
         * Returns the text page for the given @p page.
         *
         * @warning this method may be executed in its own separated thread if the
         * @ref Threaded is enabled!
         */
        virtual TextPage* textPage( Page *page );

        /**
         * Returns a pointer to the document.
         */
        const Document * document() const;

        /**
         * Toggle the @p feature .
         */
        void setFeature( GeneratorFeature feature, bool on = true );

        /**
         * Request a meta data of the Document, if available, like an internal
         * setting.
         */
        QVariant documentMetaData( const QString &key, const QVariant &option = QVariant() ) const;

        /**
         * Return the pointer to a mutex the generator can use freely.
         */
        QMutex* userMutex() const;

        /**
         * Set the bounding box of a page after the page has already been handed
         * to the Document. Call this instead of Page::setBoundingBox() to ensure
         * that all observers are notified.
         *
         * @since 0.7 (KDE 4.1)
         */
        void updatePageBoundingBox( int page, const NormalizedRect & boundingBox );

    protected Q_SLOTS:
        /**
         * Gets the font data for the given font
         *
         * @since 0.8 (KDE 4.1)
         */
        void requestFontData(const Okular::FontInfo &font, QByteArray *data);

    protected:
        /// @cond PRIVATE
        Generator( GeneratorPrivate &dd, QObject *parent, const QVariantList &args );
        Q_DECLARE_PRIVATE( Generator )
        GeneratorPrivate *d_ptr;

        friend class Document;
        friend class DocumentPrivate;
        /// @endcond PRIVATE

    private:
        Q_DISABLE_COPY( Generator )

        Q_PRIVATE_SLOT( d_func(), void pixmapGenerationFinished() )
        Q_PRIVATE_SLOT( d_func(), void textpageGenerationFinished() )
};

/**
 * @short Describes a pixmap type request.
 */
class OKULAR_EXPORT PixmapRequest
{
    friend class Document;
    friend class DocumentPrivate;

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

    private:
        Q_DISABLE_COPY( PixmapRequest )

        friend class PixmapRequestPrivate;
        PixmapRequestPrivate* const d;
};

}

#ifndef QT_NO_DEBUG_STREAM
OKULAR_EXPORT QDebug operator<<( QDebug str, const Okular::PixmapRequest &req );
#endif

#endif
