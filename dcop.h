/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef MY_INTERFACE_H
#define MY_INTERFACE_H

#include <dcopobject.h>
#include <kurl.h>

class kpdf_dcop : virtual public DCOPObject
{
K_DCOP
	k_dcop:
		virtual ASYNC goToPage(uint page) = 0;
		virtual ASYNC openDocument(KURL doc) = 0;
		virtual uint pages() = 0;
		virtual uint currentPage() = 0;
		virtual KURL currentDocument() = 0;
    virtual void slotPreferences() = 0;
    virtual void slotFind() = 0;
    virtual void slotPrintPreview() = 0;
    virtual void slotPreviousPage() = 0;
    virtual void slotNextPage() = 0;
    virtual void slotGotoFirst() = 0;
    virtual void slotGotoLast() = 0;
    virtual void slotTogglePresentation() = 0;
};

#endif
