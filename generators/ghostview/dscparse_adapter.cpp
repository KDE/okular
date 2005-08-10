/** 
 * Copyright (C) 2001 the KGhostView authors. See file GVAUTHORS.
 * 	
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "dscparse_adapter.h"

using namespace std;

/*-- KDSCBBOX implementation -----------------------------------------------*/

KDSCBBOX::KDSCBBOX() :
    _llx( 0 ), _lly( 0 ),
    _urx( 0 ), _ury( 0 )
{}

KDSCBBOX::KDSCBBOX( const KDSCBBOX& b ) :
    _llx( b._llx ), _lly( b._lly ),
    _urx( b._urx ), _ury( b._ury )
{}

KDSCBBOX::KDSCBBOX( int llx, int lly, int urx, int ury ) :
    _llx( llx ), _lly( lly ), 
    _urx( urx ), _ury( ury ) 
{}

KDSCBBOX::KDSCBBOX( const CDSCBBOX& bbox ) :
    _llx( bbox.llx ), _lly( bbox.lly ), 
    _urx( bbox.urx ), _ury( bbox.ury ) 
{}

KDSCBBOX& KDSCBBOX::operator = ( const KDSCBBOX& b ) 
{ 
    _llx = b._llx; _lly = b._lly; _urx = b._urx; _ury = b._ury;
    return *this; 
}

bool KDSCBBOX::operator == ( const KDSCBBOX& b ) 
{ 
    return ( _llx == b._llx && _lly == b._lly 
	  && _urx == b._urx && _ury == b._ury ); 
}

bool KDSCBBOX::operator != ( const KDSCBBOX& b ) 
{ 
    return !( *this == b ); 
}

int KDSCBBOX::llx() const { return _llx; }
int KDSCBBOX::lly() const { return _lly; }
int KDSCBBOX::urx() const { return _urx; }
int KDSCBBOX::ury() const { return _ury; }

int KDSCBBOX::width()  const { return _urx - _llx; }
int KDSCBBOX::height() const { return _ury - _lly; }

QSize KDSCBBOX::size() const { return QSize( width(), height() ); }

ostream& operator << ( ostream& os, const KDSCBBOX& source )
{
    os << "{ llx: "<< source.llx() << ", lly: " << source.lly()
       <<  " urx: "<< source.urx() << ", ury: " << source.ury() << " }";
    return os;
}

/*-- KDSCError implementation ----------------------------------------------*/

KDSCError::KDSCError( Type type, Severity severity, const QCString& line,
	              unsigned int lineNumber ) :
    _type( type ),
    _severity( severity ),
    _line( line ),
    _lineNumber( lineNumber )
{}

KDSCError::Type KDSCError::type() const
{
    return _type;
}

KDSCError::Severity KDSCError::severity() const
{
    return _severity; 
}

QCString KDSCError::line() const
{
    return _line; 
}

unsigned int KDSCError::lineNumber() const
{
    return _lineNumber; 
}

/*-- KDSCOkErrorHandler implementation -------------------------------------*/

KDSCErrorHandler::Response KDSCOkErrorHandler::error( const KDSCError& err ) 
{
    cout << "KDSC: error in line " << err.lineNumber() << endl;
    cout << err.line() << endl;
    return Ok;
}

/*-- KDSC implementation ---------------------------------------------------*/

KDSC::KDSC() :
    _errorHandler( 0 ),
    _commentHandler( 0 )
{
    _cdsc = dsc_init( this );
    Q_ASSERT( _cdsc != 0 );
    _scanHandler = new KDSCScanHandler( _cdsc );
}

KDSC::~KDSC()
{
    dsc_free( _cdsc );
    delete _scanHandler;
}

QString KDSC::dsc_version() const
{
    return QString( _cdsc->dsc_version );
}

QString KDSC::dsc_copyright() const
{
    return QString( _cdsc->dsc_copyright );
}

bool KDSC::dsc() const
{
    return ( _cdsc->dsc == TRUE );
}

bool KDSC::ctrld() const
{
    return ( _cdsc->ctrld == TRUE );
}

bool KDSC::pjl() const
{
    return ( _cdsc->pjl == TRUE );
}

bool KDSC::epsf() const
{
    return ( _cdsc->epsf == TRUE );
}

bool KDSC::pdf() const
{
    return ( _cdsc->pdf == TRUE );
}

unsigned int KDSC::preview() const
{
    return _cdsc->preview;
}

unsigned int KDSC::language_level() const
{
    return _cdsc->language_level;
}

unsigned int KDSC::document_data() const
{
    return _cdsc->document_data;
}

unsigned long KDSC::begincomments() const
{
    return _cdsc->begincomments;
}

unsigned long KDSC::endcomments() const
{
    return _cdsc->endcomments;
}

unsigned long KDSC::beginpreview() const
{
    return _cdsc->beginpreview;
}

unsigned long KDSC::endpreview() const
{
    return _cdsc->endpreview;
}

unsigned long KDSC::begindefaults() const
{
    return _cdsc->begindefaults;
}

unsigned long KDSC::enddefaults() const
{
    return _cdsc->enddefaults;
}

