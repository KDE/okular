# Try to find the KIPI library
# Once done this will define
#
#  KIPI_FOUND - system has the KIPI library
#  KIPI_INCLUDE_DIR - the KIPI include directory
#  KIPI_LIBRARIES - Link this to use the KIPI library


FIND_PATH(KIPI_INCLUDE_DIR
  NAMES
  libkipi/interface.h
  PATHS
  ${KDE4_INCLUDE_DIR}
  ${INCLUDE_INSTALL_DIR}
  )

FIND_LIBRARY(KIPI_LIBRARY
  NAMES
  kipi
  PATHS
  ${KDE4_LIB_DIR}
  ${LIB_INSTALL_DIR}
  )

set(KIPI_LIBRARIES ${KIPI_LIBRARY})

if(KIPI_INCLUDE_DIR AND KIPI_LIBRARIES)
  set(KIPI_FOUND TRUE)
endif(KIPI_INCLUDE_DIR AND KIPI_LIBRARIES)

if(KIPI_FOUND)
  if(NOT KIPI_FIND_QUIETLY)
    message(STATUS "Found KIPI: ${KIPI_LIBRARIES}")
  endif(NOT KIPI_FIND_QUIETLY)
else(KIPI_FOUND)
  if(KIPI_FIND_REQUIRED)
    if(NOT KIPI_INCLUDE_DIR)
      message(FATAL_ERROR "Could not find KIPI includes.")
    endif(NOT KIPI_INCLUDE_DIR)
    if(NOT KIPI_LIBRARIES)
      message(FATAL_ERROR "Could not find KIPI library.")
    endif(NOT KIPI_LIBRARIES)
  endif(KIPI_FIND_REQUIRED)
endif(KIPI_FOUND)
