# - Try to find the libkate library
# Once done this will define
#
#  LIBKATE_FOUND - system has libkate
#  LIBKATE_INCLUDE_DIR - the libkate include directory
#  LIBKATE_LIBRARY - Link this to use libkate
#

# Copyright (c) 2006-2007, Pino Toscano, <pino@kde.org>
# Copyright (c) 2008, Albert Astals Cid, <aacid@kde.org>
# Copyright (c) 2009, ogg.k.ogg.k, <ogg.k.ogg.k@googlemail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(LIBKATE_INCLUDE_DIR AND LIBKATE_LIBRARY)

  # in cache already
  set(LIBKATE_FOUND TRUE)

else(LIBKATE_INCLUDE_DIR AND LIBKATE_LIBRARY)

if(NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   INCLUDE(UsePkgConfig)

   PKGCONFIG(oggkate _KateIncDir _KateLinkDir _KateLinkFlags _KateCflags)

   if(_KateLinkFlags)
     # find again pkg-config, to query it about libkate version
     FIND_PROGRAM(PKGCONFIG_EXECUTABLE NAMES pkg-config PATHS /usr/bin/ /usr/local/bin )

     # query pkg-config asking for a libkate >= LIBKATE_MINIMUM_VERSION
     EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS --atleast-version=${LIBKATE_MINIMUM_VERSION} oggkate RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _pkgconfigDevNull )
     if(_return_VALUE STREQUAL "0")
        set(LIBKATE_FOUND TRUE)
     endif(_return_VALUE STREQUAL "0")
   endif(_KateLinkFlags)
else(NOT WIN32)
    # do not use pkg-config on windows
    find_library(_KateLinkFlags NAMES liboggkate kate PATHS ${CMAKE_LIBRARY_PATH})
    
    find_path(LIBKATE_INCLUDE_DIR kate/oggkate.h PATH_SUFFIXES libkate )
    
    set(LIBKATE_FOUND TRUE)
endif(NOT WIN32)

if (LIBKATE_FOUND)
  set(LIBKATE_LIBRARY ${_KateLinkFlags})

  # the cflags for libkate can contain more than one include path
  separate_arguments(_KateCflags)
  foreach(_includedir ${_KateCflags})
    string(REGEX REPLACE "-I(.+)" "\\1" _includedir "${_includedir}")
    set(LIBKATE_INCLUDE_DIR ${LIBKATE_INCLUDE_DIR} ${_includedir})
  endforeach(_includedir)

endif (LIBKATE_FOUND)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libkate DEFAULT_MSG LIBKATE_LIBRARY LIBKATE_FOUND)

# ensure that they are cached
set(LIBKATE_INCLUDE_DIR ${LIBKATE_INCLUDE_DIR} CACHE INTERNAL "The libkate include path")
set(LIBKATE_LIBRARY ${LIBKATE_LIBRARY} CACHE INTERNAL "The libkate library")

endif(LIBKATE_INCLUDE_DIR AND LIBKATE_LIBRARY)