unsigned long KDSC::beginprolog() const
{
    return _cdsc->beginprolog;
}

unsigned long KDSC::endprolog() const
{
    return _cdsc->endprolog;
}

unsigned long KDSC::beginsetup() const
{
    return _cdsc->beginsetup;
}

unsigned long KDSC::endsetup() const
{
    return _cdsc->endsetup;
}

unsigned long KDSC::begintrailer() const
{
    return _cdsc->begintrailer;
}

unsigned long KDSC::endtrailer() const
{
    return _cdsc->endtrailer;
}

CDSCPAGE* KDSC::page() const
{
    return _cdsc->page;
}

unsigned int KDSC::page_count() const
{
    return _cdsc->page_count;
}

unsigned int KDSC::page_pages() const
{
    return _cdsc->page_pages;
}

unsigned int KDSC::page_order() const
{
    return _cdsc->page_order;
}

unsigned int KDSC::page_orientation() const
{
    return _cdsc->page_orientation;
}

CDSCCTM* KDSC::viewing_orientation() const
{
    return _cdsc->viewing_orientation;
}

unsigned int KDSC::media_count() const
{
    return _cdsc->media_count;
}

CDSCMEDIA** KDSC::media() const
{
    return _cdsc->media;
}

const CDSCMEDIA* KDSC::page_media() const
{
    return _cdsc->page_media;
}

auto_ptr<KDSCBBOX> KDSC::bbox() const
{
    if( _cdsc->bbox == 0 )
	return auto_ptr<KDSCBBOX>( 0 );
    else
	return auto_ptr<KDSCBBOX>( new KDSCBBOX( *_cdsc->bbox ) );
}

auto_ptr<KDSCBBOX> KDSC::page_bbox() const
{
    if( _cdsc->page_bbox == 0 )
	return auto_ptr<KDSCBBOX>( 0 );
    else
	return auto_ptr<KDSCBBOX>( new KDSCBBOX( *_cdsc->page_bbox ) );
}

QString KDSC::dsc_title() const
{
    return QString( _cdsc->dsc_title );
}

QString KDSC::dsc_creator() const
{
    return QString( _cdsc->dsc_creator );
}

QString KDSC::dsc_date() const
{
    return QString( _cdsc->dsc_date );
}

QString KDSC::dsc_for() const
{
    return QString( _cdsc->dsc_for );
}

bool KDSC::scanData( char* buffer, unsigned int count )
{
    return _scanHandler->scanData( buffer, count );
}

int KDSC::fixup()
{
    return dsc_fixup( _cdsc );
}

KDSCErrorHandler* KDSC::errorHandler() const
{
    return _errorHandler;
}

void KDSC::setErrorHandler( KDSCErrorHandler* errorHandler )
{
    _errorHandler = errorHandler;
    if( errorHandler == 0 )
	dsc_set_error_function( _cdsc, 0 );
    else
	dsc_set_error_function( _cdsc, &errorFunction );
}

KDSCCommentHandler* KDSC::commentHandler() const
{
    return _commentHandler;
}

void KDSC::setCommentHandler( KDSCCommentHandler* commentHandler )
{
    if( _commentHandler != 0 && commentHandler == 0 )
    {
	delete _scanHandler;
	_scanHandler = new KDSCScanHandler( _cdsc );
    }
    else if( _commentHandler == 0 && commentHandler != 0 )
    {
	delete _scanHandler;
	_scanHandler = new KDSCScanHandlerByLine( _cdsc, commentHandler );
    }
    _commentHandler = commentHandler;
}

bool KDSC::isStructured() const 
{
    return epsf() ? ( page_count() > 1 ) : ( page_count() > 0 );
}

CDSC* KDSC::cdsc() const
{
    return _cdsc;
}

int KDSC::errorFunction( void* caller_data, CDSC* dsc,
	unsigned int explanation, const char* line, unsigned int line_len )
{
    KDSCError error( 
	    static_cast< KDSCError::Type >( explanation ), 
	    static_cast< KDSCError::Severity >( dsc->severity[explanation] ),
	    QCString( line, line_len + 1 ),
	    dsc->line_count
    );
    
    KDSC* kdsc = static_cast< KDSC* >( caller_data );
    Q_ASSERT( kdsc );
    
    return kdsc->errorHandler()->error( error );
}

bool KDSCScanHandlerByLine::scanData( char* buf, unsigned int count )
{
    char* lineStart = buf;
    char* it = buf;
    while( it < buf + count )
    {
	if( *it++ == '\n' )
	{
	    int retval = dsc_scan_data( _cdsc, lineStart, it - lineStart );
	    if( retval < 0 ) 
		return false;
	    else if( retval > 0 )
	    {
		_commentHandler->comment( 
			static_cast<KDSCCommentHandler::Name>( retval ) );
	    }
	    lineStart = it;
	}
    }
  
    if( it != lineStart )
    {
	// Scan the remaining part of the string.
	return ( dsc_scan_data( _cdsc, lineStart, it - lineStart ) < 0 );
    }
    else
	return true;
}

// vim:sw=4:sts=4:ts=8:noet
