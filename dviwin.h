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
	dviWindow( int basedpi, int zoom, const char *mfmode, const char *paper, int makepk,
	QWidget *parent=0, const char *name=0 );
	~dviWindow();
	int		page();
	int		totalPages();
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
        
signals:
	void		setPoint( QPoint p );
	void		statusChange( const QString & );


public slots:
	void		setFile(const char *fname);
	void		gotoPage(int page);
	void		setZoom(int zoom);
	void		drawPage();

protected:
        void            drawContents ( QPainter *p, 
                                       int clipx, int clipy, 
                                       int clipw, int cliph );

private:
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
};

#endif
