//
// Class: kdvi
//
// Previewer for TeX DVI files.
//

//#define KDVI_VERSION "0.4.1"

#ifndef _kdvi_miniwidget_h_
#define _kdvi_miniwidget_h_

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
#include <qsplitter.h>
#include "prefs.h"


class QSplitter;
class QToolTipGroup;

class KDVIMiniWidget : public QSplitter
{
	Q_OBJECT

public:
	KDVIMiniWidget(char *fname=0, QWidget *parent=0,const char *name=0 );
	~KDVIMiniWidget();
        void		openFile(QString name);
        dviWindow*      window(){return(dviwin);}

public slots:
	void		filePrint();
	void		fileChanged();
	void		saveProperties(KConfig*);
	void		readProperties(KConfig*);

protected:
	void		resizeEvent( QResizeEvent* e );
	bool		eventFilter ( QObject *, QEvent *);
	void		closeEvent ( QCloseEvent * e );
private slots:
	void		viewFitPage();
	void		viewFitPageWidth();
	void		pageGoto();
	void		toggleMakePK();
	void		toggleShowPS();
//	void		helpAbout();
//	void		helpAboutQt();
        void            setPage(int p=0);
	void		pageActivated(const QString &);
	void		selectLarge();
	void		selectSmall();
	void		selectShrink(QPoint);
        void		selectShrink(int);
        void            updateMarkList();
//	void		selectResolution(const char *s);
signals:
        void            statusMessage(const QString &s);
private:
        void		message( const QString &s);
	void		makeToolBar2(QWidget *parent);
	void		applyPreferences();
	void		applyMakePK();
	void		applyShowPS();
	void		applyShowMenubar();
	void		applyShowButtons();
	void		applyShowScrollbars();
	void		applyShowStatusbar();
	void		applyVertToolbar();
	QBoxLayout *	vbl;
	QBoxLayout *	hbl;
	QGridLayout *	gl;
	void		readConfig();
	dviWindow *	dviwin;
	QFrame *	f;
	QFrame *	f2;
	QLabel *	msg;
	QLabel *	statusName;
	QString		dviName;
	int		largeShrink;
	int		smallShrink;
	int		basedpi;
	QString		mfmode, paper;
	int		makepk;
	KConfig *	config;
	QPopupMenu *	ssmenu;
	ScrollBox *	sbox;
	int		showPS;
	MarkList *	marklist;
	QFrame *	toolBar2;
	const char *	sndr;
	int		pannerValue;
};


#endif
