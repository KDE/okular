/***************************************************************************
 *   Copyright (C) 2019 by Bubli                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "certificatetools.h"
#include <iostream>

CertificateTools::CertificateTools( QWidget * parent )
    : WidgetConfigurationToolsBase( parent )
{
}

QStringList CertificateTools::tools() const
{
    QStringList res;
    return res;
}

void CertificateTools::setTools(const QStringList& /*items*/)
{
    return;
}

void CertificateTools::slotAdd()
{
    std::cout << "add" << std::endl;
}

void CertificateTools::slotEdit()
{
    std::cout << "edit" << std::endl;
}
