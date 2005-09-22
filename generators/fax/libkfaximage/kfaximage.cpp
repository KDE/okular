/* 
   This file is part of KDE FAX image library
   Copyright (c) 2005 Helge Deller <deller@kde.org>

   based on Frank D. Cringle's viewfax package
   Copyright (C) 1990, 1995  Frank D. Cringle.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _LIBKFAXIMG_H_
#define _LIBKFAXIMG_H_

#include <stdlib.h>

#include <qimage.h>
#include <qfile.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>

#include "faxexpand.h"
#include "kfaximage.h"

static const char FAXMAGIC[]   = "\000PC Research, Inc\000\000\000\000\000\000";
static const char littleTIFF[] = "\x49\x49\x2a\x00";
static const char bigTIFF[]    = "\x4d\x4d\x00\x2a";

KFaxImage::KFaxImage( const QString &filename, QObject *parent, const char *name )
   : QObject(parent,name)
{
  KGlobal::locale()->insertCatalogue( QString::fromLatin1("libkfaximage") );
  loadImage(filename);
}

KFaxImage::~KFaxImage()
{ }

bool KFaxImage::loadImage( const QString &filename )
{
  reset();

  m_filename = filename;
  m_errorString = QString::null;

  if (m_filename.isEmpty())
	return false;

  int ok = notetiff();
  if (!ok) {
	reset();
  }
  return ok == 1;
}

void KFaxImage::reset()
{
  fax_init_tables();
  m_pagenodes.setAutoDelete(true);
  m_pagenodes.clear();
  // do not reset m_errorString and m_filename, since 
  // they may be needed by calling application 
}

QImage KFaxImage::page( unsigned int pageNr )
{
  if (pageNr >= numPages()) {
    kdDebug() << "KFaxImage::page() called with invalid page number\n";
    return QImage();
  }
  pagenode *pn = m_pagenodes.at(pageNr);
  GetImage(pn);
  return pn->image;
}

QPoint KFaxImage::page_dpi( unsigned int pageNr )
{
  if (pageNr >= numPages()) {
    kdDebug() << "KFaxImage::page_dpi() called with invalid page number\n";
    return QPoint(0,0);
  }
  pagenode *pn = m_pagenodes.at(pageNr);
  GetImage(pn);
  return pn->dpi;
}

QSize KFaxImage::page_size( unsigned int pageNr )
{
  if (pageNr >= numPages()) {
    kdDebug() << "KFaxImage::page_size() called with invalid page number\n";
    return QSize(0,0);
  }
  pagenode *pn = m_pagenodes.at(pageNr);
  GetImage(pn);
  return pn->size;
}


pagenode *KFaxImage::AppendImageNode(int type)
{
    pagenode *pn = new pagenode();
    if (pn) {
	pn->type = type;
        pn->expander = g31expand;
	pn->strips = NULL;
	pn->size = QSize(1728,2339);
	pn->vres = -1;
	pn->dpi = KFAX_DPI_FINE;
	m_pagenodes.append(pn);
    }
    return pn;
}

bool KFaxImage::NewImage(pagenode *pn, int w, int h)
{
    pn->image = QImage( w, h, 1, 2, QImage::systemByteOrder() );
    pn->image.setColor(0, qRgb(255,255,255));
    pn->image.setColor(1, qRgb(0,0,0));
    pn->data = (Q_UINT16*) pn->image.bits();
    pn->bytes_per_line = pn->image.bytesPerLine();
    pn->dpi = KFAX_DPI_FINE;

    return !pn->image.isNull();
}

void KFaxImage::FreeImage(pagenode *pn)
{
    pn->image = QImage();
    pn->data = NULL;
    pn->bytes_per_line = 0;
}

void KFaxImage::kfaxerror(const QString& error)
{
    m_errorString = error;
    kdError() << "kfaxerror: " << error << endl;
}


/* Enter an argument in the linked list of pages */
pagenode *
KFaxImage::notefile(int type)
{
    pagenode *newnode = new pagenode();
    newnode->type = type;
    newnode->size = QSize(1728,0);
    return newnode;
}

