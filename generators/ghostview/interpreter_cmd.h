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

#ifndef _KPDF_GVINTERPETERCMD_H_
#define _KPDF_GVINTERPETERCMD_H_

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <fixx11h.h>

#include <kprocess.h>
#include <qwidget.h>
#include <qmutex.h>

#include "internaldocument.h"
#include "core/generator.h"
#include "dscparse_adapter.h"


class QString;
class QStringList;

/**
 * @page ghostview_interface Ghostview interface to Ghostscript
 *
 * When the GHOSTVIEW environment variable is set, Ghostscript draws on
 * an existing drawable rather than creating its own window. Ghostscript
 * can be directed to draw on either a window or a pixmap.
 *
 * 
 * @section window Drawing on a Window
 *
 * The GHOSTVIEW environment variable contains the window id of the target
 * window.  The window id is an integer.  Ghostscript will use the attributes
 * of the window to obtain the width, height, colormap, screen, and visual of
 * the window. The remainder of the information is gotten from the GHOSTVIEW
 * property on that window.
 *
 * 
 * @section pixmap Drawing on a Pixmap
 *
 * The GHOSTVIEW environment variable contains a window id and a pixmap id.
 * They are integers separated by white space.  Ghostscript will use the
 * attributes of the window to obtain the colormap, screen, and visual to use.
 * The width and height will be obtained from the pixmap. The remainder of the
 * information, is gotten from the GHOSTVIEW property on the window.  In this
 * case, the property is deleted when read.
 *
 * 
 * @section gv_variable The GHOSTVIEW environment variable
 *
 * @par parameters: 
 *     <tt> window-id [pixmap-id] </tt>
 *
 * @par scanf format: 
 *     @code "%d %d" @endcode
 *
 * @par Explanation of the parameters
 *     @li @e window-id: 
 *         tells Ghostscript where to:
 *             - read the GHOSTVIEW property
 *             - send events
 *         If pixmap-id is not present, Ghostscript will draw on this window.
 *
 *     @li @e pixmap-id: 
 *         If present, tells Ghostscript that a pixmap will be used as the 
 *         final destination for drawing. The window will not be touched for 
 *         drawing purposes.
 *
 *
 * @section gv_property The GHOSTVIEW property
 *
 * @par type: 
 *     STRING
 *
 * @par parameters: 
 *     <tt> bpixmap orient llx lly urx ury xdpi ydpi [left bottom top right] 
 *     </tt>
 *
 * @par scanf format: 
 *     @code "%d %d %d %d %d %d %f %f %d %d %d %d" @endcode
 *
 * @par Explanation of the parameters
 *     @li @e bpixmap:
 *         pixmap id of the backing pixmap for the window. If no pixmap is to
 *         be used, this parameter should be zero. This parameter must be zero
 *         when drawing on a pixmap.
 *
 *     @li <em>orient:</em> 
 *         orientation of the page. The number represents clockwise rotation 
 *         of the paper in degrees.  Permitted values are 0, 90, 180, 270.
 *
 *     @li <em>llx, lly, urx, ury:</em>
 *         Bounding box of the drawable. The bounding box is specified in 
 *         PostScript points in default user coordinates. (Note the word 
 *         @e drawable. This means that this bounding box is generally not 
 *         the same as the BoundingBox specified in the DSC. In case 
 *         DocumentMedia is specified, it is equal to the Media's bounding 
 *         box.)
 *
 *     @li <em>xdpi, ydpi:</em> 
 *         Resolution of window. (This can be derived from the other 
 *         parameters, but not without roundoff error. These values are 
 *         included to avoid this error.)
 *
 *     @li <em>left, bottom, top, right (optional):</em>
 *         Margins around the window. The margins extend the imageable area 
 *         beyond the boundaries of the window. This is primarily used for 
 *         popup zoom windows. I have encountered several instances of 
 *         PostScript programs that position themselves with respect to the 
 *         imageable area. The margins are specified in PostScript points.  
 *         If omitted, the margins are assumed to be 0.
 *
 * 
 * @section events Events from Ghostscript
 *
 * If the final destination is a pixmap, the client will get a property 
 * notify event when Ghostscript reads the GHOSTVIEW property causing it to 
 * be deleted.
 *
 * Ghostscript sends events to the window where it read the GHOSTVIEW 
 * property. These events are of type ClientMessage.  The message_type is set 
 * to either PAGE or DONE.  The first long data value gives the window to be 
 * used to send replies to Ghostscript.  The second long data value gives the 
 * primary drawable. If rendering to a pixmap, it is the primary drawable. 
 * If rendering to a window, the backing pixmap is the primary drawable. If 
 * no backing pixmap is employed, then the window is the primary drawable. 
 * This field is necessary to distinguish multiple Ghostscripts rendering to 
 * separate pixmaps where the GHOSTVIEW property was placed on the same 
 * window.
 *
 * The PAGE message indicates that a "page" has completed.  Ghostscript will
 * wait until it receives a ClientMessage whose message_type is NEXT before
 * continuing.
 *
 * The DONE message indicates that Ghostscript has finished processing.
 */


