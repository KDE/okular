#include <qstring.h>

namespace KCHMUrl
{
    bool isRemoteURL( const QString & url, QString & protocol );
    bool isJavascriptURL( const QString & url );
    bool isNewChmURL( const QString & url, QString & chmfile, QString & page );
    QString makeURLabsolute ( const QString & url );
    QString makeURLabsoluteIfNeeded ( const QString & url );
}
