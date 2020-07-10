/***************************************************************************
 *   Copyright (C) 2006 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PAGESIZELABEL_H_
#define _OKULAR_PAGESIZELABEL_H_

#include <KSqueezedTextLabel>

#include "core/observer.h"

namespace Okular
{
class Document;
}

/**
 * @short A widget to display page size.
 */
class PageSizeLabel : public KSqueezedTextLabel, public Okular::DocumentObserver
{
    Q_OBJECT

public:
    PageSizeLabel(QWidget *parent, Okular::Document *document);
    ~PageSizeLabel() override;

    // [INHERITED] from DocumentObserver
    void notifyCurrentPageChanged(int previous, int current) override;

private:
    Okular::Document *m_document;
};

#endif
