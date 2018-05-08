# - Find Discount
# Find the discount markdown library.
#
# This module defines
#  discount_FOUND - whether the discount library was found
#  discount_LIBRARIES - the discount library
#  discount_INCLUDE_DIR - the include path of the discount library

# Copyright (c) 2017, Julian Wolff, <wolff@julianwolff.de>
# Copyright (c) 2018, Sune Vuorela, <sune@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (discount_INCLUDE_DIR AND discount_LIBRARIES)

  # Already in cache
  set (discount_FOUND TRUE)

else (discount_INCLUDE_DIR AND discount_LIBRARIES)

  find_library (discount_LIBRARIES
    NAMES markdown libmarkdown
  )

  find_path (discount_INCLUDE_DIR
    NAMES mkdio.h
  )

  include (FindPackageHandleStandardArgs)
  find_package_handle_standard_args (discount DEFAULT_MSG discount_LIBRARIES discount_INCLUDE_DIR)

endif (discount_INCLUDE_DIR AND discount_LIBRARIES)

mark_as_advanced(discount_INCLUDE_DIR discount_LIBRARIES)

if (discount_FOUND)
   add_library(discount::Lib UNKNOWN IMPORTED)
   set_target_properties(discount::Lib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${discount_INCLUDE_DIR} IMPORTED_LOCATION ${discount_LIBRARIES})
endif()
