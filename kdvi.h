//
// Class: kdvi
//
// Previewer for TeX DVI files.
//

//#define KDVI_VERSION "0.4.1"

#ifndef _kdvi_h_
#define _kdvi_h_

#include <qframe.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qpopupmenu.h>
#include "dviwin.h"
#include <qapplication.h>
#include <kapp.h>
#include "scrbox.h"
#include "marklist.h"
#include <qaccel.h>
#include <qlayout.h>
#include <qdialog.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include "prefs.h"

#include <ktopwidget.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <kkeydialog.h>
#include <kaccel.h>

class QSplitter;
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
	void		dropEvent( QDropEvent * dropZone );

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
	void		pageActivated(const QString &);
	void		setPage(int p=0);
	void		selectLarge();
	void		selectSmall();
	void		selectShrink(QPoint);
	void		selectShrink(int);
//	void		selectResolution(const char *s);
	void		openRecent(int id);
	void		openRecent(QPoint);
	void		openFile(QString name);
	void		updateMarklist();
	void		showTip( const QString &);
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
	QSplitter *	kpan;
	QGridLayout *	gl;
	void		newWindow( const char *name=0 );
	void		readConfig();
	dviWindow *	dviwin;
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

#include <qgroupbox.h>

class PageDialog : public QDialog
{
	Q_OBJECT
public:
	PageDialog();
signals:
	void textEntered(const QString &);
private slots:
	void go() { emit textEntered(ed.text()); accept();}
private:
	QGroupBox gb;
	QLineEdit ed;
	QPushButton ok, cancel;
};

#endif
