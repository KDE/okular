//
// Class: dviWindow
//
// Widget for displaying TeX DVI files.
//

#include "../config.h"
#include <qwidget.h>
#include <qpainter.h> 
#include <qevent.h>
#include <qscrbar.h>
#include <qtimer.h>
#include <qdatetm.h>

class dviWindow : public QWidget
{
	Q_OBJECT

public:
	dviWindow( int basedpi, const char *mfmode, const char *paper, int makepk,
	QWidget *parent=0, const char *name=0 );
	~dviWindow();
	int		page();
	int		totalPages();
	int		shrink();
	void		setChecking( int time = 1000 );
	int		checking();
	void		setShowScrollbars( int flag );
	int		showScrollbars();
	void		setShowPS( int flag );
	int		showPS();
	void		setAntiAlias( int flag );
	int		antiAlias();
	void		setMakePK( int flag );
	int		makePK();
	void		setResolution( int basedpi );
	int		resolution();
	void		setMetafontMode( const char * );
	const char *	metafontMode();
	void		setPaper( const char * );
	const char *	paper();
	void		setGamma( float );
	float		gamma();
	void		setFontPath( const char * );
	const char *	fontPath();
	QSize		viewSize();
	QSize		pageSize();
	QPoint		currentPos();

signals:
	void		currentPage(int page);
	void		pageSizeChanged( QSize );
	void		viewSizeChanged( QSize );
	void		currentPosChanged( QPoint );
	void		shrinkChanged( int shrink );
	void		fileChanged();
	void		statusChange( const char * );
	void		setPoint( QPoint p );

public slots:
	void		setFile(const char *fname);
	void		prevPage();
	void		nextPage();
	void		gotoPage(int page);
	void		goForward();
	void		firstPage();
	void		lastPage();
	void		prevShrink();
	void		nextShrink();
	void		setShrink(int shrink);
	void		scroll( QPoint to );
	void		drawPage();

protected:
	void		paintEvent( QPaintEvent *pe );
	void		resizeEvent ( QResizeEvent *);
	void		mousePressEvent ( QMouseEvent *e);
	void		mouseMoveEvent ( QMouseEvent *e);
	void		keyPressEvent ( QKeyEvent *e);

private slots:
	void		scrollVert( int );
	void		scrollHorz( int );
	void		timerEvent();

private:
	bool		changedDVI();
	bool		correctDVI();
	void		initDVI();
	void		drawDVI();
	void		changePageSize();
	void		setChildrenGeometries(int doupdate=TRUE);
	void		scrollRelative(const QPoint r, int doupdate=TRUE);
	void		scrollAbsolute(const QPoint r);
	QScrollBar *	hsb;
	QScrollBar *	vsb;
	QWidget		block;
	int 		hscroll, vscroll;	
	int		hclip, vclip;
	QPoint		mouse, base;
	QString		filename;
	int		basedpi, makepk;
	int		showsbs;
	QPixmap	*	pixmap;
	QTimer *	timer;
	QString		MetafontMode;
	QString		FontPath;
	QString		paper_type;
	int		ChangesPossible;
	int		checkinterval;
	QDateTime	changetime;
};
