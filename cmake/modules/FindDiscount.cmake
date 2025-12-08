# - Find Discount
# Find the discount markdown library.
#
# This module defines
#  Discount_FOUND - whether the discount library was found
#  PkgConfig::Discount - the discount target to link to

# SPDX-FileCopyrightText: 2017 Julian Wolff <wolff@julianwolff.de>
# SPDX-FileCopyrightText: 2018 Sune Vuorela <sune@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

include(FindPackageHandleStandardArgs)

find_package(PkgConfig)

if (PkgConfig_FOUND)
  pkg_check_modules(Discount IMPORTED_TARGET "libmarkdown")
else ()
  message(WARNING "PkgConfig not found, can't find Discount")
endif()

find_package_handle_standard_args(Discount DEFAULT_MSG Discount_LIBRARIES)
