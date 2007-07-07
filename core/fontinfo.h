/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_FONTINFO_H_
#define _OKULAR_FONTINFO_H_

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

#include <okular/core/okular_export.h>

namespace Okular {

class FontInfoPrivate;

/**
 * @short A small class that represents the information of a font.
 */
class OKULAR_EXPORT FontInfo
{
    public:
        typedef QList<FontInfo> List;

        /**
         * The possible kinds of fonts.
         */
        enum FontType
        {
            Unknown,
            Type1,
            Type1C,
            Type1COT,
            Type3,
            TrueType,
            TrueTypeOT,
            CIDType0,
            CIDType0C,
            CIDType0COT,
            CIDTrueType,
            CIDTrueTypeOT
        };

        /**
         * Construct a new empty font info.
         */
        FontInfo();
        /**
         * Copy constructor.
         */
        FontInfo( const FontInfo &fi );
        /**
         * Destructor.
         */
        ~FontInfo();

        /**
         * Returns the name of the font.
         */
        QString name() const;
        /**
         * Sets a new name for the font.
         */
        void setName( const QString& name );

        /**
         * Returns the type of the font.
         */
        FontType type() const;
        /**
         * Change the type of the font.
         */
        void setType( FontType type );

        /**
         * Returns whether the font is embedded into the document.
         */
        bool isEmbedded() const;
        /**
         * Sets whether the font is embedded into the document.
         */
        void setEmbedded( bool embedded );

        /**
         * In case of not embedded font, returns the path of the font that
         * represents this font.
         */
        QString file() const;
        void setFile( const QString& file );

        FontInfo& operator=( const FontInfo &fi );

        /**
         * Comparison operator.
         */
        bool operator==( const FontInfo &fi ) const;

        bool operator!=( const FontInfo &fi ) const;

    private:
        friend class FontInfoPrivate;
        QSharedDataPointer<FontInfoPrivate> d;
};

}

Q_DECLARE_METATYPE(Okular::FontInfo)

#endif
