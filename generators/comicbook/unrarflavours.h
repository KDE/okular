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

#include <QString>

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

        QStringList processListing( const QStringList &data ) override;
        QString name() const override;
};

class FreeUnrarFlavour : public UnrarFlavour
{
    public:
        FreeUnrarFlavour();

        QStringList processListing( const QStringList &data ) override;
        QString name() const override;
};

#endif

