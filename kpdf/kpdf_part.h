/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Andy Goossens <andygoossens@telenet.be>         *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_PART_H_
#define _KPDF_PART_H_

#include <qmutex.h>

#include <kparts/browserextension.h>
#include <kparts/part.h>

class QWidget;

class KAboutData;
class KAction;
class KPrinter;
class KURL;
class KToggleAction;
class KSelectAction;

class LinkAction;
class LinkDest;
class XOutputDev;

class ThumbnailList;
class PDFPartView;

class KPDFDocument;

namespace KPDF
{
  class BrowserExtension;
  class PageWidget;

  /**
   * This is a "Part".  It that does all the real work in a KPart
   * application.
   *
   * @short Main Part
   * @author Wilco Greven <greven@kde.org>
   * @version 0.2
   */
  class Part : public KParts::ReadOnlyPart
  {
	Q_OBJECT

  public:

	/**
	* Default constructor
	*/
	Part(QWidget* parentWidget, const char* widgetName,
	     QObject* parent, const char* name, const QStringList& args);

	/**
	* Destructor
	*/
	virtual ~Part();

	static KAboutData* createAboutData();

	// reimplemented from KParts::ReadOnlyPart
	bool closeURL();

	void displayPage(int pageNumber ); //TODO REMOVE ME!

  protected:
	// reimplemented from KParts::ReadOnlyPart
	virtual bool openFile();

	void readSettings();
	void writeSettings();
	void updateAction();
	void doPrint( KPrinter& printer );

  protected slots:
	// connected to actions
	void slotGoToPage();
	void slotPreviousPage();
	void slotNextPage();
	void slotGotoStart();
	void slotGotoEnd();
	void slotFind();
	void slotFindNext();
	void slotZoom( const QString& );
	void slotZoomIn();
	void slotZoomOut();
	void slotFitToWidthToggled( bool );
	void slotPrintPreview();
	void slotToggleScrollBars( bool );
	void slotToggleThumbnails( bool );
	void slotSaveFileAs();
	// can be connected do widget elements
	void updateActions();

  public slots:
	// connected to Shell action (and browserExtension), not local one
	void slotPrint(); 

  private:

// PORT!! ###
// PDFDoc*     m_doc;
QMutex m_docMutex; // REMOVE MEEE

	// the document
	KPDFDocument * document;

	// main widgets
	ThumbnailList *m_thumbnailList;
	PageWidget *m_pageWidget;

	// static instances counter
	static unsigned int m_count;

	// actions
	KAction *m_gotoPage;
	KAction *m_prevPage;
	KAction *m_nextPage;
	KAction *m_firstPage;
	KAction *m_lastPage;
	KToggleAction *m_showScrollBars;
	KToggleAction *m_showPageList;
	KSelectAction *m_zoomTo;
	KToggleAction *m_fitToWidth;
	KAction *m_find;
	KAction *m_findNext;
  };

  class BrowserExtension : public KParts::BrowserExtension
  {
	Q_OBJECT

  public:
	BrowserExtension(Part*);

  public slots:
	// Automatically detected by the host.
	void print();
  };

}

#endif

// vim:ts=2:sw=2:tw=78:et
