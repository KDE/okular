/*
    SPDX-FileCopyrightText: 2006 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
