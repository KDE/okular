#ifndef __KDVI_VIEW_H
#define __KDVI_VIEW_H

#include <browser.h>
#include <klibloader.h>

class KInstance;
class KDVIMiniWidget;
class KDVIKonqView;

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

class KDVIPrintingExtension : public PrintingExtension
{
  Q_OBJECT
public:
    KDVIPrintingExtension( QObject *parent ) :
        PrintingExtension( parent, "KDVIPrintingExtension" ) {}
    virtual void print();
};

class KDVIKonqView: public BrowserView
{
  Q_OBJECT
public:
  KDVIKonqView();
  virtual ~KDVIKonqView();

  virtual void openURL(const QString &url, bool reload = false,
                       int xOffset = 0, int yOffset = 0);

  virtual QString url();
  virtual int xOffset();
  virtual int yOffset();
  virtual void stop();

  KDVIMiniWidget *miniWidget() const { return w; }

protected slots:
    void slotMessage(const QString &s);
    void slotFinished(int);
    void slotRedirection(int, const char *);
    void slotError(int, int, const char *);
protected:
  virtual void resizeEvent(QResizeEvent *);

private:
  int xOff, yOff;
  QString urlStr, destStr;
  KDVIMiniWidget *w;
  int jobId;

  KAction *startAct, *backAct, *forPageAct, *forwardAct,
      *finishAct, *zoomOutAct, *smallAct, *largeAct, *zoomInAct;
};



#endif

