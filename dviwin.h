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
	dviWindow( int basedpi, double zoom, const char *mfmode, int makepk,
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
	void		setPaper(double w, double h);
	void		setFontPath( const char * );
	const char *	fontPath();

	// for the preview
 	QPixmap		*pix() { return pixmap; };
        
public slots:
	void		setFile(const char *fname);
	void		gotoPage(int page);
	//	void		setZoom(int zoom);

	void		setZoom(double zoom);
	double          zoom() { return _zoom; };

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
	double          _zoom;

};


#include <X11/Xlib.h>
//#include <X11/Intrinsic.h>

struct	WindowRec {
  Window	win;
  double	shrinkfactor;
  int		base_x;
  int           base_y;
  unsigned int	width;
  unsigned int	height;
  int	        min_x;	/* for pending expose events */
  int	        max_x;	/* for pending expose events */
  int	        min_y;	/* for pending expose events */
  int	        max_y;	/* for pending expose events */
};


struct framedata {
  long dvi_h;
  long dvi_v;
  long w;
  long x;
  long y;
  long z;
  int pxl_v;
};


struct frame {
  struct framedata data;
  struct frame *next, *prev;
};



typedef	void	(*set_char_proc)(unsigned int, unsigned int);

#include "font.h"

struct drawinf {	/* this information is saved when using virtual fonts */
  struct framedata data;
  struct font	*fontp;
  set_char_proc	set_char_p;
  int		tn_table_len;
  struct font	**tn_table;
  struct tn	*tn_head;
  unsigned char		*pos, *end;
  struct font	*_virtual;
  int		dir;
};

#endif
