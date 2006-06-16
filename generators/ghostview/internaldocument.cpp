/***************************************************************************
 *   Copyright (C) 1997-2005 the KGhostView authors. See file GV_AUTHORS.  *
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   Many portions of this file are based on kghostview's kpswidget code   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>
extern "C" {
#include "ps.h"
}

#include "internaldocument.h"

QStringList GSInternalDocument::paperSizes()
{
    QStringList list;
    int i=0;
    for (;i<CDSC_KNOWN_MEDIA;i++)
    {
        if ( ! dsc_known_media[i].name )
            break;
        list << QString ( dsc_known_media[i].name );
    }
    return list;
}

void GSInternalDocument::scanDSC()
{
    m_dsc = new KDSC();
    char buf[4096];
    int count=1;
    while( count != 0 )
    {
        count = fread( buf, sizeof(char), sizeof( buf ), m_internalFile );
        m_dsc->scanData( buf, count );
    }
    m_dsc->fixup();
}

GSInternalDocument::GSInternalDocument(const QString &fname, Format form) : m_error(false), m_fileName(fname), docInfo(0), m_format(form)
{
    m_internalFile = fopen(QFile::encodeName(fname),"r");
    if( m_internalFile == 0 )
    {
        m_error=true;
        m_errorString=strerror(errno);
    }

    if (!m_error)
    {
        m_fallbackMedia = pageSizeToString( static_cast< QPrinter::PageSize >( KGlobal::locale()->pageSize() ) );
        m_overrideOrientation = CDSC_ORIENT_UNKNOWN;
        m_overrideMedia = QString::null;

        scanDSC();

        if (!m_dsc)
        {
            m_error=true;
            m_errorString="Failed to construct KDSC";
        }
        if ( !m_error && ! m_dsc->dsc() )
        {
            m_error=true;
            // note this is not a fatal error, just a notice, we support docs without dsc
            m_errorString="Document has no DSC.";
        }
    }

    if (!m_error)
    {
        // available media information
        // 1. known predefined media
        const CDSCMEDIA* m = dsc_known_media;
        while( m->name )
        {
            m_mediaNames << m->name;
            m++;
        }

        // 2. media defined in the document
        if( m_dsc->media() ) 
        {
            unsigned int end=m_dsc->media_count();
            for( unsigned int i = 0; i < end ; i++ ) 
            {
                if( m_dsc->media()[i] && m_dsc->media()[i]->name )
                    m_mediaNames << m_dsc->media()[i]->name;
            }
        }
    }

    if (m_error)
        kDebug(4656) << m_errorString << endl;
}

GSInternalDocument::~GSInternalDocument()
{
    delete docInfo;
}

const DocumentInfo * GSInternalDocument::generateDocumentInfo()
{
    if (! m_dsc->dsc() )
        return 0L;

    if (!docInfo)
    {
        docInfo = new DocumentInfo();

        docInfo->set( "title", m_dsc->dsc_title(), i18n("Title") );
        docInfo->set( "author", m_dsc->dsc_for(), i18n("Author") );
        docInfo->set( "creator", m_dsc->dsc_creator(), i18n("Creator") );
        docInfo->set( "creationDate", m_dsc->dsc_date(), i18n("Created") );
        docInfo->set( "copyright", m_dsc->dsc_copyright(), i18n("Copyright") );
        QString dscVer=m_dsc->dsc_version();
        docInfo->set( "dscversion", dscVer, i18n("DSC version") );

        switch (m_format)
        {
            case PDF:
                docInfo->set( "mimeType", "application/pdf" );
                break;
            case PS:
                docInfo->set( "langlevel", QString::number(m_dsc->language_level()), i18n("Language Level") );
                if (dscVer.contains ("EPS"))
                    docInfo->set( "mimeType", "image/x-eps" );
                else
                    docInfo->set( "mimeType", "application/postscript" );
                break;
        }

        int pages=m_dsc->page_pages();
        if (!pages)
            pages = m_dsc->page_count();
        docInfo->set( "pages", QString::number(pages), i18n("Pages") );
    }
    return docInfo;
}


QString GSInternalDocument::pageSizeToString( QPrinter::PageSize pSize )
{
    switch( pSize )
    {
        case QPrinter::A3:
             return "A3";
        case QPrinter::A4:
             return "A4";
        case QPrinter::A5:
             return "A5";
        case QPrinter::B4:
             return "B4";
        case QPrinter::Ledger:
            return "Ledger";
        case QPrinter::Legal:
            return "Legal";
        case QPrinter::Letter:
            return "Letter";
        default:
            return "Unknown";
    }
}

QString GSInternalDocument::pageMedia() const
{
    if( !m_overrideMedia.isNull() )
        return m_overrideMedia;
    else if( m_dsc->page_media() != 0 )
        return QString( m_dsc->page_media()->name );
    else if( m_dsc->bbox().get() != 0 )
        return QString( "BoundingBox" );
    else
        return m_fallbackMedia;
}

QString GSInternalDocument::pageMedia( int pagenumber ) const
{
    if ( !m_dsc ) 
        return pageMedia();

    if ( unsigned( pagenumber ) >= m_dsc->page_count() ) 
        return pageMedia();

    if( !m_overrideMedia.isNull() )
        return m_overrideMedia;
    else if( m_dsc->page()[ pagenumber ].media != 0 )
        return QString( m_dsc->page()[ pagenumber ].media->name );
    else if( m_dsc->page_media() != 0 )
        return QString( m_dsc->page_media()->name );
    else if( m_dsc->bbox().get() != 0 )
        return QString( "BoundingBox" );
    else
        return m_fallbackMedia;
}

const CDSCMEDIA* GSInternalDocument::findMediaByName( const QString& mediaName ) const
{
    if( m_dsc->media() ) 
    {
        for( unsigned int i = 0; i < m_dsc->media_count(); i++ ) 
        {
            if( m_dsc->media()[i] && m_dsc->media()[i]->name
                && qstricmp( mediaName.toLocal8Bit(), m_dsc->media()[i]->name ) == 0 ) 
            {
                return m_dsc->media()[i];
            }
        }
    }
    /* It didn't match %%DocumentPaperSizes: */
    /* Try our known media */
    const CDSCMEDIA *m = dsc_known_media;
    while( m->name ) 
    {
        if( qstricmp( mediaName.toLocal8Bit(), m->name ) == 0 ) 
            return m;
        m++;
    }
    return 0;
}

