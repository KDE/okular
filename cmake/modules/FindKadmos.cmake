#
#  KADMOS_FOUND - system has KADMOS libs
#  KADMOS_INCLUDE_DIR - the KADMOS include directory
#  KADMOS_LIBRARIES - The libraries needed to use KADMOS

# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (KADMOS_INCLUDE_DIR)
  # Already in cache, be silent
  set(Kadmos_FIND_QUIETLY TRUE)
endif (KADMOS_INCLUDE_DIR)

FIND_PATH(KADMOS_INCLUDE_DIR kadmos.h)

FIND_PATH(KADMOS_LIBRARY NAMES librep.a)

if (KADMOS_INCLUDE_DIR AND KADMOS_LIBRARY)
   set(KADMOS_FOUND TRUE)
   set(KADMOS_LIBRARIES ${SANE_LIBRARY})
else (KADMOS_INCLUDE_DIR AND KADMOS_LIBRARY)
   set(KADMOS_FOUND FALSE)
endif (KADMOS_INCLUDE_DIR AND KADMOS_LIBRARY)

if (KADMOS_FOUND)
   if (NOT Kadmos_FIND_QUIETLY)
      message(STATUS "Found kadmos: ${KADMOS_LIBRARIES}")
   endif (NOT Kadmos_FIND_QUIETLY)
else (KADMOS_FOUND)
   if (NOT Kadmos_FIND_REQUIRED)
      message(FATAL_ERROR "didn't find KADMOS")
   endif (NOT Kadmos_FIND_REQUIRED)
endif (KADMOS_FOUND)

MARK_AS_ADVANCED(KADMOS_INCLUDE_DIR KADMOS_LIBRARIES KADMOS_LIBRARY)

