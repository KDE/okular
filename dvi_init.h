
#ifndef _DVIFILE_H
#define _DVIFILE_H

#include <stdio.h>
#include <qstring.h>

#include <fontpool.h>

class dvifile {
 public:
  dvifile(QString fname, class fontPool *pool);
  ~dvifile();

  fontPool     * font_pool;
  QString        filename;
  FILE         * file;
  int	         total_pages;
  long         * page_offset;

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
