//
// Class: dviWindow
//
// Widget for displaying TeX DVI files.
//

#ifndef _dviwin_h_
#define _dviwin_h_

#include "../config.h"
#include <qpainter.h> 
#include <qevent.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qscrollview.h> 

class dviWindow : public QScrollView
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
	void		setPoint( QPoint p );
	void		currentPage(int page);
	void		pageSizeChanged( QSize );
	void		viewSizeChanged( QSize ); 
	void		currentPosChanged( QPoint );
	void		shrinkChanged( int shrink );
	void		fileChanged();
	void		statusChange( const QString & );


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
	void            resizeEvent(QResizeEvent *e);
        void		viewportMousePressEvent ( QMouseEvent *e);
	void		viewportMouseMoveEvent ( QMouseEvent *e);
	void		keyPressEvent ( QKeyEvent *e);
        void            drawContents ( QPainter *p, 
                                       int clipx, int clipy, 
                                       int clipw, int cliph );
private slots:
	void		timerEvent();
        void            contentsMoving(int x, int y);

private:
	bool		changedDVI();
	bool		correctDVI();
	void		initDVI();
	void		changePageSize();
	QPoint		mouse;
	QString		filename;
	int		basedpi, makepk;
	QPixmap	*	pixmap;
	QTimer *	timer;
	QString		MetafontMode;
	QString		FontPath;
	QString		paper_type;
	int		ChangesPossible;
	int		checkinterval;
	QDateTime	changetime;
};

#endif
