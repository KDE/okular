// selection.cpp
//
// Part of KDVI - A previewer for TeX DVI files.
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL

#include <kdebug.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <selection.h>
#include <selection.moc>

selection::selection(void)
{
  act               = 0;
  clear();
}

selection::~selection(void)
{
}

void selection::set(Q_INT32 start, Q_INT32 end, QString text)
{
  selectedTextStart = start;
  selectedTextEnd   = end;
  selectedText      = text;

  QApplication::clipboard()->setSelectionMode(true);
  QApplication::clipboard()->setText(selectedText);

  if (act != 0)
    act->setEnabled(!selectedText.isEmpty());
}

void selection::setAction(KAction *a)
{
  act               = a;
  if (act != 0)
    act->setEnabled(!selectedText.isEmpty());
}

void selection::clear(void)
{
  set(-1, -1, QString::null);
}
