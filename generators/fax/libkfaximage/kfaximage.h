/* 
   This file is part of KDE FAX image loading library
   Copyright (c) 2005 Helge Deller <deller@kde.org>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _LIBKFAXIMAGE_H_
#define _LIBKFAXIMAGE_H_

#include <qobject.h>
#include <qstring.h>
#include <qimage.h>
#include <q3ptrlist.h>
#include <kdelibs_export.h>

class pagenode;


/**
 *  This is KFaxImage, a class for loading FAX files under KDE
 *
 *  @short KFaxImage
 *  @author Helge Deller
 *  @version 0.1
 *
 *
 *  Standard fax dpi values:
 *  ------------------------
 *   Standard:		203 dpi x 98 lpi
 *   Fine:		203 dpi x 196 lpi
 *   Super Fine:	203 dpi y 392 lpi,  or
 *			406 dpi x 392 lpi
 */

#define KFAX_DPI_STANDARD	QPoint(203,98)
#define KFAX_DPI_FINE		QPoint(203,196)
#define KFAX_DPI_SUPERFINE	QPoint(406,392)


class KDE_EXPORT KFaxImage : public QObject
{
    Q_OBJECT

public:

    /**
     *  KFaxImage - the KDE FAX loader class
     *  constructor.
     *  @param filename: an optional FAX file which should be loaded.
     *  @see: numPages
     */

    KFaxImage( const QString &filename = QString::null, QObject *parent = 0, const char *name = 0 );

    /**
     *  Destructor
     *
     *  releases internal allocated memory.
     */

    virtual ~KFaxImage();

    /**
     *  loads the FAX image and returns true when sucessful.
     *  @param filename: the file which should be loaded.
     *  @return boolean value on success or failure
     *  @see: numPages
     *  @see: errorString
     */

    bool loadImage( const QString &filename );

    /**
     *  returns currently loaded image file name.
     *  @return FAX filename
     */

    QString filename() { return m_filename; };

    /**
     *  returns number of pages which are stored in the FAX file
     *  @return page count
     */

    unsigned int numPages() const { return m_pagenodes.count(); };

    /**
     *  returns a QImage which holds the contents of page pageNr
     *  @param pageNr: page number (starting with 0)
     *  @return QImage of the page pageNr
     */

    QImage page( unsigned int pageNr );

    /**
     *  returns the DPI (dots per inch) of page pageNr
     *  @param pageNr: page number (starting with 0)
     *  @return a QPoint value with the DPIs in X- and Y-direction
     */

    QPoint page_dpi( unsigned int pageNr );

    /**
     *  returns the physical pixels of page pageNr
     *  @param pageNr: page number (starting with 0)
     *  @return a QSize value with the width and height of the image
     */

    QSize page_size( unsigned int pageNr );

    /**
     *  @return a user-visible, translated error string
     */

    const QString errorString() const { return m_errorString; };



  private:

    /**
     *  private member variables
     */

    QString m_filename;
    QString m_errorString;

    typedef Q3PtrList<pagenode> t_PageNodeList;
    t_PageNodeList m_pagenodes;

    /**
     *  private member functions
     */

    void reset();
    void kfaxerror(const QString& error);
    pagenode *AppendImageNode(int type);
    bool NewImage(pagenode *pn, int w, int h);
    void FreeImage(pagenode *pn);
    unsigned char *getstrip(pagenode *pn, int strip);
    int GetPartImage(pagenode *pn, int n);
    int GetImage(pagenode *pn);
    pagenode *notefile(int type);
    int notetiff();
    void badfile(pagenode *pn);
};

#endif /* _LIBKFAXIMAGE_H_ */

