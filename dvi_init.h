
#ifndef _DVIFILE_H
#define _DVIFILE_H

#include <stdio.h>
#include <qstring.h>


class dvifile {
 public:
  dvifile(QString fname);
  ~dvifile();

  QString        filename;
  FILE         * file;
  int	         total_pages;
  long         * page_offset;

  unsigned char  init_dvi_file(void);
  void           prepare_pages(void);
  void           read_postamble(void);
  void           find_postamble(void);
  void           process_preamble(void);
};

#endif //ifndef _DVIFILE_H
