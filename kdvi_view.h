#ifndef __KDVI_VIEW_H
#define __KDVI_VIEW_H

#include <kbrowser.h>
#include <klibloader.h>

class KAction;
class KInstance;
class KDVIMiniWidget;
class KDVIBrowserExtension;

class KDVIFactory : public KLibFactory
{
  Q_OBJECT
public:
  KDVIFactory();
  virtual ~KDVIFactory();

  virtual QObject* create(QObject* parent = 0, const char* name = 0,
                          const char* classname = "QObject",
                          const QStringList &args = QStringList() );

  static KInstance *instance();

private:
  static KInstance *s_instance;
};

class KDVIPart: public KParts::ReadOnlyPart
{
  Q_OBJECT
public:
  KDVIPart( QWidget *parent = 0, const char *name = 0 );
  virtual ~KDVIPart();

  KDVIMiniWidget *miniWidget() const { return w; }
protected:
  // reimplemented from ReadOnlyPart
  virtual bool openFile();

protected slots:

private:
  KDVIMiniWidget *w;
  KDVIBrowserExtension * m_extension;

  KAction *startAct, *backAct, *forPageAct, *forwardAct,
      *finishAct, *zoomOutAct, *smallAct, *largeAct, *zoomInAct;
};


class KDVIBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT
  friend class KDVIPart; // emits our signals
public:
  KDVIBrowserExtension( KDVIPart *parent );
  virtual ~KDVIBrowserExtension() {}

  /*
  virtual void setXYOffset( int x, int y );
  virtual int xOffset();
  virtual int yOffset();
  */

public slots:
  // Automatically detected by konqueror
  void print();
};


#endif

