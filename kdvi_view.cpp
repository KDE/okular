#include "kdvi_view.h"
//#include "konq_progressproxy.h"
#include "kdvi_miniwidget.h"
#include <kinstance.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kaction.h>
#include <kurl.h>
#include <kdebug.h>
#include <qfile.h>

extern "C"{
    void *init_libkdvi()
    {
        return new KDVIFactory;
    }
};

KInstance *KDVIFactory::s_instance = 0L;

KDVIFactory::KDVIFactory()
{
}

KDVIFactory::~KDVIFactory()
{
    if ( s_instance )
        delete s_instance;

    s_instance = 0;
}

QObject* KDVIFactory::create(QObject *parent , const char *name , const char*,
                            const QStringList & )
{
    QObject *obj = new KDVIPart( (QWidget *)parent, name );
    emit objectCreated( obj );
    return obj;
}

KInstance *KDVIFactory::instance()
{
    if ( !s_instance )
        s_instance = new KInstance( "kdvi" );
    return s_instance;
}

KDVIPart::KDVIPart( QWidget *parent, const char *name )
 : KParts::ReadOnlyPart( parent, name )
{
    setInstance( KDVIFactory::instance() );
    w = new KDVIMiniWidget(NULL, parent );
    // Clicking on it should make it active (required by KParts)
    w->setFocusPolicy( QWidget::ClickFocus );

    dviWindow *dviwin = w->window();
    setWidget( w );

    startAct = new KAction(i18n("Go to first page"),
                           QIconSet(BarIcon("start", KDVIFactory::instance())) ,
                           0, dviwin, SLOT(firstPage() ), actionCollection(), "firstPage");
    backAct = new KAction(i18n("Go to previous page"),
                          QIconSet(BarIcon("back", KDVIFactory::instance())) ,
                          0, dviwin, SLOT(prevPage() ), actionCollection(), "prevPage");
    forPageAct = new KAction(i18n("Go down then top of next page"),
                             QIconSet(UserIcon("forwpage", KDVIFactory::instance())) ,
                             0, dviwin, SLOT(goForward() ), actionCollection(), "goForward");
    forwardAct = new KAction(i18n("Go to next page"),
                             QIconSet(BarIcon("forward", KDVIFactory::instance())) ,
                             0, dviwin, SLOT(nextPage() ), actionCollection(), "nextPage");
    finishAct = new KAction(i18n("Go to last page"),
                            QIconSet(BarIcon("finish", KDVIFactory::instance())) ,
                            0, dviwin, SLOT(lastPage() ), actionCollection(), "lastPage");
    zoomOutAct = new KAction(i18n("Decrease magnification"),
                             QIconSet(BarIcon("viewmag-", KDVIFactory::instance())) ,
                             0, dviwin, SLOT(nextShrink() ), actionCollection(), "nextShrink");
    smallAct = new KAction(i18n("Small text"),
                           QIconSet(UserIcon("smalltext", KDVIFactory::instance())) ,
                           0, w, SLOT(selectSmall() ), actionCollection(), "selectSmall");
    largeAct = new KAction(i18n("Large text"),
                           QIconSet(UserIcon("largetext", KDVIFactory::instance())) ,
                           0, w, SLOT(selectLarge() ), actionCollection(), "selectLarge");
    zoomInAct = new KAction(i18n("Increase magnification"),
                            QIconSet(BarIcon("viewmag+", KDVIFactory::instance())) ,
                            0, dviwin, SLOT(prevShrink() ), actionCollection(), "prevShrink");

    m_extension = new KDVIBrowserExtension( this );

    connect(w, SIGNAL(statusMessage(const QString &)),
            this, SIGNAL( setStatusBarText( const QString & ) ) );
    connect(w->window(), SIGNAL(statusChange(const QString &)),
            this, SIGNAL( setStatusBarText( const QString & ) ) );

    setXMLFile( "kdvi_part.rc" );

}

KDVIPart::~KDVIPart()
{
}

bool KDVIPart::openFile()
{
  if(!QFile::exists(m_file))
  {
    kDebugWarning("KDVIKonqView: dest file %s does not exist!",
                  m_file.latin1());
    return false;
  }
  else
  {
    w->openFile(m_file);
    return true;
  }
}

/////////
KDVIBrowserExtension::KDVIBrowserExtension( KDVIPart *parent ) :
  KParts::BrowserExtension( parent, "KDVIBrowserExtension" )
{
}

void KDVIBrowserExtension::print()
{
    ((KDVIPart *)parent())->miniWidget()->filePrint();
}


#include "kdvi_view.moc"

