#ifndef history_h
#define history_h

// history.h
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL

#include <qobject.h>
#include <qvaluelist.h>

#define HISTORYLENGTH 10

class HistoryItem
{
 public:
  HistoryItem(Q_UINT32, Q_UINT32);
  HistoryItem() {};

  bool operator == (const HistoryItem& item);

  Q_UINT32 page;
  Q_UINT32 ypos;
};

class History : public QObject
{
  Q_OBJECT

public:
  History(void);

  void          add(Q_UINT32 page, Q_UINT32 ypos);
  void          clear();

  HistoryItem*  forward();
  HistoryItem*  back();

signals:
  void backItem(bool);
  void forwardItem(bool);

private:
  // List of history items. First item is the oldest.
  QValueList<HistoryItem> historyList;

  QValueList<HistoryItem>::iterator currentItem;
};

#endif
