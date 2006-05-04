# Marcus Hufgard <Hufgardm@hufgard.de> 2006
# cmake macro to test if we use sane
#
#  SANE_FOUND - system has SANE libs
#  SANE_INCLUDE_DIR - the SANE include directory
#  SANE_LIBRARIES - The libraries needed to use SANE

if (SANE_INCLUDE_DIR)
  # Already in cache, be silent
  set(SANE_FIND_QUIETLY TRUE)
endif (SANE_INCLUDE_DIR)

FIND_PATH(SANE_INCLUDE_DIR sane/sane.h
   /usr/include
   /usr/local/include
)

FIND_LIBRARY(SANE_LIBRARY NAMES  sane libsane
   PATHS
   /usr/lib/sane
   /usr/lib
   /usr/local/lib/sane
   /usr/local/lib
)

if (SANE_INCLUDE_DIR AND SANE_LIBRARY)
   set(SANE_FOUND TRUE)
   set(SANE_LIBRARIES ${SANE_LIBRARY})
else (SANE_INCLUDE_DIR AND SANE_LIBRARY)
   set(SANE_FOUND FALSE)
endif (SANE_INCLUDE_DIR AND SANE_LIBRARY)

if (SANE_FOUND)
   if (NOT Sane_FIND_QUIETLY)
      message(STATUS "Found sane: ${SANE_LIBRARIES}")
   endif (NOT Sane_FIND_QUIETLY)
else (SANE_FOUND)
   if (NOT Sane_FIND_QUIETLY)
      message(STATUS "didn't find SANE")
   endif (NOT Sane_FIND_QUIETLY)
endif (SANE_FOUND)

MARK_AS_ADVANCED(SANE_INCLUDE_DIR SANE_LIBRARIES SANE_LIBRARY)