class GVInterpreterCMD : public QWidget
{
    Q_OBJECT
    public:
        GVInterpreterCMD( QWidget* parent, const char* name , const QString & fileName);
        ~GVInterpreterCMD();
        QPixmap* takePixmap() { return m_pixmap; m_pixmap=0; }; 
        bool startInterpreter();
        void setGhostscriptPath( const QString& path );
        void setGhostscriptArguments( const QStringList& arguments );
        void setOrientation( CDSC_ORIENTATION_ENUM orientation );
        void setBoundingBox( const KDSCBBOX& boundingBox );
        void setMagnification( double magnification );
        bool sendPS( FILE* fp, const PagePosition *pos, PixmapRequest * req );
        bool x11Event( XEvent* e );
        bool busy();
        bool running() { return m_process && m_process->isRunning(); };
        QMutex m_work;
    signals:
    /**
     * This signal gets emited whenever a page is finished, but contains a reference to the pixmap
     * used to hold the image.
     *
     * Don't change the pixmap or bad things will happen. This is the backing pixmap of the display.
     */
        void newPageImage( PixmapRequest *req );

    /**
     * This signal is emitted whenever the ghostscript process has
     * written data to stdout or stderr.
     */
        void output( char* data, int len );

    /**
     * This means that gs exited uncleanly
     *
     * @param msg a <strong>translated</strong> error message to display the user which may be null if we cannot tell anything important
     */
        void ghostscriptError( const QString& mgs );

    private slots:
        void slotProcessExited( KProcess* process );
        void gs_output( KProcess*, char* buffer, int len );
        void gs_input( KProcess* process );

    private:
        // functions
        void stopInterpreter();

        void setupWidget();

        // x11 communication stuff
        enum AtomName { GHOSTVIEW = 0, GHOSTVIEW_COLORS, NEXT, PAGE, DONE };
        Atom m_atoms[5];
        Window comm_window;

        // state bools
        bool m_useWindow;
        bool m_needUpdate;
        bool m_stdinReady;
        bool m_busy;
        bool m_usePipe;

        char *m_buffer;
        KDSCBBOX m_boundingBox;

        // result
        QPixmap* m_pixmap;

        // settings stuff
        QString m_path;
        QStringList m_args;

        // process stuff
        KProcess *m_process;

        // FILE INFORMATION:
        // hold pointer to a file never delete it, it should 
        // change everytime new request is done
        double m_magnify;
        CDSC_ORIENTATION_ENUM m_orientation;
        QString m_name;
        FILE * m_tmpFile;
        unsigned long m_begin;
        unsigned int m_len;
        PixmapRequest* m_req;
};
#endif
