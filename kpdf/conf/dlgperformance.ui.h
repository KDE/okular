/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

#include <klocale.h>


void DlgPerformance::init()
{
    QFont labelFont = descLabel->font();
    labelFont.setBold( true );
    descLabel->setFont( labelFont );
}

void DlgPerformance::lowRadio_toggled( bool on )
{
    if ( on ) descLabel->setText( i18n("Keeps used memory as low as possible. Don't reuse anything. (for systems with low memory)") );
}

void DlgPerformance::normalRadio_toggled( bool on )
{
    if ( on ) descLabel->setText( i18n("A good compromise between memory usage and speed gain. Preload next page and boost searches. (for systems with 256MB of memory typically)") );
}

void DlgPerformance::aggressiveRadio_toggled( bool on )
{
    if ( on ) descLabel->setText( i18n("Keeps everything in mempory. Preload next pages. Boost searches. (for systems with more than 512MB of memory)") );
}

