
#ifndef _DVIFILE_H
#define _DVIFILE_H

#include <stdio.h>
#include <qdatastream.h>
#include <qfile.h>
#include <qintdict.h>
#include <qstring.h>

#include "bigEndianByteReader.h"

class fontPool;
class TeXFontDefinition;

class dvifile : public bigEndianByteReader
{
 public:
  dvifile(QString fname, class fontPool *pool, bool sourceSpecialMark=true);
  ~dvifile();

  fontPool     * font_pool;
  QString        filename;
  QString        generatorString;
  Q_UINT16       total_pages;
  Q_UINT32     * page_offset;

  Q_UINT8      * dvi_Data;
  QIODevice::Offset size_of_file;
  QString        errorMsg;
  
  /** This flag is set to "true" during the construction of the
      dvifile, and is never changed afterwards by the dvifile
      class. It is used in kdvi in conjuction with source-specials:
      the first time a page with source specials is rendered, KDVI
      shows an info dialog, and the flag is set to false. That way
      KDVI ensures that the user is only informed once. */
  bool           sourceSpecialMarker;
  
  QIntDict<TeXFontDefinition> tn_table;

  double         cmPerDVIunit;

  /** This member is set to zero on construction and can be used by
      other software to count error messages that were printed when
      the DVI-file was processed. Suggested application: limit the
      number of error messages to, say, 25. */
  Q_UINT8        errorCounter;

 private:
  /** process_preamble reads the information in the preamble and
      stores it into global variables for later use. */
  /** read_postamble reads the information in the postamble, storing
      it into global variables. It also takes care of reading in all
      of the pixel files for the fonts used in the job. */
  void           process_preamble(void);
  void           find_postamble(void);
  void           read_postamble(void);
  void           prepare_pages(void);


  /** Offset in DVI file of last page, set in read_postamble(). */
  Q_UINT32       last_page_offset;
  Q_UINT32       beginning_of_postamble;
  Q_UINT32 magnification;
};

#endif //ifndef _DVIFILE_H
