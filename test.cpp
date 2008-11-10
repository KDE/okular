#include "mobipocket.h"
#include <qfile.h>
#include <kdebug.h>

int main(int argc, char ** argv) 
{
    QFile f(argv[1]);
    f.open(QIODevice::ReadOnly);
    Mobipocket::Document* d=new Mobipocket::Document(&f);
    kDebug() << d->isValid();
    d->text();
    return 0;
}
