//
// KDVIMiniWidget
//
// Previewer for TeX DVI files.
//

#include <qlayout.h>
#include <qdir.h>
#include <qfiledialog.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qapplication.h>
#include <qgroupbox.h>
#include <qfileinfo.h>
#include <qkeycode.h>
#include <qlineedit.h>
#include <qframe.h>

#include <kmenubar.h>
#include <kmessagebox.h>
#include <kapp.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kprocess.h>
#include <kfiledialog.h>

#include "kdvi_miniwidget.h"
#include "kdvi.h"
#include "scrbox.h"
#include "print.h"
#include "pushbutton.h"
#include "prefs.h"
#include "version.h"
#include <qsplitter.h>

#include <unistd.h>
#include <signal.h>
#include <kglobal.h>
#include <qdragobject.h>
#include <kconfig.h>

KDVIMiniWidget::KDVIMiniWidget( char *fname, QWidget *parent, const char *name )
    : QSplitter(QSplitter::Horizontal, parent, name)
{
	msg = NULL;
        hbl = NULL;
        ssmenu = NULL;

	readConfig();

        // Create a dvi window

	dviwin = new dviWindow( basedpi, mfmode, paper, makepk,
                                this, "dviWindow" );
	connect( dviwin, SIGNAL(currentPage(int)), SLOT(setPage(int)) );
	connect( dviwin, SIGNAL(fileChanged()), SLOT(fileChanged()) );

        makeToolBar2(this);
        moveToLast(dviwin);


	QValueList<int> size;
	size << 10 << 90;
	setSizes(size);

        // Read config options
        applyPreferences();

        selectSmall();
        dviwin->installEventFilter( this );

        openFile(QString(fname));
}

KDVIMiniWidget::~KDVIMiniWidget()
{
}

void KDVIMiniWidget::makeToolBar2(QWidget *parent)
{
	QPixmap pm;

	toolBar2 = new QFrame( parent );

	QBoxLayout * gl = new QBoxLayout( toolBar2, QBoxLayout::Down );

	sbox = new ScrollBox( toolBar2 );
	connect( sbox, SIGNAL(valueChanged(QPoint)),
		 dviwin, SLOT(scroll(QPoint)) );
	connect( sbox, SIGNAL(button3Pressed()), dviwin, SLOT(nextPage()) );
	connect( sbox, SIGNAL(button2Pressed()), dviwin, SLOT(prevPage()) );
	connect( dviwin, SIGNAL(pageSizeChanged( QSize )),
		 sbox, SLOT(setPageSize( QSize )) );
	connect( dviwin, SIGNAL(viewSizeChanged( QSize )),
		 sbox, SLOT(setViewSize( QSize )) );
        connect( dviwin, SIGNAL(currentPosChanged( QPoint )),
                 sbox, SLOT(setViewPos( QPoint )) );
	sbox->setFixedSize(70,80);
	gl->addWidget( sbox );

  // Create a MarkList

	marklist = new MarkList( toolBar2 );
	connect( marklist, SIGNAL(selected(const QString &)),
		SLOT(pageActivated(const QString &)) );
        gl->addWidget( marklist );
        gl->activate();

        sbox->setPageSize( dviwin->pageSize() );
}

void KDVIMiniWidget::updateMarkList()
{
	QString s;
	marklist->setAutoUpdate( FALSE );
	marklist->clear();
	for (int i = dviwin->totalPages(); i>0; i--)
	{
		s.sprintf( "%4d", i );
		marklist->insertItem( s, 0 );
	}
	marklist->select(0);
	marklist->setAutoUpdate( TRUE );
	marklist->update();
}


// This avoids seg fault at destructor:

void KDVIMiniWidget::closeEvent( QCloseEvent *e )
{
  QWidget::closeEvent(e);
  e->accept();
}


void KDVIMiniWidget::resizeEvent( QResizeEvent* e )
{
    QSplitter::resizeEvent( e );
	
    config->setGroup( "kdvi" );
    config->writeEntry( "Width", width() );
    config->writeEntry( "Height", height() );
}

bool KDVIMiniWidget::eventFilter( QObject *obj , QEvent *e )
{
    if ( obj != dviwin || e->type() != QEvent::MouseButtonPress )
        return FALSE;
    QMouseEvent *me = (QMouseEvent*)e;
    if ( me->button() != RightButton )
        return FALSE;
    return TRUE;
}


