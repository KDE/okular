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
  ~selection(void);

  void          set(Q_INT32 selectedTextStart, Q_INT32 selectedTextEnd, QString text);
  void          setAction(KAction *act);
  void          clear(void);

  Q_INT32       selectedTextStart, selectedTextEnd;
  QString       selectedText;
  KAction *     act;
};
