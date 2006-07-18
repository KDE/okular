#include <QTextStream>

#include "generator.h"

QTextStream& operator<< (QTextStream& str, const PixmapRequest *req)
{
    QString s;
    s += QString(req->async ? "As" : "S") + QString("ync PixmapRequest (id: %1) (%2x%3) ").arg(req->id,req->width,req->height);
    s += QString("prio: %1, pageNo: %2) ").arg(req->priority,req->pageNumber);
    return (str << s);
}

#include "generator.moc"
