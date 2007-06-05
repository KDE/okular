# - Try to find the Exiv2 library
# Once done this will define
#
#  EXIV2_FOUND - system has EXIV2
#  EXIV2_INCLUDE_DIR - the EXIV2 include directory
#  EXIV2_LIBRARIES - Link these to use EXIV2
#  EXIV2_DEFINITIONS - Compiler switches required for using EXIV2

# Copyright (c) 2007, Aurelien Gateau, <aurelien.gateau@free.fr>
# Based on FindIMLIB.cmake
# Copyright (c) 2006, Dirk Mueller, <mueller@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


IF (DEFINED CACHED_EXIV2)

  # in cache already
  IF ("${CACHED_EXIV2}" STREQUAL "YES")
    SET(EXIV2_FOUND TRUE)
  ENDIF ("${CACHED_EXIV2}" STREQUAL "YES")

ELSE (DEFINED CACHED_EXIV2)

IF (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  INCLUDE(UsePkgConfig)
  
  PKGCONFIG(exiv2 _EXIV2IncDir _EXIV2LinkDir _EXIV2LinkFlags _EXIV2Cflags)
  
  set(EXIV2_DEFINITIONS ${_EXIV2Cflags})
ENDIF (NOT WIN32) 

  FIND_PATH(EXIV2_INCLUDE_DIR exiv2/exiv2_version.h
    ${_EXIV2IncDir}
    /usr/include
    /usr/local/include
  )
  
  FIND_LIBRARY(EXIV2_LIBRARIES NAMES exiv2 libexiv2
    PATHS
    ${_EXIV2LinkDir}
    /usr/lib
    /usr/local/lib
  )

  if (EXIV2_INCLUDE_DIR AND EXIV2_LIBRARIES)
     SET(EXIV2_FOUND true)
  endif (EXIV2_INCLUDE_DIR AND EXIV2_LIBRARIES)
 
  if (EXIV2_FOUND)
    set(CACHED_EXIV2 "YES")
    if (NOT Exiv2_FIND_QUIETLY)
      message(STATUS "Found EXIV2: ${EXIV2_LIBRARIES}")
    endif (NOT Exiv2_FIND_QUIETLY)
  else (EXIV2_FOUND)
    set(CACHED_EXIV2 "NO")
    if (NOT Exiv2_FIND_QUIETLY)
      message(STATUS "didn't find EXIV2")
    endif (NOT Exiv2_FIND_QUIETLY)
    if (Exiv2_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find EXIV2")
    endif (Exiv2_FIND_REQUIRED)
  endif (EXIV2_FOUND)
  
  MARK_AS_ADVANCED(EXIV2_INCLUDE_DIR EXIV2_LIBRARIES)
  
  set(CACHED_EXIV2 ${CACHED_EXIV2} CACHE INTERNAL "If libexiv2 was checked")

ENDIF (DEFINED CACHED_EXIV2)

