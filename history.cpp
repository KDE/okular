// history.cpp
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL

#include <kdebug.h>

#include "history.h"

HistoryItem::HistoryItem(Q_UINT32 p, Q_UINT32 y)
  : page(p), ypos(y)
{
}

bool HistoryItem::operator == (const HistoryItem& item)
{
  return page == item.page && ypos == item.ypos;
}

History::History()
{
}

void History::add(Q_UINT32 page, Q_UINT32 ypos)
{
  HistoryItem item(page, ypos);

  if (historyList.empty())
  {
    currentItem = historyList.append(item);
  }
  else
  {
    // Don't add the same item several times in a row
    if (item == *currentItem)
      return;

      currentItem++;
    if (currentItem == historyList.end())
    {
      currentItem = historyList.append(item);
    }
    else
    {
      currentItem = historyList.insert(currentItem, item);
    }
    // Delete items starting after currentItem to the end of the list.
    QValueList<HistoryItem>::iterator deleteItemsStart = currentItem;
    deleteItemsStart++;
    historyList.erase(deleteItemsStart, historyList.end()); 

    if (historyList.size() > HISTORYLENGTH)
      historyList.pop_front();
  }
  emit backItem(currentItem != historyList.begin());
  emit forwardItem(false);
}

HistoryItem* History::forward()
{
  if (historyList.empty() || currentItem == historyList.fromLast())
    return 0;
    
    currentItem++;
  emit backItem(true);
  emit forwardItem(currentItem != historyList.fromLast());
  return &(*currentItem);
}

HistoryItem* History::back()
{
  if (historyList.empty() || currentItem == historyList.begin())
    return 0;
  
    currentItem--;
  emit backItem(currentItem != historyList.begin());
  emit forwardItem(true);
  return &(*currentItem);
}

void History::clear(void)
{
  historyList.clear();
  currentItem = historyList.begin();
  emit backItem(false);
  emit forwardItem(false);
}

#include "history.moc"