static t32bits
get4(unsigned char *p, QImage::Endian endian)
{
    return (endian == QImage::BigEndian) 
	? (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3] :
	   p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24);
}

static int
get2(unsigned char *p, QImage::Endian endian)
{
    return (endian == QImage::BigEndian) ? (p[0]<<8)|p[1] : p[0]|(p[1]<<8);
}

/* generate pagenodes for the images in a tiff file */
int
KFaxImage::notetiff()
{
#define SC(x) (char *)(x)
    unsigned char header[8];
    QImage::Endian endian;
    t32bits IFDoff;
    pagenode *pn = NULL;
    QString str;

    QFile file(filename());
    if (!file.open(IO_ReadOnly)) {
	kfaxerror(i18n("Unable to open file for reading."));
	return 0;
    }

    if (file.readBlock(SC(header), 8) != 8) {
	kfaxerror(i18n("Unable to read file header (file too short)."));
	return 0;
    }
    if (memcmp(header, &littleTIFF, 4) == 0)
	endian = QImage::LittleEndian;
    else if (memcmp(header, &bigTIFF, 4) == 0)
	endian = QImage::BigEndian;
    else {
 maybe_RAW_FAX:
        kfaxerror(i18n("This is not a TIFF FAX file."));
        // AppendImageNode(FAX_RAW);
	return 0;
    }
    IFDoff = get4(header+4, endian);
    if (IFDoff & 1) {
	goto maybe_RAW_FAX;
    }
    do {			/* for each page */
	unsigned char buf[8];
	unsigned char *dir = NULL , *dp = NULL;
	int ndirent;
	pixnum iwidth = 1728;
	pixnum iheight = 2339;
	int inverse = false;
	int lsbfirst = 0;
	int t4opt = 0, comp = 0;
	int orient = TURN_NONE;
	int yres = 196; /* 98.0 */
	struct strip *strips = NULL;
	unsigned int rowsperstrip = 0;
	t32bits nstrips = 1;

	if (!file.at(IFDoff)) {
	realbad:
	  kfaxerror( i18n("Invalid or incomplete TIFF file.") );
	bad:
	    if (strips)
		free(strips);
	    if (dir)
		free(dir);
	    file.close();
	    return 1;
	}
	if (file.readBlock(SC(buf), 2) != 2)
	    goto realbad;
	ndirent = get2(buf, endian);
        int len = 12*ndirent+4;
	dir = (unsigned char *) malloc(len);
	if (file.readBlock(SC(dir), len) != len)
	    goto realbad;
	for (dp = dir; ndirent; ndirent--, dp += 12) {
	    /* for each directory entry */
	    int tag, ftype;
	    t32bits count, value = 0;
	    tag = get2(dp, endian);
	    ftype = get2(dp+2, endian);
	    count = get4(dp+4, endian);
	    switch(ftype) {	/* value is offset to list if count*size > 4 */
	    case 3:		/* short */
		value = get2(dp+8, endian);
		break;
	    case 4:		/* long */
		value = get4(dp+8, endian);
		break;
	    case 5:		/* offset to rational */
		value = get4(dp+8, endian);
		break;
	    }
	    switch(tag) {
	    case 256:		/* ImageWidth */
		iwidth = value;
		break;
	    case 257:		/* ImageLength */
		iheight = value;
		break;
	    case 259:		/* Compression */
		comp = value;
		break;
	    case 262:		/* PhotometricInterpretation */
		inverse ^= (value == 1);
		break;
	    case 266:		/* FillOrder */
		lsbfirst = (value == 2);
		break;
	    case 273:		/* StripOffsets */
		nstrips = count;
		strips = (struct strip *) malloc(count * sizeof *strips);
		if (count == 1 || (count == 2 && ftype == 3)) {
		    strips[0].offset = value;
		    if (count == 2)
			strips[1].offset = get2(dp+10, endian);
		    break;
		}
		if (!file.at(value))
		    goto realbad;
		for (count = 0; count < nstrips; count++) {
		    if (file.readBlock(SC(buf), (ftype == 3) ? 2 : 4) <= 0)
			goto realbad;
		    strips[count].offset = (ftype == 3) ?
			get2(buf, endian) : get4(buf, endian);
		}
		break;
	    case 274:		/* Orientation */
		switch(value) {
		default:	/* row0 at top,    col0 at left   */
		    orient = 0;
		    break;
		case 2:		/* row0 at top,    col0 at right  */
		    orient = TURN_M;
		    break;
		case 3:		/* row0 at bottom, col0 at right  */
		    orient = TURN_U;
		    break;
		case 4:		/* row0 at bottom, col0 at left   */
		    orient = TURN_U|TURN_M;
		    break;
		case 5:		/* row0 at left,   col0 at top    */
		    orient = TURN_M|TURN_L;
		    break;
		case 6:		/* row0 at right,  col0 at top    */
		    orient = TURN_U|TURN_L;
		    break;
		case 7:		/* row0 at right,  col0 at bottom */
		    orient = TURN_U|TURN_M|TURN_L;
		    break;
		case 8:		/* row0 at left,   col0 at bottom */
		    orient = TURN_L;
		    break;
		}
		break;
	    case 278:		/* RowsPerStrip */
		rowsperstrip = value;	
		break;
	    case 279:		/* StripByteCounts */
		if (count != nstrips) {
		  str = i18n("In file %1\nStripsPerImage tag 273=%2,tag279=%3\n")
			      .arg(filename()).arg(nstrips).arg(count);
		  kfaxerror(str);
		  goto realbad;
		}
		if (count == 1 || (count == 2 && ftype == 3)) {
		    strips[0].size = value;
		    if (count == 2)
			strips[1].size = get2(dp+10, endian);
		    break;
		}
		if (!file.at(value))
		    goto realbad;
		for (count = 0; count < nstrips; count++) {
		    if (file.readBlock(SC(buf), (ftype == 3) ? 2 : 4) <= 0)
			goto realbad;
		    strips[count].size = (ftype == 3) ?
			get2(buf, endian) : get4(buf, endian);
		}
		break;
	    case 283:		/* YResolution */
		if (!file.at(value) ||
		    file.readBlock(SC(buf), 8) != 8)
		    goto realbad;
		yres = get4(buf, endian) / get4(buf+4, endian);
		break;
	    case 292:		/* T4Options */
		t4opt = value;
		break;
	    case 293:		/* T6Options */
		/* later */
		break;
	    case 296:		/* ResolutionUnit */
		if (value == 3)
		    yres = (yres * 254) / 100;  /* *2.54 */
		break;
	    }
	}
	IFDoff = get4(dp, endian);
	free(dir);
	dir = NULL;
	if (comp == 5) {
          // compression type 5 is LZW compression // XXX
	  kfaxerror(i18n("Due to patent reasons LZW (Lempel-Ziv & Welch) "
		    "compressed Fax files cannot be loaded yet.\n"));
	    goto bad;
	}
	if (comp < 2 || comp > 4) {
	  kfaxerror(i18n("This version can only handle Fax files\n"));
	    goto bad;
	}
	pn = AppendImageNode(FAX_TIFF);
	pn->nstrips = nstrips;
	pn->rowsperstrip = nstrips > 1 ? rowsperstrip : iheight;
	pn->strips = strips;
	pn->size = QSize(iwidth,iheight);
	pn->inverse = inverse;
	pn->lsbfirst = lsbfirst;
	pn->orient = orient;
	pn->dpi.setY(yres);
	pn->vres = (yres > 150) ? 1:0; /* arbitrary threshold for fine resolution */
	if (comp == 2)
	    pn->expander = MHexpand;
	else if (comp == 3)
	    pn->expander = (t4opt & 1) ? g32expand : g31expand;
	else
	    pn->expander = g4expand;
    } while (IFDoff);
    file.close();
    return 1;
#undef UC
}

