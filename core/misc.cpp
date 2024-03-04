/*
    SPDX-FileCopyrightText: 2005 Piotr Szymanski <niedakh@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "core/misc.h"

#include <QDebug>

#include "debug_p.h"

using namespace Okular;

class TextSelection::Private
{
public:
    int direction;
    NormalizedPoint cur[2];
};

TextSelection::TextSelection(const NormalizedPoint &start, const NormalizedPoint &end)
    : d(std::make_unique<Private>())
{
    if (end.y - start.y < 0 || (end.y - start.y == 0 && end.x - start.x < 0)) {
        d->direction = 1;
    } else {
        d->direction = 0;
    }

    d->cur[0] = start;
    d->cur[1] = end;
}

TextSelection::~TextSelection() = default;

NormalizedPoint TextSelection::start() const
{
    return d->cur[d->direction % 2];
}

NormalizedPoint TextSelection::end() const
{
    return d->cur[(d->direction + 1) % 2];
}
