#include <qobject.h>
#include <qlabel.h>
#include <qscrollview.h>


#include <kinstance.h>
#include <klocale.h>
#include <kdebug.h>


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
  : KMultiPage(parent, name)
{
  setInstance(KDVIMultiPageFactory::instance()); 

  window = new dviWindow(300, 1.0, "cx", "a4", 0, scrollView());

  scrollView()->addChild(window);
}


KDVIMultiPage::~KDVIMultiPage()
{
}


bool KDVIMultiPage::openFile()
{
  window->setFile(m_file);

  emit numberOfPages(window->totalPages());
  scrollView()->resizeContents(window->width(), window->height());

  return true;
}


// test code
QStringList KDVIMultiPage::fileFormats()
{
  QStringList r;
  r << "*.dvi|DVI files (*dvi)";
  return r;
}


bool KDVIMultiPage::gotoPage(int page)
{
  window->gotoPage(page);
  return true;
}


void KDVIMultiPage::setZoom(double zoom)
{
  window->setZoom(zoom);
  scrollView()->resizeContents(window->width(), window->height());
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
