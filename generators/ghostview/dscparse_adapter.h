/** 
 * Copyright (C) 2001 the KGhostView authors. See file AUTHORS.
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

#ifndef DSCPARSE_ADAPTER_H
#define DSCPARSE_ADAPTER_H

#include <iostream>
#include <map>
#include <memory>

#include <qsize.h>
#include <qstring.h>

#include "dscparse.h"
#undef min
#undef max

#if defined(__GNUC__)
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 93)
/*
 * We add a quick 'n' dirty inline implementation of auto_ptr for older
 * releases of GCC, which don't include an auto_ptr implementation in
 * <memory>.
 */

template <class T> class auto_ptr {
private:
	T* _ptr;

public:
	typedef T element_type;
	explicit auto_ptr(T* p = 0) : _ptr(p) {}
	auto_ptr(auto_ptr& a) : _ptr(a.release()) {}
	template <class T1> auto_ptr(auto_ptr<T1>& a) : _ptr(a.release()) {}
	auto_ptr& operator=(auto_ptr& a) {
		if (&a != this) {
			delete _ptr;
			_ptr = a.release();
		}
		return *this;
	}
	template <class T1>
	auto_ptr& operator=(auto_ptr<T1>& a) {
		if (a.get() != this->get()) {
			delete _ptr;
			_ptr = a.release();
		}
		return *this;
	}
	~auto_ptr() { delete _ptr; }

	T& operator*() const { return *_ptr; }
	T* operator->() const { return _ptr; }
	T* get() const { return _ptr; }
	T* release() { T* tmp = _ptr; _ptr = 0; return tmp; }
	void reset(T* p = 0) { delete _ptr; _ptr = p; }
};

#endif
#endif


class KDSCBBOX
{
public:
    KDSCBBOX(); 
    KDSCBBOX( const KDSCBBOX& b );
    KDSCBBOX( int llx, int lly, int urx, int ury ); 
    KDSCBBOX( const CDSCBBOX& bbox ); 

    KDSCBBOX& operator = ( const KDSCBBOX& b );  
    
    bool operator == ( const KDSCBBOX& b ); 
    bool operator != ( const KDSCBBOX& b );
    
    int llx() const;
    int lly() const;
    int urx() const;
    int ury() const;

    int width()  const;
    int height() const;

    QSize size() const;

private:
    int _llx, _lly, _urx, _ury;
};

std::ostream& operator << ( std::ostream&, const KDSCBBOX& );


class KDSCError
{
public:
    enum Type
    {
	BBox             = CDSC_MESSAGE_BBOX,
	EarlyTrailer     = CDSC_MESSAGE_EARLY_TRAILER,
	EarlyEOF         = CDSC_MESSAGE_EARLY_EOF,
	PageInTrailer    = CDSC_MESSAGE_PAGE_IN_TRAILER,
	PageOrdinal      = CDSC_MESSAGE_PAGE_ORDINAL,
	PagesWrong       = CDSC_MESSAGE_PAGES_WRONG,
	EPSNoBBox        = CDSC_MESSAGE_EPS_NO_BBOX,
	EPSPages         = CDSC_MESSAGE_EPS_PAGES,
	NoMedia          = CDSC_MESSAGE_NO_MEDIA,
	AtEnd            = CDSC_MESSAGE_ATEND,
	DuplicateComment = CDSC_MESSAGE_DUP_COMMENT,
	DuplicateTrailer = CDSC_MESSAGE_DUP_TRAILER,
	BeginEnd         = CDSC_MESSAGE_BEGIN_END,
	BadSection       = CDSC_MESSAGE_BAD_SECTION,
	LongLine         = CDSC_MESSAGE_LONG_LINE,
	IncorrectUsage   = CDSC_MESSAGE_INCORRECT_USAGE
    };

    enum Severity
    {
	Information = CDSC_ERROR_INFORM,
	Warning     = CDSC_ERROR_WARN,
	Error       = CDSC_ERROR_ERROR
    };

    KDSCError( Type, Severity, const QCString& line, 
	       unsigned int lineNumber );
    
    Type         type()       const;
    Severity     severity()   const;
    QCString     line()       const;
    unsigned int lineNumber() const;
    
private:
    Type         _type;
    Severity     _severity;
    QCString     _line;
    unsigned int _lineNumber;
};


