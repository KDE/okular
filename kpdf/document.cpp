/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt includes
#include <qfile.h>
#include <qmutex.h>
#include <qvaluevector.h>

// local includes
#include "PDFDoc.h"
//#include "TextOutputDev.h"

#include "kpdf_error.h"
#include "document.h"
#include "page.h"

// structure used internally by KPDFDocument for data storage
struct KPDFDocumentPrivate
{
    // document related
    QMutex docLock;
    PDFDoc * pdfdoc;
    int currentPage;
    QValueVector< KPDFPage* > pages;
    
    // observers related (note: won't delete oservers)
    QValueList< KPDFDocumentObserver* > observers;
};

#define foreachObserver( cmd ) {\
    QValueList<KPDFDocumentObserver*>::iterator it = d->observers.begin();\
    QValueList<KPDFDocumentObserver*>::iterator end = d->observers.end();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }

/*
 * KPDFDocument class
 */
KPDFDocument::KPDFDocument()
{
    d = new KPDFDocumentPrivate;
    d->pdfdoc = 0;
}
    
KPDFDocument::~KPDFDocument()
{
    close();
    delete d;
}

bool KPDFDocument::openFile( const QString & docFile )
{
    // docFile is always local so we can use QFile on it
    QFile fileReadTest( docFile );
    if ( !fileReadTest.open( IO_ReadOnly ) )
        return false;
    fileReadTest.close();

    GString *filename = new GString( QFile::encodeName( docFile ) );
    delete d->pdfdoc;
    d->pdfdoc = new PDFDoc( filename, 0, 0 );
    deletePages();
    
    if ( !d->pdfdoc->isOk() || d->pdfdoc->getNumPages() < 1 )
    {
        delete d->pdfdoc;
        d->pdfdoc = 0;
        return false;
    }

    // clear xpdf errors
    errors::clear();

    // build Pages
    uint pageCount = d->pdfdoc->getNumPages();
    d->pages.resize( pageCount );
    for ( uint i = 0; i < pageCount ; i++ )
        d->pages[i] = new KPDFPage( i, d->pdfdoc->getPageHeight(i + 1), d->pdfdoc->getPageWidth(i + 1) );

    //filter = NONE; TODO
    sendFilteredPageList();

    setCurrentPage( 0 );
    
    return true;
}

void KPDFDocument::close()
{
    //stopRunningJobs()...
    deletePages();
    delete d->pdfdoc;
    d->pdfdoc = 0;
}

const KPDFPage * KPDFDocument::page( uint n ) const
{
    return ( n < d->pages.count() ) ? d->pages[n] : 0;
}

uint KPDFDocument::pages() const
{
    return d->pdfdoc ? d->pdfdoc->getNumPages() : 0;
}

void KPDFDocument::setCurrentPage( uint page, float position )
{   //FIXME event queuing to avoid flow interruption (!!??)
    if ( (int)page == d->currentPage )   //TODO check position
        return;
    d->currentPage = page;
    foreachObserver( pageSetCurrent( page, position ) );
    pageChanged();
}

void KPDFDocument::find( bool /*nextMatch*/, const QString & /*text*/ )
{
/*
  TextOutputDev *textOut;
  Unicode *u;
  bool found;
  double xMin1, yMin1, xMax1, yMax1;
  int len, pg;

  // This is more or less copied from what xpdf does, surely can be optimized
  len = s.length();
  u = (Unicode *)gmalloc(len * sizeof(Unicode));
  for (int i = 0; i < len; ++i) u[i] = (Unicode)(s.latin1()[i] & 0xff);

  // search current
  found = m_outputDev->find(u, len, next);

  if (!found)
  {
    // search following pages
    textOut = new TextOutputDev(NULL, gTrue, gFalse, gFalse);
    if (!textOut->isOk())
    {
      gfree(u);
      delete textOut;
      return;
    }

    pg = m_currentPage + 1;
    while(!found && pg <= d->pdfdoc->getNumPages())
    {
      m_docMutex.lock();
      d->pdfdoc->displayPage(textOut, pg, 72, 72, 0, gTrue, gFalse);
      m_docMutex.unlock();
      found = textOut->findText(u, len, gTrue, gTrue, gFalse, gFalse, &xMin1, &yMin1, &xMax1, &yMax1);
      if (!found) pg++;
    }

    if (!found && m_currentPage != 1)
    {
      if (KMessageBox::questionYesNo(widget(), i18n("End of document reached.\nContinue from the beginning?")) == KMessageBox::Yes)
      {
        // search previous pages
        pg = 1;
        while(!found && pg < m_currentPage)
        {
          m_docMutex.lock();
          d->pdfdoc->displayPage(textOut, pg, 72, 72, 0, gTrue, gFalse);
          m_docMutex.unlock();
          found = textOut->findText(u, len, gTrue, gTrue, gFalse, gFalse, &xMin1, &yMin1, &xMax1, &yMax1);
          if (!found) pg++;
        }
      }
    }

    delete textOut;
    if (found)
    {
       kdDebug() << "found at " << pg << endl;
       goToPage(pg);
       // xpdf says: can happen that we do not find the text if coalescing is bad OUCH
       m_outputDev->find(u, len, false);
    }
    else
    {
        if (next) KMessageBox::information(widget(), i18n("No more matches found for '%1'.").arg(s));
        else KMessageBox::information(widget(), i18n("No matches found for '%1'.").arg(s));
    }
  }

  if (found) m_findText = s;
  else m_findText = QString::null;

  gfree(u);
*/
}

void KPDFDocument::goToLink( /* QString anchor */ )
{

}

void KPDFDocument::addObserver( KPDFDocumentObserver * pObserver )
{
    d->observers.push_back( pObserver );
}

void KPDFDocument::sendFilteredPageList()
{   //TODO implement filtering

    // make up a value list of the pages [1,2,3..]
    uint pageCount = d->pages.count();
    QValueList<int> pagesList;
    for ( uint i = 0; i < pageCount ; i++ )
        pagesList.push_back( i );

    // send the list to observers
    foreachObserver( pageSetup( pagesList ) );
}

void KPDFDocument::deletePages()
{
    if ( d->pages.isEmpty() )
        return;

    // broadcast an empty page list to observers
    QValueList<int> pagesList;
    foreachObserver( pageSetup( pagesList ) );

    // delete pages and clear container
    for ( uint i = 0; i < d->pages.count() ; i++ )
        delete d->pages[i];
    d->pages.clear();
    d->currentPage = -1;
}

#include "document.moc"
