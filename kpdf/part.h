#ifndef PDFPARTVIEW_H
#define PDFPARTVIEW_H

#include <qpixmap.h>
#include <qwidget.h>

class QHBoxLayout;
class QPixmap;
class QMutex;

namespace KPDF {
class PageWidget;
}

class ThumbnailList;
class PDFDoc;

class PDFPartView : public QWidget
{
    Q_OBJECT

public:
    PDFPartView(QWidget* parent, const char* name, QMutex *docMutex);
    ~PDFPartView();
    
    void setCurrentItem(int i);
    
    void setPages(int i, double ar);
    void generateThumbnails(PDFDoc *doc);
    void stopThumbnailGeneration();
    
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
