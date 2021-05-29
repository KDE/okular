/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2003 Christophe Devriese <Christophe.Devriese@student.kuleuven.ac.be>
    SPDX-FileCopyrightText: 2003 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2003-2007 Albert Astals Cid <aacid@kde.org>
    SPDX-FileCopyrightText: 2004 Andy Goossens <andygoossens@telenet.be>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_MAIN_H_
#define _OKULAR_MAIN_H_

class QString;
class QStringList;

namespace Okular
{
enum Status { Error, AttachedOtherProcess, Success };

Status main(const QStringList &paths, const QString &serializedOptions);

}

#endif

/* kate: replace-tabs on; indent-width 4; */
