# Try to find the KIPI library
# Once done this will define
#
#  KIPI_FOUND - system has the KIPI library
#  KIPI_INCLUDE_DIR - the KIPI include directory
#  KIPI_LIBRARIES - Link this to use the KIPI library

if (KIPI_INCLUDE_DIR AND KIPI_LIBRARIES)
  # Already in cache
  set (KIPI_FOUND TRUE)
else (KIPI_INCLUDE_DIR AND KIPI_LIBRARIES)
  if (NOT WIN32)
    include(UsePkgConfig)
    pkgconfig(libkipi _kipiIncDir _kipiLinkDir kipiLinkFlags _kipiCflags)
  else (NOT WIN32)
    # if win32 we force find_path
    set (_kipiIncDir "")
  endif(NOT WIN32)

  if(_kipiIncDir)
    find_path(KIPI_INCLUDE_DIR libkipi/interface.h
      ${_kipiIncDir}
      ${KDE4_INCLUDE_DIR}
      ${INCLUDE_INSTALL_DIR}
      ${GNUWIN32_DIR}/include
    )

    find_library(KIPI_LIBRARIES NAMES kipi
      PATHS
      ${_kipiLinkDir}
      ${KDE4_LIB_DIR}
      ${LIB_INSTALL_DIR}
      ${GNUWIN32_DIR}/lib
    )
  endif(_kipiIncDir)

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Kipi DEFAULT_MSG KIPI_INCLUDE_DIR KIPI_LIBRARIES )

  mark_as_advanced(KIPI_INCLUDE_DIR KIPI_LIBRARIES)

endif (KIPI_INCLUDE_DIR AND KIPI_LIBRARIES)