QString GSInternalDocument::getPaperSize( const QString& mediaName ) const
{
    const CDSCMEDIA * r = 0;
    r=findMediaByName( mediaName );
    if (!r)
    {
        const CDSCMEDIA *m = dsc_known_media;
        while( m->name ) 
        {
            if( qstricmp( m_fallbackMedia.toLocal8Bit(), m->name ) == 0 ) 
                return QString(m->name);
            m++;
        }
        // should never happen as we have fallback
        kDebug(4656) << "UNABLE TO FIND PAPER SIZE FOR MEDIA NAME: " << mediaName << endl;
        return QString("a4");
    }
    return QString(r->name);
}

QSize GSInternalDocument::computePageSize( const QString& mediaName ) const
{
    if( mediaName == "BoundingBox" ) 
    {
        if( m_dsc->bbox().get() != 0 )
            return m_dsc->bbox()->size();
        else
            return QSize( 0, 0 );
    }

    const CDSCMEDIA* m = findMediaByName( mediaName );
    Q_ASSERT( m );
    return QSize( static_cast<int>( m->width ), static_cast<int>( m->height ) );
}

CDSC_ORIENTATION_ENUM GSInternalDocument::orientation() const
{
    if( m_overrideOrientation != CDSC_ORIENT_UNKNOWN )
        return m_overrideOrientation;
    else if( m_dsc->page_orientation() != CDSC_ORIENT_UNKNOWN )
        return static_cast< CDSC_ORIENTATION_ENUM >( m_dsc->page_orientation());
    else if( m_dsc->bbox().get() != 0 
            && m_dsc->bbox()->width() > m_dsc->bbox()->height() )
                return CDSC_LANDSCAPE;
    else
        return CDSC_PORTRAIT;
}

