# - Try to find the DjVuLibre library
# Once done this will define
#
#  DJVULIBRE_FOUND - system has the DjVuLibre library
#  DJVULIBRE_INCLUDE_DIR - the DjVuLibre include directory
#  DJVULIBRE_LIBRARY - Link this to use the DjVuLibre library

if (DJVULIBRE_INCLUDE_DIR AND DJVULIBRE_LIBRARY)

  # in cache already
  set(DJVULIBRE_FOUND TRUE)

else (DJVULIBRE_INCLUDE_DIR AND DJVULIBRE_LIBRARY)
  IF (NOT WIN32)
  	include(UsePkgConfig)

  	pkgconfig(ddjvuapi _ddjvuIncDir _ddjvuLinkDir ddjvuLinkFlags _ddjvuCflags)
  endif(NOT WIN32)

  if(_ddjvuIncDir)
    find_path(DJVULIBRE_INCLUDE_DIR libdjvu/ddjvuapi.h
      ${_ddjvuIncDir}
      ${GNUWIN32_DIR}/include
    )

    find_library(DJVULIBRE_LIBRARY NAMES djvulibre
      PATHS
      ${_ddjvuLinkDir}
      ${GNUWIN32_DIR}/lib
    )
  endif(_ddjvuIncDir)

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(DjVuLibre DEFAULT_MSG DJVULIBRE_INCLUDE_DIR DJVULIBRE_LIBRARY )
  
  mark_as_advanced(DJVULIBRE_INCLUDE_DIR DJVULIBRE_LIBRARY)

endif (DJVULIBRE_INCLUDE_DIR AND DJVULIBRE_LIBRARY)
