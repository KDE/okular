#include <QTextStream>

#include "generator.h"

QTextStream& operator<< (QTextStream& str, const PixmapRequest *req)
{
    QString s("");
    if (req->async)
        s += "As";
    else
        s += "S";
    s += QString ("ync PixmapRequest (id: %1) (%2x%3) ").arg(req->id,req->width,req->height);
    s += QString("rot:%4, prio: %5, pageNo: %6) ").arg(req->rotation,req->priority,req->pageNumber);
    return (str << s);
}