class KDSCErrorHandler
{
public:
    virtual ~KDSCErrorHandler() {}
    enum Response 
    { 
	Ok        = CDSC_RESPONSE_OK, 
	Cancel    = CDSC_RESPONSE_CANCEL,
	IgnoreAll = CDSC_RESPONSE_IGNORE_ALL
    };
    
    virtual Response error( const KDSCError& ) = 0;
};

class KDSCOkErrorHandler : public KDSCErrorHandler
{
    Response error( const KDSCError& );
};

class KDSCCommentHandler
{
public:
    virtual ~KDSCCommentHandler() {}
    enum Name
    {
	// Header section
	PSAdobe               = CDSC_PSADOBE,
	BeginComments         = CDSC_BEGINCOMMENTS,
	EndComments           = CDSC_ENDCOMMENTS,
	Pages                 = CDSC_PAGES,
	Creator               = CDSC_CREATOR,
	CreationDate          = CDSC_CREATIONDATE,
	Title                 = CDSC_TITLE,
	For                   = CDSC_FOR,
	LanguageLevel         = CDSC_LANGUAGELEVEL,
	BoundingBox           = CDSC_BOUNDINGBOX,
	Orientation           = CDSC_ORIENTATION,
	PageOrder             = CDSC_PAGEORDER,
	DocumentMedia         = CDSC_DOCUMENTMEDIA,
	DocumentPaperSizes    = CDSC_DOCUMENTPAPERSIZES,
	DocumentPaperForms    = CDSC_DOCUMENTPAPERFORMS,
	DocumentPaperColors   = CDSC_DOCUMENTPAPERCOLORS,
	DocumentPaperWeights  = CDSC_DOCUMENTPAPERWEIGHTS,
	DocumentData          = CDSC_DOCUMENTDATA,
	Requirements          = CDSC_REQUIREMENTS,
	DocumentNeededFonts   = CDSC_DOCUMENTNEEDEDFONTS,
	DocumentSuppliedFonts = CDSC_DOCUMENTSUPPLIEDFONTS,
	HiResBoundingBox      = CDSC_HIRESBOUNDINGBOX,
	CropBox               = CDSC_CROPBOX,
	
	// Preview section
	BeginPreview          = CDSC_BEGINPREVIEW,
	EndPreview            = CDSC_ENDPREVIEW,

	// Defaults section
	BeginDefaults         = CDSC_BEGINDEFAULTS,
	EndDefaults           = CDSC_ENDDEFAULTS,
	// also %%PageMedia, %%PageOrientation, %%PageBoundingBox

	// Prolog section 
	BeginProlog           = CDSC_BEGINPROLOG,
	EndProlog             = CDSC_ENDPROLOG,
	BeginFont             = CDSC_BEGINFONT,
	EndFont               = CDSC_ENDFONT,
	BeginFeature          = CDSC_BEGINFEATURE,
	EndFeature            = CDSC_ENDFEATURE,
	BeginResource         = CDSC_BEGINRESOURCE,
	EndResource           = CDSC_ENDRESOURCE,
	BeginProcset          = CDSC_BEGINPROCSET,
	EndProcset            = CDSC_ENDPROCSET,

	// Setup section
	BeginSetup            = CDSC_BEGINSETUP,
	EndSetup              = CDSC_ENDSETUP,
	Feature               = CDSC_FEATURE,
	PaperColor            = CDSC_PAPERCOLOR,
	PaperForm             = CDSC_PAPERFORM,
	PaperWeight           = CDSC_PAPERWEIGHT,
	PaperSize             = CDSC_PAPERSIZE,
	// also %%Begin/EndFeature, %%Begin/EndResource

	// Page section
	Page                  = CDSC_PAGE,
	PageTrailer           = CDSC_PAGETRAILER,
	BeginPageSetup        = CDSC_BEGINPAGESETUP,
	EndPageSetup          = CDSC_ENDPAGESETUP,
	PageMedia             = CDSC_PAGEMEDIA,
	// also %%PaperColor, %%PaperForm, %%PaperWeight, %%PaperSize
	PageOrientation       = CDSC_PAGEORIENTATION,
	PageBoundingBox       = CDSC_PAGEBOUNDINGBOX,
	// also %%Begin/EndFont, %%Begin/EndFeature 
	// also %%Begin/EndResource, %%Begin/EndProcSet 
	IncludeFont           = CDSC_INCLUDEFONT,
	ViewingOrientation    = CDSC_VIEWINGORIENTATION,

