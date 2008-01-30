# - Try to find the libspectre PS library
# Once done this will define
#
#  LIBSPECTRE_FOUND - system has libspectre
#  LIBSPECTRE_INCLUDE_DIR - the libspectre include directory
#  LIBSPECTRE_LIBRARY - Link this to use libspectre
#

# Copyright (c) 2006-2007, Pino Toscano, <pino@kde.org>
# Copyright (c) 2008, Albert Astals Cid, <aacid@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(LIBSPECTRE_INCLUDE_DIR AND LIBSPECTRE_LIBRARY)

  # in cache already
  set(LIBSPECTRE_FOUND TRUE)

else(LIBSPECTRE_INCLUDE_DIR AND LIBSPECTRE_LIBRARY)

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
INCLUDE(UsePkgConfig)

PKGCONFIG(libspectre _SpectreIncDir _SpectreLinkDir _SpectreLinkFlags _SpectreCflags)

if(_SpectreLinkFlags)
  # find again pkg-config, to query it about libspectre version
  FIND_PROGRAM(PKGCONFIG_EXECUTABLE NAMES pkg-config PATHS /usr/bin/ /usr/local/bin )

  # query pkg-config asking for a libspectre >= LIBSPECTRE_MINIMUM_VERSION
  EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS --atleast-version=${LIBSPECTRE_MINIMUM_VERSION} libspectre RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _pkgconfigDevNull )
  if(_return_VALUE STREQUAL "0")
    set(LIBSPECTRE_FOUND TRUE)
  endif(_return_VALUE STREQUAL "0")
endif(_SpectreLinkFlags)

if (LIBSPECTRE_FOUND)
  set(LIBSPECTRE_LIBRARY ${_SpectreLinkFlags})

  # the cflags for libspectre can contain more than one include path
  separate_arguments(_SpectreCflags)
  foreach(_includedir ${_SpectreCflags})
    string(REGEX REPLACE "-I(.+)" "\\1" _includedir "${_includedir}")
    set(LIBSPECTRE_INCLUDE_DIR ${LIBSPECTRE_INCLUDE_DIR} ${_includedir})
  endforeach(_includedir)

  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_LIBRARIES)
else (LIBSPECTRE_FOUND)
  if (LIBSPECTRE_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find libspectre")
  endif (LIBSPECTRE_FIND_REQUIRED)
  message(STATUS "Could not find OPTIONAL package libspectre")
endif (LIBSPECTRE_FOUND)

# ensure that they are cached
set(LIBSPECTRE_INCLUDE_DIR ${LIBSPECTRE_INCLUDE_DIR} CACHE INTERNAL "The libspectre include path")
set(LIBSPECTRE_LIBRARY ${LIBSPECTRE_LIBRARY} CACHE INTERNAL "The libspectre library")

endif(LIBSPECTRE_INCLUDE_DIR AND LIBSPECTRE_LIBRARY)
