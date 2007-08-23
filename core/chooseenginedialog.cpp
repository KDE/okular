/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "chooseenginedialog_p.h"

#include <QtGui/QComboBox>
#include <QtGui/QLabel>

#include <klocale.h>

#include "ui_chooseenginewidget.h"

ChooseEngineDialog::ChooseEngineDialog( const QStringList &generators, const KMimeType::Ptr &mime, QWidget * parent )
    : KDialog( parent )
{
    setCaption( i18n( "Backend Selection" ) );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );
    QWidget *main = new QWidget( this );
    setMainWidget( main );
    m_widget = new Ui_ChooseEngineWidget();
    m_widget->setupUi( main );

    m_widget->engineList->addItems(generators);

    m_widget->description->setText(
        i18n( "<qt>More than one backend found for the MIME type:<br />"
              "<b>%1</b> (%2).<br /><br />"
              "Please select which one to use:</qt>", mime->comment(), mime->name() ) );
}

ChooseEngineDialog::~ChooseEngineDialog()
{
    delete m_widget;
}

int ChooseEngineDialog::selectedGenerator() const
{
    return m_widget->engineList->currentIndex();
}
