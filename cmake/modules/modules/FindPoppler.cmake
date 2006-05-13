# - Try to find the poppler PDF library
# Once done this will define
#
#  POPPLER_FOUND - system has poppler
#  POPPLER_INCLUDE_DIR - the poppler include directory
#  POPPLER_LIBRARY - Link this to use poppler
#

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
INCLUDE(UsePkgConfig)

PKGCONFIG(poppler-qt4 _PopplerIncDir _PopplerLinkDir _PopplerLinkFlags _PopplerCflags)

if(_PopplerLinkFlags)
  # find again pkg-config, to query it about poppler version
  FIND_PROGRAM(PKGCONFIG_EXECUTABLE NAMES pkg-config PATHS /usr/bin/ /usr/local/bin )

  # query pkg-config asking for a poppler-qt4 >= 0.5.1
  EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS --atleast-version=0.5.1 poppler-qt4 RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _pkgconfigDevNull )
  if(_return_VALUE STREQUAL "0")
    set(POPPLER_FOUND TRUE)
  endif(_return_VALUE STREQUAL "0")
endif(_PopplerLinkFlags)

if (POPPLER_FOUND)
  set(POPPLER_INCLUDE_DIR ${_PopplerIncDir})
  set(POPPLER_LIBRARY ${_PopplerLinkFlags})
  if (NOT POPPLER_FIND_QUIETLY)
    message(STATUS "Found Poppler-qt4: ${POPPLER_LIBRARY}")
  endif (NOT POPPLER_FIND_QUIETLY)
else (POPPLER_FOUND)
  if (POPPLER_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find Poppler-qt4")
  endif (POPPLER_FIND_REQUIRED)
endif (POPPLER_FOUND)

MARK_AS_ADVANCED(POPPLER_INCLUDE_DIR POPPLER_LIBRARY)
