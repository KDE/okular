#include <kchmurl.h>
#include <qdir.h>
#include <qregexp.h>

namespace KCHMUrl
{
    bool isRemoteURL( const QString & url, QString & protocol )
    {
        // Check whether the URL is external
        QRegExp uriregex ( "^(\\w+):\\/\\/" );

        if ( uriregex.indexIn ( url ) != -1 )
        {
            QString proto = uriregex.cap ( 1 ).toLower();

            // Filter the URLs which need to be opened by a browser
            if ( proto == "http" 
            || proto == "ftp"
            || proto == "mailto"
            || proto == "news" )
            {
                protocol = proto;
                return true;
            }
        }
        return false;
    }
    bool isJavascriptURL( const QString & url )
    {
        return url.startsWith ("javascript://");
    }

    bool isNewChmURL( const QString & url, QString & chmfile, QString & page )
    {
        QRegExp uriregex ( "^ms-its:(.*)::(.*)$" );

        if ( uriregex.indexIn ( url ) != -1 )
        {
            chmfile = uriregex.cap ( 1 );
            page = uriregex.cap ( 2 );

            return true;
        }

        return false;
    }

    QString makeURLabsolute ( const QString & url )
    {
        QString p1, p2, newurl = url;

        if ( !isRemoteURL (url, p1)
        && !isJavascriptURL (url)
        && !isNewChmURL (url, p1, p2) )
        {
            newurl = QDir::cleanPath (url);

            // Normalize url, so it becomes absolute
            if ( newurl[0] != '/' )
                newurl = "/" + newurl;

            newurl = QDir::cleanPath (newurl);
        }

    //qDebug ("KCHMViewWindow::makeURLabsolute (%s) -> (%s)", url.ascii(), newurl.ascii());
        return newurl;
    }


    QString makeURLabsoluteIfNeeded ( const QString & url )
    {
        QString p1, p2, newurl = url;

        if ( !isRemoteURL (url, p1)
        && !isJavascriptURL (url)
        && !isNewChmURL (url, p1, p2) )
        {
            newurl = QDir::cleanPath (url);

            // Normalize url, so it becomes absolute
            if ( newurl[0] != '/' )
                newurl = "/" + newurl;
        }

    //qDebug ("KCHMViewWindow::makeURLabsolute (%s) -> (%s)", url.ascii(), newurl.ascii());
        return newurl;
    }

}
