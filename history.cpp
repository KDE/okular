// selection.cpp
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL

#include <kdebug.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <history.h>

history::history(void)
{
  backAct = forwardAct = 0;
  clear();
}

history::~history(void)
{
}

void history::add(Q_UINT32 page, Q_UINT32 ypos)
{
  if (numItems == 0) {
    historyList[0].page = page;
    historyList[0].ypos = ypos;
    numItems = 1;
    return;
  } else {
    if (historyList[currentItem].page == page) 
      return;

    if (currentItem == HISTORYLENGTH-1) {
      // Move all items forward one step, add item at the end
      for (int i=0; i<HISTORYLENGTH-1; i++)
	historyList[i] = historyList[i+1];
      historyList[HISTORYLENGTH-1].page = page;
      historyList[HISTORYLENGTH-1].ypos = ypos;
    } else {
      currentItem++;
      historyList[currentItem].page = page;
      historyList[currentItem].ypos = ypos;
      numItems = currentItem+1;
    }
  }

  if (backAct != 0)
    backAct->setEnabled((currentItem > 0)&&(numItems > 0));
  if (forwardAct != 0)
    forwardAct->setEnabled(false);
}

void history::setAction(KAction *back, KAction *forward)
{
  backAct    = back;
  forwardAct = forward;

  if (backAct != 0)
    backAct->setEnabled((currentItem > 0)&&(numItems > 0));
  if (forwardAct != 0)
    forwardAct->setEnabled(currentItem < numItems-1);
}

historyItem *history::forward()
{
  if (currentItem == numItems)
    return 0;
  else {
    currentItem++;
    if (backAct != 0)
      backAct->setEnabled(true);
    if (forwardAct != 0)
      forwardAct->setEnabled(currentItem < numItems-1);
    return historyList+currentItem;
  }
}

historyItem *history::back()
{
  if (currentItem == 0)
    return 0;
  else {
    currentItem--;
    if (backAct != 0)
      backAct->setEnabled((currentItem > 0)&&(numItems > 0));
    if (forwardAct != 0)
      forwardAct->setEnabled(true);
    return historyList+currentItem;
  }
}

void history::clear(void)
{
  currentItem = numItems = 0;
  if (backAct != 0)
    backAct->setEnabled(false);
  if (forwardAct != 0)
    forwardAct->setEnabled(false);
}
