#include "QOutputDevPixmap.h"

#include <qcursor.h>
#include <qpainter.h>

#include "PDFDoc.h"

#include "kpdf_pagewidget.h"
#include "kpdf_pagewidget.moc"

#include "utils.h"

namespace KPDF
{
	PageWidget::PageWidget(QWidget* parent, const char* name)
		: QScrollView(parent, name, WRepaintNoErase),
	m_doc(0),
	m_zoomFactor( 1.0 ),
	m_currentPage( 1 ),
	m_pressedAction( 0 )
	{
		m_outputdev = new QOutputDevPixmap();
		setMouseTracking(true);
	}

	void
		PageWidget::setPDFDocument(PDFDoc* doc)
		{
			m_doc = doc;
			updatePixmap();
		}

	void
		PageWidget::setPixelsPerPoint(float ppp)
		{
			m_ppp = ppp;
		}

	void
		PageWidget::contentsMousePressEvent(QMouseEvent* e)
		{
			if (m_doc == 0)
				return;

			m_pressedAction = m_doc->findLink(e->x()/m_ppp, e->y()/m_ppp);
		}

	void
		PageWidget::contentsMouseReleaseEvent(QMouseEvent* e)
		{
			if (m_doc == 0)
				return;

			LinkAction* action = m_doc->findLink(e->x()/m_ppp, e->y()/m_ppp);
			if (action == m_pressedAction)
				emit linkClicked(action);

			m_pressedAction = 0;
		}

	void
		PageWidget::contentsMouseMoveEvent(QMouseEvent* e)
		{
			if (m_doc == 0)
				return;

			LinkAction* action = m_doc->findLink(e->x()/m_ppp, e->y()/m_ppp);
			setCursor(action != 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
		}

	void PageWidget::drawContents ( QPainter *p, int clipx, int clipy, int clipw, int cliph )
	{
		QPixmap * m_pixmap = NULL;
		if (m_outputdev)
			m_pixmap = m_outputdev->getPixmap();
		if ( m_pixmap != NULL && ! m_pixmap->isNull() )
			p->drawPixmap ( clipx, clipy, *m_pixmap, clipx, clipy, clipw, cliph );
		else
			p->fillRect ( clipx, clipy, clipw, cliph, white );
	}

	void PageWidget::nextPage()
	{
		setPage( getPage() + 1);
	};

	void PageWidget::previousPage()
	{
		setPage( getPage() - 1 );
	};

	void PageWidget::zoomIn()
	{
		m_zoomFactor += 0.1;
		updatePixmap();
	};

	void PageWidget::zoomOut()
	{
		m_zoomFactor -= 0.1;
		updatePixmap();
	};

	void PageWidget::setPage(int page)
	{
		if (m_doc)
		{
			m_currentPage = max(0, min( page, m_doc->getNumPages()));
		} else {
			m_currentPage = 0;
		}
		updatePixmap();
	};

	void PageWidget::updatePixmap()
	{
            if ( m_doc )
            {
		const double pageWidth  = m_doc->getPageWidth (m_currentPage) * m_zoomFactor;
		const double pageHeight = m_doc->getPageHeight(m_currentPage) * m_zoomFactor;

		// Pixels per point when the zoomFactor is 1.
		const float basePpp  = QPaintDevice::x11AppDpiX() / 72.0;

		const float ppp = basePpp * m_zoomFactor; // pixels per point

		m_doc->displayPage(m_outputdev, m_currentPage, int(ppp * 72.0), 0, true);

		resizeContents ( m_outputdev->getPixmap()->width ( ), m_outputdev->getPixmap()->height ( ));

		viewport()->update();
            }
	}
}

// vim:ts=2:sw=2:tw=78:et
