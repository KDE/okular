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
#include <qwidget.h> 


class dviWindow : public QWidget
{
	Q_OBJECT

public:
	dviWindow( int basedpi, int zoom, const char *mfmode, const char *paper, int makepk,
	QWidget *parent=0, const char *name=0 );
	~dviWindow();

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
        
public slots:
	void		setFile(const char *fname);
	void		gotoPage(int page);
	void		setZoom(int zoom);
	void		drawPage();

protected:
	void paintEvent(QPaintEvent *ev);


private:
	bool		correctDVI();
	void		initDVI();
	void		changePageSize();
	QString		filename;
	int		basedpi, makepk;
	QPixmap	*	pixmap;
	QString		MetafontMode;
	QString		FontPath;
	QString		paper_type;
	int		ChangesPossible;
};

#endif
