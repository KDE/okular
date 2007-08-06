# - Try to find the chm library
# Once done this will define
#
#  CHM_FOUND - system has the chm library
#  CHM_INCLUDE_DIR - the chm include directory
#  CHM_LIBRARY - Link this to use the chm library
#
# Copyright (c) 2006, Pino Toscano, <toscano.pino@tiscali.it>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (CHM_LIBRARY AND CHM_INCLUDE_DIR)
  # in cache already
  set(CHM_FOUND TRUE)
else (CHM_LIBRARY AND CHM_INCLUDE_DIR)

  find_path(CHM_INCLUDE_DIR chm_lib.h
    ${GNUWIN32_DIR}/include
  )

  find_library(CHM_LIBRARY NAMES chm
    PATHS
    ${GNUWIN32_DIR}/lib
  )

  if(CHM_INCLUDE_DIR AND CHM_LIBRARY)
    set(CHM_FOUND TRUE)
  endif(CHM_INCLUDE_DIR AND CHM_LIBRARY)

  if (CHM_FOUND)
    if (NOT CHM_FIND_QUIETLY)
      message(STATUS "Found CHM: ${CHM_LIBRARY}")
    endif (NOT CHM_FIND_QUIETLY)
  else (CHM_FOUND)
    if (CHM_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find CHM")
    endif (CHM_FIND_REQUIRED)
  endif (CHM_FOUND)

    # ensure that they are cached
    set(CHM_INCLUDE_DIR ${CHM_INCLUDE_DIR} CACHE INTERNAL "The chmlib include path")
    set(CHM_LIBRARY ${CHM_LIBRARY} CACHE INTERNAL "The libraries needed to use chmlib")

endif (CHM_LIBRARY AND CHM_INCLUDE_DIR)
