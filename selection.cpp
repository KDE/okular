// selection.cpp
//
// Part of KDVI - A previewer for TeX DVI files.
//
// (C) 2001-2004 Stefan Kebekus
// Distributed under the GPL

#include <kdebug.h>
#include <qapplication.h>
#include <qclipboard.h>
#include "selection.h"
#include "selection.moc"

selection::selection(void)
{
  act            = 0;
  page           = 0;
  clear();
}


void selection::set(Q_UINT16 pageNr, Q_INT32 start, Q_INT32 end, QString text)
{
  Q_UINT16 oldpage  = page;
  page              = pageNr;
  selectedTextStart = start;
  selectedTextEnd   = end;
  if (page != 0)
    selectedText = text;
  else
    selectedText = QString::null;

  if (page != 0) {
    QApplication::clipboard()->setSelectionMode(true);
    QApplication::clipboard()->setText(selectedText);
  }
  
  if (act != 0)
    act->setEnabled(!selectedText.isEmpty());

  emit selectionIsNotEmpty( !isEmpty() );

  if (page != oldpage)
    emit pageChanged();
}


void selection::copyText(void)
{
  QApplication::clipboard()->setSelectionMode(false);
  QApplication::clipboard()->setText(selectedText);
}


void selection::setAction(KAction *a)
{
  act               = a;
  if (act != 0)
    act->setEnabled(!selectedText.isEmpty());
}


void selection::clear(void)
{
  set(0, -1, -1, QString::null);
}
