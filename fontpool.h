
#ifndef _FONTPOOL_H
#define _FONTPOOL_H

#include <qlist.h>

#include "font.h"

/**
 *  A list of fonts and a compilation of utility functions
 *
 * This class holds a list of fonts and is able to perform a number of
 * functions on each of the fonts. The main use of this class is that
 * it is able to control a concurrently running "kpsewhich" programm
 * which is used to locate and load the fonts.
 *
 *@author Stefan Kebekus   <kebekus@kde.org>
 *
 *
 **/

class fontPool : public QList<struct font> {
 public:

  /** Method used to set the Base resolution for the PK font
   * files. This data is used when loading fonts, and it must match
   * the Metafontmode which is set with the setMetafontMode method
   * below. Currently, a change here will be applied only to those
   * font which were not yet loaded ---expect funny results when
   * changing the data in the mid-work. */
  void setResolution( int basedpi ); 

  /** Method used to set the MetafontMode for the PK font files. This
   * data is used when loading fonts, and it must match the resolution
   * which is set with the setResolution method above. Currently, a
   * change here will be applied only to those font which were not yet
   * loaded ---expect funny results when changing the data in the
   * mid-work. */
  void setMetafontMode( const QString & );
  
  /** This method adds a font to the list. If the font is not
   * currently loaded, it's file will be located and font::load_font
   * will be called. Since this is done using a concurrently running
   * process, there is no guarantee that the loading is already
   * performed when the method returns.  */
  void          appendx(const font *fontp);

  /** Prints very basic debugging information about the fonts in the
   * pool to the kdDebug output stream. */
  void          status(); 

 private:
  int     Resolution;
  QString MetafontMode;
};

#endif //ifndef _FONTPOOL_H
