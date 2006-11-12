/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KDJVU_
#define _KDJVU_

#include <qcolor.h>
#include <qimage.h>
#include <qobject.h>
#include <qpolygon.h>
#include <qvariant.h>
#include <qvector.h>

class QDomDocument;

/**
 * @brief Qt (KDE) encapsulation of the DjVuLibre
 */
class KDjVu : public QObject
{
    Q_OBJECT
    public:
        KDjVu();
        ~KDjVu();

        /**
         * A DjVu page.
         */
        class Page
        {
            friend class KDjVu;

            public:
                ~Page();

                int width() const;
                int height() const;
                int dpi() const;
                int orientation() const;

            private:
                Page();

                int m_width;
                int m_height;
                int m_dpi;
                int m_orientation;
        };


        /**
         * The base implementation for a DjVu link.
         */
        class Link
        {
            friend class KDjVu;

            public:
                virtual ~Link();

                enum LinkType { PageLink, UrlLink };
                enum LinkArea { UnknownArea, RectArea, EllipseArea, PolygonArea };
                virtual int type() const = 0;
                LinkArea areaType() const;
                QPoint point() const;
                QSize size() const;
                QPolygon polygon() const;

            private:
                LinkArea m_area;
                QPoint m_point;
                QSize m_size;
                QPolygon m_poly;
        };

        /**
         * A link to reach a page of a DjVu document.
         */
        class PageLink : public Link
        {
            friend class KDjVu;

            public:
                virtual int type() const;
                QString page() const;

            private:
                PageLink();
                QString m_page;
        };

        /**
         * A DjVu link to open an external Url.
         */
        class UrlLink : public Link
        {
            friend class KDjVu;

            public:
                virtual int type() const;
                QString url() const;

            private:
                UrlLink();
                QString m_url;
        };


        /**
         * The base implementation for a DjVu annotation.
         */
        class Annotation
        {
            friend class KDjVu;

            public:
                virtual ~Annotation();

                enum AnnotationType { TextAnnotation, LineAnnotation };
                virtual int type() const = 0;
                QPoint point() const;
                QString comment() const;
                QColor color() const;

            private:
                QPoint m_point;
                QString m_comment;
                QColor m_color;
        };

        /**
         * A DjVu text annotation.
         */
        class TextAnnotation : public Annotation
        {
            friend class KDjVu;

            public:
                virtual int type() const;
                QSize size() const;
                bool inlineText() const;

            private:
                TextAnnotation();
                QSize m_size;
                bool m_inlineText;
        };

        /**
         * A DjVu line annotation.
         */
        class LineAnnotation : public Annotation
        {
            friend class KDjVu;

            public:
                virtual int type() const;
                QPoint point2() const;
                bool isArrow() const;
                int width() const;

            private:
                LineAnnotation();
                QPoint m_point2;
                bool m_isArrow;
                int m_width;
        };


        /**
         * Opens the file \p fileName, closing the old one if necessary.
         */
        bool openFile( const QString & fileName );
        /**
         * Close the file currently opened, if any.
         */
        void closeFile();

        /**
         * The pages of the current document, or an empty vector otherwise.
         * \note KDjVu handles the pages, so you don't need to delete them manually
         * \return a vector with the pages of the current document
         */
        const QVector<KDjVu::Page*> &pages() const;

        /**
         * Get the metadata for the specified \p key, or a null variant otherwise.
         */
        QVariant metaData( const QString & key ) const;

        /**
         * Get an XML document with the bookmarks of the current document (if any).
         * The XML will look like this:
         * \verbatim
         * <!DOCTYPE KDjVuBookmarks>
         * <item title="Title 1" destination="dest1">
         *   <item title="Title 1.1" destination="dest1.1" />
         *   ...
         * </item>
         * <item title="Title 2" destination="dest2">
         * \endverbatim
         */
        const QDomDocument * documentBookmarks() const;

        /**
         * Reads the links and the annotations for the page \p pageNum
         */
        void linksAndAnnotationsForPage( int pageNum, QList<KDjVu::Link*>& links, QList<KDjVu::Annotation*>& annotations ) const;

        // image handling
        /**
         * Check if the image for the specified \p page with the specified
         * \p width, \p height and \p rotation is already in cache, and returns
         * it. If not, a null image is returned.
         */
        QImage image( int page, int width, int height, int rotation );
        /**
         * Request to load the pixmap for \p page having the specified \p width,
         * \p height and \p rotation. It will emit pixmapGenerated() when done.
         */
        void requestImage( int page, int width, int height, int rotation );

    signals:
        /**
         * The image \p pix for page \p page was generated.
         */
        void imageGenerated( int page, const QImage & pix );

    private:
        class Private;
        Private * const d;
};

#endif
