
#ifndef _DVIFILE_H
#define _DVIFILE_H

#include <stdio.h>
#include <qstring.h>

class fontPool;

class dvifile {
 public:
  dvifile(QString fname, class fontPool *pool);
  ~dvifile();

  fontPool     * font_pool;
  QString        filename;
  QString        generatorString;
  FILE         * file;
  int	         total_pages;
  long         * page_offset;

  /** Numerator and denominator of the TeX units, as explained in
      section A.3 of the DVI driver standard, Level 0, published by
      the TUG DVI driver standards committee. */
  unsigned long           numerator, denominator;

  /** Magnification value, as explained in section A.3 of the DVI
      driver standard, Level 0, published by the TUG DVI driver
      standards committee. */
  unsigned long	        magnification;

  /** dimconv = numerator*magnification/(1000*denominator), as
      explained in section A.3 of the DVI driver standard, Level 0,
      published by the TUG DVI driver standards committee. */
  double         dimconv;

  /** Offset in DVI file of last page, set in read_postamble(). */
  long           last_page_offset;

  /** init_dvi_file is the main subroutine for reading the startup
      information from the dvi file. Returns True on success.  */
  unsigned char  init_dvi_file(void);
  void           prepare_pages(void);

  /** read_postamble reads the information in the postamble, storing
      it into global variables. It also takes care of reading in all
      of the pixel files for the fonts used in the job. */
  void           read_postamble(void);
  void           find_postamble(void);

  /** process_preamble reads the information in the preamble and
      stores it into global variables for later use. */
  void           process_preamble(void);
};

#endif //ifndef _DVIFILE_H
