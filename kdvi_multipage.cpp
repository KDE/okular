#include <qobject.h>
#include <qlabel.h>
#include <qscrollview.h>
#include <qimage.h>
#include <qpixmap.h>


#include <kinstance.h>
#include <klocale.h>
#include <kdebug.h>
#include <kimageeffect.h>
#include <kglobal.h>
#include <kconfig.h>


#include "print.h"
#include "optiondialog.h"
#include "kdvi_multipage.moc"


extern "C"
{
  void *init_libkdvi()
  {
    return new KDVIMultiPageFactory;
  }
};
 

KInstance *KDVIMultiPageFactory::s_instance = 0L;
 

KDVIMultiPageFactory::KDVIMultiPageFactory()
{
}

 
KDVIMultiPageFactory::~KDVIMultiPageFactory()
{
  if (s_instance)
    delete s_instance;
 
  s_instance = 0;
}


QObject *KDVIMultiPageFactory::create(QObject *parent, const char *name , const char*, const QStringList & )
{
  QObject *obj = new KDVIMultiPage((QWidget *)parent, name);
  emit objectCreated(obj);
  return obj;
}
 

KInstance *KDVIMultiPageFactory::instance()
{
  if (!s_instance)
    s_instance = new KInstance("kdvi");
  return s_instance;
} 


KDVIMultiPage::KDVIMultiPage(QWidget *parent, const char *name)
  : KMultiPage(parent, name), window(0), options(0)
{
  setInstance(KDVIMultiPageFactory::instance()); 

  window = new dviWindow(300, 1.0, "cx", 0, scrollView());
  preferencesChanged();

  new KAction(i18n("&DVI Options"), 0, this,
                       SLOT(doSettings()), actionCollection(),
                       "settings_dvi");

  setXMLFile("kdvi_part.rc");

  scrollView()->addChild(window);

  readSettings();
}


KDVIMultiPage::~KDVIMultiPage()
{
  writeSettings();
}


bool KDVIMultiPage::openFile()
{
  window->setFile(m_file);

  emit numberOfPages(window->totalPages());
  scrollView()->resizeContents(window->width(), window->height());

  emit previewChanged(true);

  return true;
}


bool KDVIMultiPage::closeURL()
{
  window->setFile("");
  scrollView()->resizeContents(0, 0);

  emit previewChanged(false);

  return true;
}


// test code
QStringList KDVIMultiPage::fileFormats()
{
  QStringList r;
  r << i18n("*.dvi|DVI files (*.dvi)");
  return r;
}


bool KDVIMultiPage::gotoPage(int page)
{
  window->gotoPage(page+1);

  emit previewChanged(true);

  return true;
}


double KDVIMultiPage::setZoom(double zoom)
{
  window->setZoom(zoom);
  scrollView()->resizeContents(window->width(), window->height());

  return zoom;
}


extern unsigned int page_w, page_h; 

double KDVIMultiPage::zoomForHeight(int height)
{
  return (window->zoom() * (double)height)/(double)page_h;
}


double KDVIMultiPage::zoomForWidth(int width)
{
  return (window->zoom() * (double)width)/(double)page_w;
}


void KDVIMultiPage::setPaperSize(double w, double h)
{
  window->setPaper(w, h);
}


bool KDVIMultiPage::preview(QPainter *p, int w, int h)
{
  QPixmap *map = window->pix();

  if (!map)
    return false;

  
  // TODO: use higher quality preview if anti-aliasing?
  //p->drawImage(0, 0, window->pix()->convertToImage().smoothScale(w,h));

  p->scale((double)w/(double)map->width(), (double)h/(double)map->height());
  p->drawPixmap(0, 0, *map);

  return true;
}


void KDVIMultiPage::doSettings()
{
  if (options)
    {
      options->show();
      return;
    }

  options = new OptionDialog(window);
  connect(options, SIGNAL(preferencesChanged()), this, SLOT(preferencesChanged()));
  options->show();
}


void KDVIMultiPage::preferencesChanged()
{
  KConfig *config = instance()->config();

  QString s;
  
  config->setGroup( "kdvi" );
 
  s = config->readEntry( "FontPath" );
  if ( !s.isEmpty() && s != window->fontPath() )
    window->setFontPath( s );
 
  int basedpi = config->readNumEntry( "BaseResolution" );
  if ( basedpi <= 0 )
    config->writeEntry( "BaseResolution", basedpi = 300 );
  if ( basedpi != window->resolution() )
    window->setResolution( basedpi );
 
  QString mfmode =  config->readEntry( "MetafontMode" );
  if ( mfmode.isNull() )
    config->writeEntry( "MetafontMode", mfmode = "/" );
  if ( mfmode != window->metafontMode() )
    window->setMetafontMode( mfmode );
  
  int makepk = config->readNumEntry( "MakePK" );
  if ( makepk != window->makePK() )
    window->setMakePK( makepk );                                                  
 
  int showPS = config->readNumEntry( "ShowPS" );
  if (showPS != window->showPS())
    window->setShowPS(showPS);
 
  window->setAntiAlias( config->readNumEntry( "PS Anti Alias", 1 ) );  
}


bool KDVIMultiPage::print(const QStrList &pages, int current)
{
  Print * printdlg = new Print(window, "printdlg");
 
  printdlg->setFile(m_file);
  printdlg->setCurrentPage(current+1, window->totalPages());
  printdlg->setMarkList(&pages);
  printdlg->exec();
  
  delete printdlg;       

  return true;
}
