/* config.h: master configuration file, included first by all compilable
   source files (not headers).  */

#ifndef CONFIG_H
#define CONFIG_H

/* The stuff from the path searching library.  */
#include <kpathsea/config.h>

#include <setjmp.h>

/* Some kdvi options we want by default.  */
#define USE_PK
#define USE_GF
#define GREY
#define TEXXET
#define PS_GS
#define KDVI

/* xdvi's definitions.  */
#include "xdvi.h"

#endif /* not CONFIG_H */