	// Trailer section
	Trailer               = CDSC_TRAILER,
	// also %%Pages, %%BoundingBox, %%Orientation, %%PageOrder, 
	// %%DocumentMedia
	// %%Page is recognised as an error
	// also %%DocumentNeededFonts, %%DocumentSuppliedFonts

	// End of File */
	Eof                    = CDSC_EOF
    };
	
    virtual void comment( Name name ) { std::cout << name << std::endl; }
};

class KDSCScanHandler;
class KDSC
{
public:
    KDSC();
    ~KDSC();

    /*--- Adapter for CDSC ------------------------------------------------*/
    QString dsc_version() const;
    
    bool dsc()   const;
    bool ctrld() const;
    bool pjl()   const;
    bool epsf()  const;
    bool pdf()   const;
    
    unsigned int preview()        const;
    unsigned int language_level() const;
    unsigned int document_data()  const;

    unsigned long begincomments() const;
    unsigned long endcomments()   const;
    unsigned long beginpreview()  const;
    unsigned long endpreview()    const;
    unsigned long begindefaults() const;
    unsigned long enddefaults()   const;
    unsigned long beginprolog()   const;
    unsigned long endprolog()     const;
    unsigned long beginsetup()    const;
    unsigned long endsetup()      const;
    unsigned long begintrailer()  const;
    unsigned long endtrailer()    const;

    CDSCPAGE* page() const;

    unsigned int page_count()       const;
    unsigned int page_pages()       const;
    unsigned int page_order()       const;
    unsigned int page_orientation() const;

    CDSCCTM* viewing_orientation() const;
    
    unsigned int media_count()    const;
    CDSCMEDIA** media()           const;
    const CDSCMEDIA* page_media() const;

#if defined(__GNUC__) && (__GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 93))
    auto_ptr<KDSCBBOX> bbox()      const;
    auto_ptr<KDSCBBOX> page_bbox() const;
#else
    std::auto_ptr<KDSCBBOX> bbox()      const;
    std::auto_ptr<KDSCBBOX> page_bbox() const;
#endif

    // CDSCDOSEPS *doseps;

    QString dsc_title()   const;
    QString dsc_creator() const;
    QString dsc_date()    const;
    QString dsc_for()     const;

    // unsigned int max_error
   
    bool scanData( char*, unsigned int );

    /**
     * Tidy up from incorrect DSC comments.
     */
    int fixup();
    
    KDSCErrorHandler* errorHandler() const;
    void setErrorHandler( KDSCErrorHandler* );
    
    KDSCCommentHandler* commentHandler() const;
    void setCommentHandler( KDSCCommentHandler* );
    
    /*--- Extra methods for convenience -----------------------------------*/
    bool isStructured() const;

    /*--- Temporary -------------------------------------------------------*/
    CDSC* cdsc() const;

protected:
    static int errorFunction( void* caller_data, CDSC* dsc, 
                              unsigned int explanation, 
                              const char* line, unsigned int line_len );
    
private:
    CDSC*               _cdsc;
    KDSCErrorHandler*   _errorHandler;
    KDSCCommentHandler* _commentHandler;
    KDSCScanHandler*    _scanHandler;
};

class KDSCScanHandler
{
public:
    virtual ~KDSCScanHandler() {}
    KDSCScanHandler( CDSC* cdsc ) : _cdsc( cdsc ) {}
    
    virtual bool scanData( char* buf, unsigned int count )
    {
	return ( dsc_scan_data( _cdsc, buf, count ) >= 0 );
    }
    
protected:
    CDSC* _cdsc;
};

class KDSCScanHandlerByLine : public KDSCScanHandler
{
public:
    KDSCScanHandlerByLine( CDSC* cdsc, KDSCCommentHandler* commentHandler ) : 
	KDSCScanHandler( cdsc ),
	_commentHandler( commentHandler ) 
    {}
    
    virtual bool scanData( char* buf, unsigned int count );

protected:
    KDSCCommentHandler* _commentHandler;
};

#endif

// vim:sw=4:sts=4:ts=8:noet
