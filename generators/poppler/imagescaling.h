/*
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_IMAGESCALING_H
#define OKULAR_IMAGESCALING_H

#include <QImage>

class imagescaling
{
public:
    static QImage scaleAndFitCanvas(const QImage &input, QSize expectedSize);
};

#endif // OKULAR_IMAGESCALING_H
