#ifndef _KPDF_PAGEWIDGET_H_
#define _KPDF_PAGEWIDGET_H_

#include <qpixmap.h>
#include <qwidget.h>
#include <qscrollview.h>

class LinkAction;
class PDFDoc;

class QOutputDevPixmap;

namespace KPDF
{
	/**
	 * Widget displaying a pixmap containing a PDF page and Links.
	 */
	class PageWidget : public QScrollView
	{
		Q_OBJECT

		enum ZoomMode { FitInWindow, FitWidth, FitVisible, FixedFactor };

		public:
			PageWidget(QWidget* parent = 0, const char* name = 0);
            ~PageWidget();
			void setPDFDocument(PDFDoc*);
			void setPixelsPerPoint(float);
			/* void setLinks(); */

			void setPage(int pagenum);
			int getPage() { return m_currentPage; };

		public slots:
			void nextPage();
			void previousPage();
			void zoomIn();
			void zoomOut();

			void updatePixmap();

		signals:
			void linkClicked(LinkAction*);

		protected:
			void contentsMousePressEvent(QMouseEvent*);
			void contentsMouseReleaseEvent(QMouseEvent*);
			void contentsMouseMoveEvent(QMouseEvent*);

			virtual void drawContents ( QPainter *p, int, int, int, int );

		private:

			QOutputDevPixmap * m_outputdev;
			PDFDoc* m_doc;

			float   m_ppp; // Pixels per point
			float		m_zoomFactor;
			ZoomMode m_zoomMode;

			int m_currentPage;

			LinkAction* m_pressedAction;
	};
}

#endif

// vim:ts=2:sw=2:tw=78:et
