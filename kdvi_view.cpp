#include "kdvi_view.h"
#include "konq_progressproxy.h"
#include "kdvi_miniwidget.h"
#include <kinstance.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kio_job.h>
#include <kio_cache.h>
#include <kurl.h>
#include <qfile.h>

#include <config.h>
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif
 
#ifndef _PATH_TMP
#define _PATH_TMP "/tmp/"
#endif       


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

QObject* KDVIFactory::create(QObject* , const char* , const char*,
                            const QStringList & )
{
    QObject *obj = new KDVIKonqView;
    emit objectCreated( obj );
    return obj;
}

KInstance *KDVIFactory::instance()
{
    if ( !s_instance )
        s_instance = new KInstance( "kdvi" );
    return s_instance;
}

KDVIKonqView::KDVIKonqView()
{
    urlStr = "";
    w = new KDVIMiniWidget(NULL, this );
    dviWindow *dviwin = w->window();
    jobId = 0;
    startAct = new KAction(i18n("Go to first page"),
                           QIconSet(BarIcon("start", KDVIFactory::instance())) ,
                           0, dviwin, SLOT(firstPage() ), this);
    backAct = new KAction(i18n("Go to previous page"),
                          QIconSet(BarIcon("back", KDVIFactory::instance())) ,
                          0, dviwin, SLOT(prevPage() ), this);
    forPageAct = new KAction(i18n("Go down then top of next page"),
                             QIconSet(BarIcon("forwpage", KDVIFactory::instance())) ,
                             0, dviwin, SLOT(goForward() ), this);
    forwardAct = new KAction(i18n("Go to next page"),
                             QIconSet(BarIcon("forward", KDVIFactory::instance())) ,
                             0, dviwin, SLOT(nextPage() ), this);
    finishAct = new KAction(i18n("Go to last page"),
                            QIconSet(BarIcon("finish", KDVIFactory::instance())) ,
                            0, dviwin, SLOT(lastPage() ), this);
    zoomOutAct = new KAction(i18n("Decrease magnification"),
                             QIconSet(BarIcon("viewmag-", KDVIFactory::instance())) ,
                             0, dviwin, SLOT(nextShrink() ), this);
    smallAct = new KAction(i18n("Small text"),
                           QIconSet(BarIcon("smalltext", KDVIFactory::instance())) ,
                           0, w, SLOT(selectSmall() ), this);
    largeAct = new KAction(i18n("Large text"),
                           QIconSet(BarIcon("largetext", KDVIFactory::instance())) ,
                           0, w, SLOT(selectLarge() ), this);
    zoomInAct = new KAction(i18n("Increase magnification"),
                            QIconSet(BarIcon("viewmag+", KDVIFactory::instance())) ,
                            0, dviwin, SLOT(prevShrink() ), this);

    actions()->append(BrowserView::ViewAction(startAct, BrowserView::MenuEdit
                                               | BrowserView::ToolBar ) );
    actions()->append(BrowserView::ViewAction(backAct, BrowserView::MenuEdit
                                               | BrowserView::ToolBar ) );
    actions()->append(BrowserView::ViewAction(forPageAct, BrowserView::MenuEdit
                                               | BrowserView::ToolBar ) );
    actions()->append(BrowserView::ViewAction(forwardAct, BrowserView::MenuEdit
                                               | BrowserView::ToolBar ) );
    actions()->append(BrowserView::ViewAction(finishAct, BrowserView::MenuEdit
                                               | BrowserView::ToolBar ) );
    actions()->append(BrowserView::ViewAction(zoomOutAct, BrowserView::MenuEdit
                                               | BrowserView::ToolBar ) );
    actions()->append(BrowserView::ViewAction(smallAct, BrowserView::MenuEdit
                                              | BrowserView::ToolBar ) );
    actions()->append(BrowserView::ViewAction(largeAct, BrowserView::MenuEdit
                                              | BrowserView::ToolBar ) );
    actions()->append(BrowserView::ViewAction(zoomInAct, BrowserView::MenuEdit
                                              | BrowserView::ToolBar ) );

    (void)new KDVIPrintingExtension( this );

    connect(w, SIGNAL(statusMessage(const QString &)), this,
            SLOT(slotMessage(const QString &)));
    connect(w->window(), SIGNAL(statusChange(const QString &)), this,
            SLOT(slotMessage(const QString &)));
    
}

KDVIKonqView::~KDVIKonqView()
{
    stop();
    warning("In KDVIKonqView destructor");
    KURL destURL(destStr);
    if(QFile::exists(destURL.path()))
        QFile::remove(destURL.path());
}

void KDVIKonqView::openURL(const QString &url, bool, int, int)
{
    urlStr = url;
    KIOCachedJob *iojob = new KIOCachedJob;
    iojob->setGUImode(KIOJob::NONE);
    jobId = iojob->id();
    connect(iojob, SIGNAL(sigFinished(int)), this,
            SLOT(slotFinished(int)));
    connect(iojob, SIGNAL(sigRedirection(int, const char *)), this,
            SLOT(slotRedirection(int, const char *)));
    connect(iojob, SIGNAL(sigError(int, int, const char *)), this,
            SLOT(slotError(int, int, const char *)));

    (void)new KonqProgressProxy( this, iojob );
    destStr.sprintf("file:"_PATH_TMP"/kdvi%i", time( 0L ));
    iojob->copy(url.latin1(), destStr.latin1());
    emit started();
}

QString KDVIKonqView::url()
{
    return urlStr;
}

int KDVIKonqView::xOffset()
{
    return 0;
}

int KDVIKonqView::yOffset()
{
    return 0;
}

void KDVIKonqView::stop()
{
    if (jobId){
        KIOJob *job = KIOJob::find(jobId);
        if (job)
            job->kill();
        jobId = 0;
    }
}

void KDVIKonqView::slotMessage(const QString &s)
{
    emit setStatusBarText(s);
}

void KDVIKonqView::slotFinished( int )
{

    KURL destURL(destStr);
    if(!QFile::exists(destURL.path()))
        warning("KDVIKonqView: dest file %s does not exist!",
                destURL.path().latin1());
    else
        w->openFile(destURL.path());
    jobId = 0;
    emit completed();
}

void KDVIKonqView::slotRedirection( int, const char *url )
{
    emit setLocationBarURL(QString(url));
}

void KDVIKonqView::slotError( int, int, const char * )
{
    stop();
    emit canceled();
}

void KDVIKonqView::resizeEvent( QResizeEvent * )
{
    w->setGeometry(0, 0, width(), height() );
}

void KDVIPrintingExtension::print()
{
    ((KDVIKonqView *)parent())->miniWidget()->filePrint();
}


#include "kdvi_view.moc"



