
#  KADMOS_FOUND - system has KADMOS libs
#  KADMOS_INCLUDE_DIR - the KADMOS include directory
#  KADMOS_LIBRARIES - The libraries needed to use KADMOS

# Copyright (c) 2006, 2007 Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (KADMOS_INCLUDE_DIR AND KADMOS_LIBRARY)
  # Already in cache, be silent
  set(Kadmos_FIND_QUIETLY TRUE)
endif (KADMOS_INCLUDE_DIR AND KADMOS_LIBRARY)

FIND_PATH(KADMOS_INCLUDE_DIR kadmos.h)

FIND_PATH(KADMOS_LIBRARY NAMES librep.a)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Kadmos DEFAULT_MSG KADMOS_INCLUDE_DIR KADMOS_LIBRARY )

MARK_AS_ADVANCED(KADMOS_INCLUDE_DIR KADMOS_LIBRARIES KADMOS_LIBRARY)