/* report error and remove bad file from the list */
void
KFaxImage::badfile(pagenode *pn)
{
  kfaxerror(i18n("%1: Bad Fax File").arg(filename()));
  FreeImage(pn);
}

/* rearrange input bits into t16bits lsb-first chunks */
static void
normalize(pagenode *pn, int revbits, int swapbytes, size_t length)
{
    t32bits *p = (t32bits *) pn->data;

    kdDebug() << "normalize = " << ((revbits<<1)|swapbytes) << endl;
    switch ((revbits<<1)|swapbytes) {
    case 0:
	break;
    case 1:
	for ( ; length; length -= 4) {
	    t32bits t = *p;
	    *p++ = ((t & 0xff00ff00) >> 8) | ((t & 0x00ff00ff) << 8);
	}
	break;
    case 2:
	for ( ; length; length -= 4) {
	    t32bits t = *p;
	    t = ((t & 0xf0f0f0f0) >> 4) | ((t & 0x0f0f0f0f) << 4);
	    t = ((t & 0xcccccccc) >> 2) | ((t & 0x33333333) << 2);
	    *p++ = ((t & 0xaaaaaaaa) >> 1) | ((t & 0x55555555) << 1);
	}
	break;
    case 3:
	for ( ; length; length -= 4) {
	    t32bits t = *p;
	    t = ((t & 0xff00ff00) >> 8) | ((t & 0x00ff00ff) << 8);
	    t = ((t & 0xf0f0f0f0) >> 4) | ((t & 0x0f0f0f0f) << 4);
	    t = ((t & 0xcccccccc) >> 2) | ((t & 0x33333333) << 2);
	    *p++ = ((t & 0xaaaaaaaa) >> 1) | ((t & 0x55555555) << 1);
	}
    }
}


