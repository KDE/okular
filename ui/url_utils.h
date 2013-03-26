#include <QRegExp>

namespace UrlUtils
{
    QString getUrl( QString txt )
    {
        // match the url
        QRegExp reg( QString( "\\b((https?|ftp)://(www\\d{0,3}[.])?[\\S]+)|((www\\d{0,3}[.])[\\S]+)" ) );
        // blocks from detecting invalid urls
        QRegExp reg1( QString( "[\\w'\"\\(\\)]+https?://|[\\w'\"\\(\\)]+ftp://|[\\w'\"\\(\\)]+www\\d{0,3}[.]" ) );
        txt = txt.remove( "\n" );
        if( reg1.indexIn( txt ) < 0 && reg.indexIn( txt ) >= 0 && QUrl( reg.cap() ).isValid() )
        {
            QString url = reg.cap();
            if( url.startsWith( "www" ) )
                url.prepend( "http://" );
            return url;
        }
        else
            return QString();
    }
};

