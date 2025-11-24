/*
    SPDX-FileCopyrightText: 2005 Piotr Szymanski <niedakh@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_MISC_H_
#define _OKULAR_MISC_H_

#include "area.h"
#include "okularcore_export.h"

namespace Okular
{
/**
  @short Wrapper around the information needed to generate the selection area
*/
class OKULARCORE_EXPORT TextSelection
{
public:
    /**
     * Creates a new text selection with the given @p start and @p end point.
     */
    TextSelection(const NormalizedPoint &start, const NormalizedPoint &end);

    /**
     * Destroys the text selection.
     */
    ~TextSelection();

    TextSelection(const TextSelection &) = delete;
    TextSelection &operator=(const TextSelection &) = delete;

    /**
     * Returns the start point of the selection.
     */
    NormalizedPoint start() const;

    /**
     * Returns the end point of the selection.
     */
    NormalizedPoint end() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

OKULARCORE_EXPORT QString removeLineBreaks(const QString &text);
}

#endif
