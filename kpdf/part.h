#ifndef PDFPARTVIEW_H
#define PDFPARTVIEW_H

#include <qpixmap.h>
#include <qwidget.h>

class QHBoxLayout;
class QPixmap;

namespace KPDF {
class PageWidget;
}

class ThumbnailList;

class PDFPartView : public QWidget
{
    Q_OBJECT

public:
    PDFPartView(QWidget* parent, const char* name);
    ~PDFPartView();
    
    void setCurrentItem(int i);
    int getCurrentItem() const;
    
    void setPages(int i, double ar);
    void setThumbnail(int i, const QPixmap *thumbnail);
    
    void showPageList(bool show);

    // TODO make private
    KPDF::PageWidget* outputdev;

signals:
    void clicked(int);

protected:
    QHBoxLayout* PDFPartViewLayout;

private:
    ThumbnailList* pagesList;
};

#endif // PDFPARTVIEW_H
