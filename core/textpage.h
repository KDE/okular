/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TEXTPAGE_H_
#define _OKULAR_TEXTPAGE_H_

#include <QtCore/QList>
#include <QtCore/QString>

#include <okular/core/okular_export.h>
#include <okular/core/global.h>

class QMatrix;

namespace Okular {

class NormalizedRect;
class Page;
class PagePrivate;
class TextPagePrivate;
class TextSelection;
class RegularAreaRect;

/*! @class TextEntity
 * @short Abstract textentity of Okular
 * @par The context
 * A document can provide different forms of information about textual representation
 * of its contents. It can include information about positions of every character on the
 * page, this is the best possibility.
 *
 * But also it can provide information only about positions of every word on the page (not the character).
 * Furthermore it can provide information only about the position of the whole page's text on the page.
 *
 * Also some document types have glyphes - sets of characters rendered as one, so in search they should
 * appear as a text but are only one character when drawn on screen. We need to allow this.
 */
class OKULAR_EXPORT TextEntity
{
    public:
        typedef QList<TextEntity*> List;

        /**
         * Creates a new text entity with the given @p text and the
         * given @p area.
         */
        TextEntity( const QString &text, NormalizedRect *area );

        /**
         * Destroys the text entity.
         */
        ~TextEntity();

        /**
         * Returns the text of the text entity.
         */
        QString text() const;

        /**
         * Returns the bounding area of the text entity.
         */
        NormalizedRect* area() const;

        /**
         * Returns the transformed area of the text entity.
         */
        NormalizedRect transformedArea(const QMatrix &matrix) const;

    private:
        QString m_text;
        NormalizedRect* m_area;

        class Private;
        const Private *d;

        Q_DISABLE_COPY( TextEntity )
};

/**
 * The TextPage class represents the text of a page by
 * providing @see TextEntity items for every word/character of
 * the page.
 */
class OKULAR_EXPORT TextPage
{
    /// @cond PRIVATE
    friend class Page;
    friend class PagePrivate;
    /// @endcond

    public:
        /**
         * Creates a new text page.
         */
        TextPage();

        /**
         * Creates a new text page with the given @p words.
         */
        TextPage( const TextEntity::List &words );

        /**
         * Destroys the text page.
         */
        ~TextPage();

        /**
         * Appends the given @p text with the given @p area as new
         * @ref TextEntity to the page.
         */
        void append( const QString &text, NormalizedRect *area );

        /**
         * Returns the bounding rect of the text which matches the following criteria
         * or 0 if the search is not successful.
         *
         * @param id An unique id for this search.
         * @param text The search text.
         * @param direction The direction of the search (@ref SearchDirection)
         * @param caseSensitivity If Qt::CaseSensitive, the search is case sensitive; otherwise
         *                        the search is case insensitive.
         * @param lastRect If 0 the search starts at the beginning of the page, otherwise
         *                 right/below the coordinates of the given rect.
         */
        RegularAreaRect* findText( int id, const QString &text, SearchDirection direction,
                                   Qt::CaseSensitivity caseSensitivity, const RegularAreaRect *lastRect );

        /**
         * Text extraction function.
         *
         * Returns:
         * - a null string if @p rect is a valid pointer to a null area
         * - the whole page text if @p rect is a null pointer
         * - the text which is included by rectangular area @p rect otherwise
         */
        QString text( const RegularAreaRect *rect = 0 ) const;

        /**
         * Returns the rectangular area of the given @p selection.
         */
        RegularAreaRect *textArea( TextSelection *selection ) const;

    private:
        TextPagePrivate* const d;

        Q_DISABLE_COPY( TextPage )
};

}

#endif 
