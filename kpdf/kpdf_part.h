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
class KURL;
class KToggleAction;
class KSelectAction;

class LinkAction;
class LinkDest;
class PDFDoc;
class XOutputDev;

class PDFPartView;
class KPrinter;

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
   * @version 0.1
   */
  class Part : public KParts::ReadOnlyPart
  {
    Q_OBJECT

  public:
     // Do with these first. We can always add the other zoommodes which can
     // be specified in a Destination later.
     enum ZoomMode { FitInWindow, FitWidth, FitVisible, FixedFactor };

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

    bool closeURL();

      void displayPage(int pageNumber, float zoomFactor = 1.0);
    /*void displayDestination(LinkDest*);*/
      void updateActionPage();
      void setFullScreen( bool fs );
      PageWidget* pageWidget() const {return m_outputDev;}

  public slots:
    void print();


  protected:
    /**
     * This must be implemented by each part
     */
    virtual bool openFile();

      void update();
      void readSettings();
      void writeSettings();
      bool nextPage();
      bool previousPage();
      void updateAction();
      void goToPage( int page );
      void doPrint( KPrinter& printer );

  protected slots:
    void find();
    void findNext();
    void zoomIn()   { m_zoomFactor += 0.1; update(); };
    void zoomOut()  { m_zoomFactor -= 0.1; update(); };
    void back()     { /* stub */ };
    void forward()  { /* stub */ };
    void slotNextPage();
    void slotPreviousPage();
    void slotGotoEnd();
    void slotGotoStart();
    void slotGoToPage();
    void printPreview();

    /*void executeAction(LinkAction*);*/

      void showScrollBars( bool );
      void showMarkList( bool );
      void slotReadUp();
      void slotReadDown();
      void slotOpenUrlDropped( const KURL & );
      void slotZoom( const QString& );

  private:
      void doFind(QString s, bool next);
     
      PDFDoc*     m_doc;
      PageWidget* m_outputDev;
      PDFPartView * pdfpartview;
      KAction* m_firstPage;
      KAction* m_lastPage;
      KAction* m_prevPage;
      KAction* m_nextPage;
      KAction *m_gotoPage;
      KToggleAction* m_showScrollBars;
      KToggleAction* m_showPageList;
      KSelectAction *m_zoomTo;
      KToggleAction* m_fitToWidth;
      KAction *m_find;
      KAction *m_findNext;

      QString m_findText;

    // first page is page 1
    int   m_currentPage;
    QMutex m_docMutex;

    ZoomMode m_zoomMode;
    float    m_zoomFactor;

    static unsigned int m_count;

  private slots:
		void slotFitToWidthToggled();
		void redrawPage();
		void pageClicked ( int );
    void fileSaveAs();
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
