//
// Class: kdvi
//
// Previewer for TeX DVI files.
//

//#define KDVI_VERSION "0.4.1"

#include <qframe.h>
#include <qlabel.h>
#include <qcombo.h>
#include <qpopmenu.h>
#include "dviwin.h"
#include <qapp.h>
#include <kapp.h>
#include "scrbox.h"
#include "marklist.h"
#include <qaccel.h>
#include <qlayout.h>
#include <qdialog.h>
#include <qlined.h>
#include <qpushbt.h>
#include "prefs.h"

#include <ktopwidget.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <kkeydialog.h>
#include <kaccel.h>

class KPanner;
class QToolTipGroup;

class kdvi : public KTopLevelWidget
{
	Q_OBJECT

public:
	kdvi(char *fname=0, QWidget *parent=0,const char *name=0 );
	~kdvi();

public slots:
	void		shrinkChanged(int);
	void		fileChanged();
	void		applyPreferences();
	void		saveProperties(KConfig*);
	void		readProperties(KConfig*);

protected:
	void		resizeEvent( QResizeEvent* e );
	bool		eventFilter ( QObject *, QEvent *);
	void		closeEvent ( QCloseEvent * e );

private slots:
	void		fileOpen();
	void		fileNew();
	void		filePrint();
	void		fileExit();
	void		viewFitPage();
	void		viewFitPageWidth();
	void		pageGoto();
	void		configKeys();
	void		optionsPreferences();
	void		toggleMakePK();
	void		toggleShowPS();
	void		toggleShowMenubar();
	void		toggleShowButtons();
	void		toggleVertToolbar();
	void		toggleShowStatusbar();
	void		toggleShowScrollbars();
	void		helpContents();
//	void		helpAbout();
//	void		helpAboutQt();
	void		pannerChanged();
	void		pageActivated(const char *);
	void		setPage(int p=0);
	void		selectLarge();
	void		selectSmall();
	void		selectShrink(QPoint);
	void		selectShrink(int);
//	void		selectResolution(const char *s);
	void		openRecent(int id);
	void		openRecent(QPoint);
	void		openFile(QString name);
	void		dropEvent( KDNDDropZone * dropZone );
	void		updateMarklist();
	void		showTip( const char *);
	void		removeTip( );
	void		showPoint( QPoint );

private:
	void		makeMenuBar();
	void		updateMenuAccel();
	void		makeButtons();
	void		makeToolBar2(QWidget *parent);
	void		makeStatusBar( QString s );
	void		bindKeys();
	void		message( const QString &s );
	void		applyMakePK();
	void		applyShowPS();
	void		applyShowMenubar();
	void		applyShowButtons();
	void		applyShowScrollbars();
	void		applyShowStatusbar();
	void		applyVertToolbar();
	QBoxLayout *	vbl;
	QBoxLayout *	hbl;
	KPanner *	kpan;
	QGridLayout *	gl;
	void		newWindow( const char *name=0 );
	void		readConfig();
	dviWindow *	dviwin;
	KMenuBar *	menuBar;
	KToolBar *	toolBar;
	KStatusBar *	statusBar;
	QFrame *	f;
	QFrame *	f2;
	QLabel *	msg;
	QLabel *	statusName;
	QComboBox *	page;
	QString		dviName;
	int		largeShrink;
	int		smallShrink;
	int		basedpi;
	QString		mfmode, paper;
	int		makepk;
	QPopupMenu *	optionsmenu;
	KConfig *	config;
	QStrList	recent;
	QPopupMenu *	recentmenu;
	QPopupMenu *	rmbmenu;
	QPopupMenu *	ssmenu;
	int		recentmax;
	ScrollBox *	sbox;
	int		hideMenubar, hideStatusbar;
	int		hideButtons, vertToolbar;
	int		hideScrollbars;
	int		showPS;
	QAccel *	accel;
	MarkList *	marklist;
	QFrame *	toolBar2;
	const char *	sndr;
	int		pannerValue;
	QToolTipGroup *	tipgroup;
	kdviprefs* 	prefs;
	KAccel *	keys;
};

#include <qgrpbox.h>

class PageDialog : public QDialog
{
	Q_OBJECT
public:
	PageDialog();
signals:
	void textEntered(const char *);
private slots:
	void go() { emit textEntered(ed.text()); accept();}
private:
	QGroupBox gb;
	QLineEdit ed;
	QPushButton ok, cancel;
};