/* get compressed data into memory */
unsigned char *
KFaxImage::getstrip(pagenode *pn, int strip)
{
    size_t offset, roundup;
    unsigned char *Data;

    union { t16bits s; unsigned char b[2]; } so;
#define ShortOrder	so.b[1]
    so.s = 1;  /* XXX */

    QFile file(filename());
    if (!file.open(IO_ReadOnly)) {
	badfile(pn);
	return NULL;
    }

    if (pn->strips == NULL) {
	offset = 0;
	pn->length = file.size();
    }
    else if (strip < pn->nstrips) {
	offset = pn->strips[strip].offset;
	pn->length = pn->strips[strip].size;
    }
    else {
      kfaxerror( i18n("Trying to expand too many strips.") );
      return NULL;
    }

    /* round size to full boundary plus t32bits */
    roundup = (pn->length + 7) & ~3;

    Data = (unsigned char *) malloc(roundup);
    /* clear the last 2 t32bits, to force the expander to terminate
       even if the file ends in the middle of a fax line  */
    *((t32bits *) Data + roundup/4 - 2) = 0;
    *((t32bits *) Data + roundup/4 - 1) = 0;

    /* we expect to get it in one gulp... */
    if (!file.at(offset) ||
	(size_t) file.readBlock((char *)Data, pn->length) != pn->length) { 
	badfile(pn);
	free(Data);
	return NULL;
    }
    file.close();

    pn->data = (t16bits *) Data;
    if (pn->strips == NULL && memcmp(Data, FAXMAGIC, sizeof(FAXMAGIC)-1) == 0) {
	/* handle ghostscript / PC Research fax file */
      if (Data[24] != 1 || Data[25] != 0){
	kfaxerror( i18n("Only the first page of the PC Research multipage file will be shown.") );
      }
	pn->length -= 64;
	pn->vres = Data[29];
	pn->data += 32;
	roundup -= 64;
    }

    normalize(pn, !pn->lsbfirst, ShortOrder, roundup);
    if (pn->size.height() == 0)
	pn->size.setHeight( G3count(pn, pn->expander == g32expand) );
    if (pn->size.height() == 0) {

      kfaxerror( i18n("No fax found in file.") );
      badfile(pn);
      free(Data);
      return NULL;
    }
    if (pn->strips == NULL)
	pn->rowsperstrip = pn->size.height();
    return Data;
}


