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
#include <QStringList>

struct ProcessArgs {
    ProcessArgs();
    ProcessArgs(const QStringList &appArgs, bool useLsar);

    QStringList appArgs;
    bool useLsar;
};

class QStringList;

class UnrarFlavour
{
public:
    virtual ~UnrarFlavour();

    UnrarFlavour(const UnrarFlavour &) = delete;
    UnrarFlavour &operator=(const UnrarFlavour &) = delete;

    virtual QStringList processListing(const QStringList &data) = 0;
    virtual QString name() const = 0;

    virtual ProcessArgs processListArgs(const QString &fileName) const = 0;
    virtual ProcessArgs processOpenArchiveArgs(const QString &fileName, const QString &path) const = 0;

    void setFileName(const QString &fileName);

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

    QStringList processListing(const QStringList &data) override;
    QString name() const override;

    ProcessArgs processListArgs(const QString &fileName) const override;
    ProcessArgs processOpenArchiveArgs(const QString &fileName, const QString &path) const override;
};

class FreeUnrarFlavour : public UnrarFlavour
{
public:
    FreeUnrarFlavour();

    QStringList processListing(const QStringList &data) override;
    QString name() const override;

    ProcessArgs processListArgs(const QString &fileName) const override;
    ProcessArgs processOpenArchiveArgs(const QString &fileName, const QString &path) const override;
};

class UnarFlavour : public UnrarFlavour
{
public:
    UnarFlavour();

    QStringList processListing(const QStringList &data) override;
    QString name() const override;

    ProcessArgs processListArgs(const QString &fileName) const override;
    ProcessArgs processOpenArchiveArgs(const QString &fileName, const QString &path) const override;
};

#endif
