#ifndef selection_h
#define selection_h

// selection.h
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL

#include <kaction.h>
#include <qstring.h>

class selection : public QObject
{
  Q_OBJECT

public:
  selection(void);

  void          set(Q_UINT16 pageNr, Q_INT32 selectedTextStart, Q_INT32 selectedTextEnd, QString text);
  void          setAction(KAction *act);
  void          clear(void);

  /** page==0 means 'invalid page', nothing is selected */
  Q_UINT16      page;

  Q_INT32       selectedTextStart, selectedTextEnd;
  QString       selectedText;
  KAction *     act;

public slots:
  void          copyText(void);

signals:
  /** Passed through to the top-level kpart. */
  void          pageChanged(void);
};

#endif