void KDVIMiniWidget::openFile( QString name)
{
	if ( name.isEmpty() )
		return;
        if (!QFileInfo(name).isReadable())
        {
            KMessageBox::sorry(this, i18n("Can't read file:\n") + name);
            return;
        }
        
        QDir::setCurrent( QFileInfo( name ).dirPath() );
        dviName = name.copy();
	message( i18n("Opening ") + name + " ...");

	dviwin->setFile( name );
        dviwin->repaint();
        setPage();
        updateMarkList();
	message( i18n("Opened ") + name );
}

void KDVIMiniWidget::filePrint()
{
	if (!dviName)
		return;

	print * printdlg = new print( this, "printdlg" );

        message( i18n("Print dialog is open") );
        printdlg->setFile( dviName );
        printdlg->setCurrentPage( dviwin->page(), dviwin->totalPages() );
        printdlg->setMarkList( marklist->markList() );
        printdlg->exec();
        message( "" );
        delete printdlg;
}

void KDVIMiniWidget::viewFitPage()
{
	resize( rect().size() + dviwin->pageSize() - dviwin->viewSize()	);
	resize( rect().size() + dviwin->pageSize() - dviwin->viewSize()	);
	resize( rect().size() + dviwin->pageSize() - dviwin->viewSize()	);

	message( i18n("View size fits page") );
}

void KDVIMiniWidget::viewFitPageWidth()
{
	QSize s = rect().size();
	resize( s.width() + dviwin->pageSize().width() - dviwin->viewSize().width(),
		s.height() );

	message( i18n("View width fits page") );
}


void KDVIMiniWidget::pageGoto()
{
	PageDialog dlg;
	connect( &dlg, SIGNAL(textEntered(const QString &)), SLOT(pageActivated(const QString &)) );
	dlg.show();
}

void KDVIMiniWidget::toggleMakePK()
{
	makepk = !makepk;
        applyMakePK();
        message( makepk ? i18n("Missing PK-fonts will be generated"):
                 i18n("Missing PK-fonts will be logged to 'missfont.log'") );
}

void KDVIMiniWidget::applyMakePK()
{
	if ( makepk == dviwin->makePK() )
            return;
        dviwin->setMakePK( makepk );
        config->writeEntry( "MakePK", makepk );
}

void KDVIMiniWidget::toggleShowPS()
{
	showPS = !showPS;
	applyShowPS();
	message( showPS ? i18n("Postcsript specials are rendered") :
		i18n("Postscript specials are not rendered") );
}

void KDVIMiniWidget::applyShowPS()
{
	if ( showPS == dviwin->showPS() )
		return;
	dviwin->setShowPS( showPS );
	config->writeEntry( "ShowPS", showPS );
}



void KDVIMiniWidget::pageActivated( const QString & text)
{
	int pg = text.toInt();
	if (dviwin->page() != pg)
		dviwin->gotoPage( pg );
	dviwin->setFocus();
}


void KDVIMiniWidget::selectShrink( QPoint p )
{
	sndr = sender()->name();
	if (!ssmenu)
	{
		ssmenu = new QPopupMenu;
		ssmenu->setMouseTracking( TRUE );
		connect( ssmenu, SIGNAL(activated(int)),
			 SLOT(selectShrink(int)) );
		QString s;
		for ( int i=1; i<=basedpi/20; i++ )
			ssmenu->insertItem( s.setNum( i ) );
	}
	ssmenu->popup( p - QPoint( 10, 10 ),
		(QString(sndr) == "largeButton" ? largeShrink : smallShrink) - 1 );
}

void KDVIMiniWidget::selectShrink( int id )
{
	if ( QString(sndr) == "largeButton" )
	{
		dviwin->setShrink( id + 1 );
		largeShrink = dviwin->shrink();
		config->writeEntry( "LargeShrink", largeShrink );
		message(i18n("Large text button set to shrink factor ") +
					QString().setNum(largeShrink) );
	}
	else
	{
		dviwin->setShrink( id + 1 );
		smallShrink = dviwin->shrink();
		config->writeEntry( "SmallShrink", smallShrink );
		message(i18n("Small text button set to shrink factor ") +
					QString().setNum(smallShrink) );
	}
}

