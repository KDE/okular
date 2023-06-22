/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "js_data_p.h"

#include <QDateTime>

#include "../document.h"
#include "js_display_p.h"

using namespace Okular;

QDateTime JSData::creationDate() const
{
    return m_file->creationDate();
}

QString JSData::description() const
{
    return m_file->description();
}

QString JSData::MIMEType() const
{
    return QLatin1String("");
}

QDateTime JSData::modDate() const
{
    return m_file->modificationDate();
}

QString JSData::name() const
{
    return m_file->name();
}

QString JSData::path() const
{
    return QLatin1String("");
}

int JSData::size() const
{
    return m_file->size();
}

JSData::JSData(EmbeddedFile *f, QObject *parent)
    : QObject(parent)
    , m_file(f)
{
}

JSData::~JSData() = default;
