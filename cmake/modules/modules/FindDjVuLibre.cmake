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

  include(UsePkgConfig)

  pkgconfig(ddjvuapi _ddjvuIncDir _ddjvuLinkDir ddjvuLinkFlags _ddjvuCflags)

  find_path(DJVULIBRE_INCLUDE_DIR libdjvu/ddjvuapi.h
    ${_ddjvuIncDir}
    /usr/local/include
    /usr/include
    ${GNUWIN32_DIR}/include
  )

  find_library(DJVULIBRE_LIBRARY NAMES djvulibre
    PATHS
    ${_ddjvuLinkDir}
    /usr/lib
    /usr/local/lib
    ${GNUWIN32_DIR}/lib
  )

  if(DJVULIBRE_INCLUDE_DIR AND DJVULIBRE_LIBRARY)
    set(DJVULIBRE_FOUND TRUE)
  endif(DJVULIBRE_INCLUDE_DIR AND DJVULIBRE_LIBRARY)

  if (DJVULIBRE_FOUND)
    if (NOT DJVULIBRE_FIND_QUIETLY)
      message(STATUS "Found DjVuLibre: ${DJVULIBRE_LIBRARY}")
    endif (NOT DJVULIBRE_FIND_QUIETLY)
  else (DJVULIBRE_FOUND)
    if (DJVULIBRE_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find DjVuLibre")
    endif (DJVULIBRE_FIND_REQUIRED)
  endif (DJVULIBRE_FOUND)

  mark_as_advanced(DJVULIBRE_INCLUDE_DIR DJVULIBRE_LIBRARY)

endif (DJVULIBRE_INCLUDE_DIR AND DJVULIBRE_LIBRARY)
