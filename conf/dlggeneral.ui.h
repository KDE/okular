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

#include <kapplication.h>

#include <config.h>

void DlgGeneral::showEvent( QShowEvent * )
{
#if KPDF_FORCE_DRM
    kcfg_ObeyDRM->hide();
#else
    if (kapp->authorize("skip_drm")) kcfg_ObeyDRM->show();
    else kcfg_ObeyDRM->hide();
#endif
}