CDSC_ORIENTATION_ENUM GSInternalDocument::orientation( int pagenumber ) const
{
    if ( !m_dsc ||  unsigned( pagenumber ) >= m_dsc->page_count() ) 
    {
        return orientation();
    }
    if( m_overrideOrientation != CDSC_ORIENT_UNKNOWN ) 
    {
        return m_overrideOrientation;
    }

    if( m_dsc->page()[ pagenumber ].orientation != CDSC_ORIENT_UNKNOWN ) 
    {
        return static_cast< CDSC_ORIENTATION_ENUM >( m_dsc->page()[ pagenumber ].orientation );
    }
    if( m_dsc->page_orientation() != CDSC_ORIENT_UNKNOWN ) 
    {
        return static_cast< CDSC_ORIENTATION_ENUM >( m_dsc->page_orientation());
    }
    if( !m_dsc->epsf() ) 
    {
        return CDSC_PORTRAIT;
    }
    if( m_dsc->bbox().get() != 0
          && m_dsc->bbox()->width() > m_dsc->bbox()->height() ) {
        return CDSC_LANDSCAPE;
    }
    return CDSC_PORTRAIT;
}

KDSCBBOX GSInternalDocument::boundingBox() const
{
    QString currentMedia = pageMedia();
    if( currentMedia == "BoundingBox" )
        return KDSCBBOX( * (m_dsc->bbox().get()) );
    else {
        QSize size = computePageSize( currentMedia );
        return KDSCBBOX( 0, 0, size.width(), size.height() );
    }
}

KDSCBBOX GSInternalDocument::boundingBox( int pageNo ) const
{
    QString currentMedia = pageMedia( pageNo );
    if( currentMedia == "BoundingBox" )
        return KDSCBBOX( *(m_dsc->bbox().get()) );
    else
    {
        QSize size = computePageSize( currentMedia );
        return KDSCBBOX( 0, 0, size.width(), size.height() );
    }
}

bool GSInternalDocument::savePages( const QString& saveFileName,
                               const PageList& pageList )
{
    if( pageList.empty() )
	return true;
    
/*    if( _format == PDF ) 
    {
	KTempFile psSaveFile( QString::null, ".ps" );
	psSaveFile.setAutoDelete( true );
	if( psSaveFile.status() != 0 )
	    return false;

	// Find the minimum and maximum pagenumber in pageList.
	int minPage = pageList.first(), maxPage = pageList.first();
	for( PageList::const_iterator ci = pageList.begin();
	     ci != pageList.end(); ++ci )
	{
	    minPage = qMin( *ci, minPage );
	    maxPage = qMax( *ci, maxPage );
	}
	

	// TODO: Optimize "print whole document" case
	//
	// The convertion below from PDF to PS creates a temporary file which, in
	// the case where we are printing the whole document will then be
	// completelly copied to another temporary file.
	//
	// In very large files, the inefficiency is visible (measured in
	// seconds).
	//
	// luis_pedro 4 Jun 2003
	
	
	
	// Convert the pages in the range [minPage,maxPage] from PDF to
	// PostScript.
	if( !convertFromPDF( psSaveFile.name(), 
	                     minPage, maxPage ) )
	    return false;

	// The page minPage in the original file becomes page 1 in the 
	// converted file. We still have to select the desired pages from
	// this file, so we need to take into account that difference.
	PageList normedPageList;
	transform( pageList.begin(), pageList.end(),
	           back_inserter( normedPageList ),
	           bind2nd( minus<int>(), minPage - 1 ) );
	
	// Finally select the desired pages from the converted file.
	psCopyDoc( psSaveFile.name(), saveFileName, normedPageList );
    }
    else
    {*/
	psCopyDoc( m_fileName, saveFileName, pageList );
    //}

    return true;
}

// length calculates string length at compile time
// can only be used with character constants
#define length( a ) ( sizeof( a ) - 1 )