static void
drawline(pixnum *run, int LineNum, pagenode *pn)
{
    t32bits *p, *p1;		/* p - current line, p1 - low-res duplicate */
    pixnum *r;			/* pointer to run-lengths */
    t32bits pix;		/* current pixel value */
    t32bits acc;		/* pixel accumulator */
    int nacc;			/* number of valid bits in acc */
    int tot;			/* total pixels in line */
    int n;

    LineNum += pn->stripnum * pn->rowsperstrip;
    if (LineNum >= pn->size.height()) {
       if (LineNum == pn->size.height())
           kdError() << "Height exceeded\n";
       return;
    }
    
    p = (t32bits *) pn->image.scanLine(LineNum*(2-pn->vres));
    p1 =(t32bits *)( pn->vres ? 0 : pn->image.scanLine(1+LineNum*(2-pn->vres)));
    r = run;
    acc = 0;
    nacc = 0;
    pix = pn->inverse ? ~0 : 0;
    tot = 0;
    while (tot < pn->size.width()) {
	n = *r++;
	tot += n;
        /* Watch out for buffer overruns, e.g. when n == 65535.  */
        if (tot > pn->size.width())
            break;
	if (pix)
	    acc |= (~(t32bits)0 >> nacc);
	else if (nacc)
	    acc &= (~(t32bits)0 << (32 - nacc));
	else
	    acc = 0;
	if (nacc + n < 32) {
	    nacc += n;
	    pix = ~pix;
	    continue;
	}
	*p++ = acc;
	if (p1)
	    *p1++ = acc;
	n -= 32 - nacc;
	while (n >= 32) {
	    n -= 32;
	    *p++ = pix;
	    if (p1)
		*p1++ = pix;
	}
	acc = pix;
	nacc = n;
	pix = ~pix;
    }
    if (nacc) {
	*p++ = acc;
	if (p1)
	    *p1++ = acc;
    }
}

int
KFaxImage::GetPartImage(pagenode *pn, int n)
{
    unsigned char *Data = getstrip(pn, n);
    if (Data == 0)
	return 3;
    pn->stripnum = n;
    (*pn->expander)(pn, drawline);
    free(Data);
    return 1;
}

int 
KFaxImage::GetImage(pagenode *pn)
{
    if (!pn->image.isNull())
	return 1;

    int i;
    if (pn->strips == 0) {

        kdDebug() << "Loading RAW fax file " << m_filename << " size=" << pn->size << endl;

	/* raw file; maybe we don't have the height yet */
	unsigned char *Data = getstrip(pn, 0);
	if (Data == 0){
	  return 0;
	}
	if (!NewImage(pn, pn->size.width(), (pn->vres ? 1:2) * pn->size.height()))
	  return 0;

	(*pn->expander)(pn, drawline);
    }
    else {
	/* multi-strip tiff */
        kdDebug() << "Loading MULTI STRIP TIFF fax file " << m_filename << endl;

	if (!NewImage(pn, pn->size.width(), (pn->vres ? 1:2) * pn->size.height()))
	  return 0;
	pn->stripnum = 0;
	kdDebug() << "has " << pn->nstrips << " strips.\n";
	for (i = 0; i < pn->nstrips; i++){

	  int k = GetPartImage(pn, i); 
	  if ( k == 3 ){
	    FreeImage(pn);
            kfaxerror( i18n("Fax G3 format not yet supported.") );
	    return k;
	  }

	}
    }

    // byte-swapping the image on little endian machines
#if defined(Q_BYTE_ORDER) && (Q_BYTE_ORDER == Q_LITTLE_ENDIAN)
    for (int y=pn->image.height()-1; y>=0; --y) {
      Q_UINT32 *source = (Q_UINT32 *) pn->image.scanLine(y);
      Q_UINT32 *dest   = source;
      for (int x=(pn->bytes_per_line/4)-1; x>=0; --x) {
 	Q_UINT32 dv = 0, sv = *source;
	for (int bit=32; bit>0; --bit) {
		dv <<= 1;
		dv |= sv&1;
		sv >>= 1;
	}
        *dest = dv;
	++dest;
	++source;
      }
    }
#endif

    kdDebug() << filename()
	<< "\n\tsize = " << pn->size
	<< "\n\tDPI = " << pn->dpi
	<< "\n\tresolution = " << (pn->vres ? "fine" : "normal")
	<< endl;

    return 1;
}


#include "kfaximage.moc"

#endif /* _LIBKFAXIMG_H_ */

