/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef UNRARFLAVOURS_H
#define UNRARFLAVOURS_H

#include <QtCore/QString>

class QStringList;

class UnrarFlavour
{
    public:
        virtual ~UnrarFlavour();

        virtual QStringList processListing( const QStringList &data ) = 0;
        virtual QString name() const = 0;

        void setFileName( const QString &fileName );

    protected:
        UnrarFlavour();

        QString fileName() const;

    private:
        QString mFileName;
};

class NonFreeUnrarFlavour : public UnrarFlavour
{
    public:
        NonFreeUnrarFlavour();

        virtual QStringList processListing( const QStringList &data );
        virtual QString name() const;
};

class FreeUnrarFlavour : public UnrarFlavour
{
    public:
        FreeUnrarFlavour();

        virtual QStringList processListing( const QStringList &data );
        virtual QString name() const;
};

#endif