// Copy the headers, marked pages, and trailer to fp

bool GSInternalDocument::psCopyDoc( const QString& inputFile,
	const QString& outputFile, const PageList& pageList )
{
    FILE* from;
    FILE* to;
    char text[ PSLINELENGTH ];
    char* comment;
    bool pages_written = false;
    bool pages_atend = false;
    unsigned int i = 0;
    unsigned int pages = 0;
    long here;

    kDebug(4656) << "Copying pages from " << inputFile << " to "
    << outputFile << endl;

    from = fopen( QFile::encodeName( inputFile ), "r" );
    to = fopen( QFile::encodeName( outputFile ), "w" );

    pages = pageList.size();

    if( pages == 0 ) 
    {
// FIXME ERROR HANDLING
/*        KMessageBox::sorry( 0,
            i18n( "Printing failed because the list of "
                "pages to be printed was empty." ),
            i18n( "Error Printing" ) );*/
        return false;
    }

    // Hack in order to make printing of PDF files work. FIXME
    CDSC* dsc;

    if( m_format == PS )
	   dsc = m_dsc->cdsc();
/*    else {
	FILE* fp = fopen( QFile::encodeName( inputFile ), "r");
	char buf[256];
	int count;
	dsc = dsc_init( 0 );
	while( ( count = fread( buf, 1, sizeof( buf ), fp ) ) != 0 )
	    dsc_scan_data( dsc, buf, count );
	dsc_fixup( dsc );
	fclose( fp );

	if( !dsc )
	    return false;
    }*/

    here = dsc->begincomments;
    while( ( comment = pscopyuntil( from, to, here,
            dsc->endcomments, "%%Pages:" ) ) ) 
    {
        here = ftell( from );
        if( pages_written || pages_atend ) 
        {
            free( comment );
            continue;
        }
        sscanf( comment + length("%%Pages:" ), "%256s", text );
        text[256] = 0; // Just in case of an overflow
        if( strcmp( text, "(atend)" ) == 0 ) 
        {
            fputs( comment, to );
            pages_atend = true;
        }
        else 
        {
            switch ( sscanf( comment + length( "%%Pages:" ), "%*d %u", &i ) )  
            {
                case 1:
                fprintf( to, "%%%%Pages: %d %d\n", pages, i );
                break;
                default:
                fprintf( to, "%%%%Pages: %d\n", pages );
                break;
            }
            pages_written = true;
        }
        free(comment);
    }
    pscopy( from, to, dsc->beginpreview, dsc->endpreview );
    pscopy( from, to, dsc->begindefaults, dsc->enddefaults );
    pscopy( from, to, dsc->beginprolog, dsc->endprolog );
    pscopy( from, to, dsc->beginsetup, dsc->endsetup );

    //TODO -- Check that a all dsc attributes are copied

    unsigned int count = 1;
    PageList::const_iterator it;
    for( it = pageList.begin(); it != pageList.end(); ++it ) 
    {
        i = (*it) - 1;
        comment = pscopyuntil( from, to, dsc->page[i].begin,
                    dsc->page[i].end, "%%Page:" );
        if ( comment )
            free( comment );
        fprintf( to, "%%%%Page: %s %d\n", dsc->page[i].label,
            count++ );
        pscopy( from, to, -1, dsc->page[i].end );
    }

    here = dsc->begintrailer;
    while( ( comment = pscopyuntil( from, to, here,
                    dsc->endtrailer, "%%Pages:" ) ) ) 
    {
        here = ftell( from );
        if ( pages_written ) 
        {
            free( comment );
            continue;
        }
        switch ( sscanf( comment + length( "%%Pages:" ), "%*d %u", &i ) ) 
        {
            case 1:
                fprintf( to, "%%%%Pages: %d %d\n", pages, i );
                break;
            default:
                fprintf( to, "%%%%Pages: %d\n", pages );
                break;
        }
        pages_written = true;
        free( comment );
    }

    fclose( from );
    fclose( to );

/*    if( format == PDF )
	dsc_free( dsc );*/

    return true;
}

#undef length