void KDVIMiniWidget::selectLarge()
{
	dviwin->setShrink(largeShrink);
}

void KDVIMiniWidget::selectSmall()
{
	dviwin->setShrink(smallShrink);
}

void KDVIMiniWidget::fileChanged()
{
    message( i18n("File reloaded.") );
    if ( dviwin->totalPages() != marklist->count() )
        updateMarkList();
    setPage();
}

void KDVIMiniWidget::setPage(int pg)
{
    if ( marklist )
        marklist->select( pg - 1 );
}


void KDVIMiniWidget::message( const QString &s )
{
    emit statusMessage(s);
}


void KDVIMiniWidget::readConfig()
{
	QString s;
	config = KApplication::kApplication()->config();
        config->setGroup( "kdvi" );

	pannerValue = config->readNumEntry( "Separator" );
	if (!pannerValue)
		config->writeEntry( "Separator", pannerValue = 80 );
	
	showPS = config->readNumEntry( "ShowPS" );

	smallShrink = config->readNumEntry( "SmallShrink" );
	if (!smallShrink)
		config->writeEntry( "SmallShrink", smallShrink = 7 );

	largeShrink = config->readNumEntry( "LargeShrink" );
	if (!largeShrink)
		config->writeEntry( "LargeShrink", largeShrink = 3 );

	int width = config->readNumEntry( "Width" );
	if (!width)
		width = 500;

	int height = config->readNumEntry( "Height" );
	if (!height)
		height = 450;
	resize( width, height );
}

void KDVIMiniWidget::saveProperties(KConfig *config )
{
	config->writeEntry( "FileName", dviName );
	config->writeEntry( "Page", dviwin->page() );
	config->writeEntry( "Shrink", dviwin->shrink() );
	config->writeEntry( "Pos.x", dviwin->currentPos().x() );
	config->writeEntry( "Pos.y", dviwin->currentPos().y() );
}


void KDVIMiniWidget::readProperties(KConfig *config)
{
	QString file = config->readEntry("FileName");
	if ( file.isNull() )
		return;
	openFile( file );
	int page = config->readNumEntry( "Page", 1 );
        dviwin->gotoPage( page );
        setPage(page);
	dviwin->setShrink( config->readNumEntry( "Shrink" ) );
	dviwin->scroll( QPoint( config->readNumEntry( "Pos.x" ),
				config->readNumEntry( "Pos.y" ) ) );
}

void KDVIMiniWidget::applyPreferences()
{
	QString s;
	config->setGroup( "kdvi" );

	s = config->readEntry( "FontPath" );
	if ( !s.isEmpty() && s != dviwin->fontPath() )
		dviwin->setFontPath( s );

	basedpi = config->readNumEntry( "BaseResolution" );
	if ( basedpi <= 0 )
		config->writeEntry( "BaseResolution", basedpi = 300 );
	if ( basedpi != dviwin->resolution() )
		dviwin->setResolution( basedpi );

	mfmode =  config->readEntry( "MetafontMode" );
	if ( mfmode.isNull() )
		config->writeEntry( "MetafontMode", mfmode = "/" );
	if ( mfmode != dviwin->metafontMode() )
		dviwin->setMetafontMode( mfmode );

	paper = config->readEntry( "Paper" );
	if ( paper.isNull() )
		config->writeEntry( "Paper", paper = "A4" );
	if ( paper != dviwin->paper() )
		dviwin->setPaper( paper );

	s = config->readEntry( "Gamma" );
	if ( !s.isEmpty() && s.toFloat() != dviwin->gamma() )
		dviwin->setGamma( s.toFloat() );

	makepk = config->readNumEntry( "MakePK" );
	applyMakePK();

	showPS = config->readNumEntry( "ShowPS" );
	applyShowPS();
	
	dviwin->setAntiAlias( config->readNumEntry( "PS Anti Alias", 1 ) );

	smallShrink = config->readNumEntry( "SmallShrink" );
	if (!smallShrink) smallShrink = 6;

	largeShrink = config->readNumEntry( "LargeShrink" );
	if (!largeShrink) largeShrink = 2;

	message(i18n("Preferences applied"));
}


#include "kdvi_miniwidget.moc"

