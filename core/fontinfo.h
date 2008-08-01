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
         * The possible kinds of embed.
         */
        enum EmbedType
        {
            NotEmbedded,
            EmbeddedSubset,
            FullyEmbedded
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
         * Returns the type of font embedding.
         */
        EmbedType embedType() const;
        /**
         * Sets the type of font embedding.
         */
        void setEmbedType( EmbedType type );

        /**
         * In case of not embedded font, returns the path of the font that
         * represents this font.
         */
        QString file() const;
        void setFile( const QString& file );

        /**
         * In case of embedded fonts, returns if the font can be extracted into a QByteArray
         *
         * @since 0.8 (KDE 4.2)
         */
        bool canBeExtracted() const;

        /**
         * Sets if a font can be extracted or not. False by default
         */
        void setCanBeExtracted( bool extractable );
     
        /**
         * Sets the "native" @p id of the font info.
         *
         * This is for use of the Generator, that can optionally store an
         * handle (a pointer, an identifier, etc) of the "native" font
         * object, if any.
         *
         * @since 0.8 (KDE 4.2)
         */
        void setNativeId( const QVariant &id );

        /**
         * Returns the "native" id of the font info.
         *
         * @since 0.8 (KDE 4.2)
         */
        QVariant nativeId() const;

        FontInfo& operator=( const FontInfo &fi );

        /**
         * Comparison operator.
         */
        bool operator==( const FontInfo &fi ) const;

        bool operator!=( const FontInfo &fi ) const;

    private:
        /// @cond PRIVATE
        friend class FontInfoPrivate;
        /// @endcond
        QSharedDataPointer<FontInfoPrivate> d;
};

}

Q_DECLARE_METATYPE(Okular::FontInfo)

#endif
