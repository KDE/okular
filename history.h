// history.h
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL

#include <kaction.h>
#include <qstring.h>

#define HISTORYLENGTH 10


class historyItem
{
 public: 
  Q_UINT32 page;
  Q_UINT32 ypos;
};

class history : public QObject
{
  Q_OBJECT

 public:
  history(void);
  ~history(void);

  void          add(Q_UINT32 page, Q_UINT32 ypos);
  void          setAction(KAction *back, KAction *forward);
  void          clear(void);

  historyItem   *forward();
  historyItem   *back();

 private:
  KAction      *backAct;
  KAction      *forwardAct;

  // List of history items. Here item[i+1] is a more recent item item
  // item[i].
  historyItem   historyList[HISTORYLENGTH];

  // If numItems == 0, the currentItem contains an arbitrary value. If
  // numItems > 0, this points to the element in the list, which
  // corresponds to the currently displayed value. Example: start with
  // an empty list, add two members, and currentItem will be 1
  // (remember: arrays count 0..n). Go back once and currentItem will
  // be 0.
  Q_INT16       currentItem;

  // Numer of items in use. 
  Q_INT16       numItems;
};
