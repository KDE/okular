# - Try to find the qgs library
# Once done this will define
#
#  QGS_FOUND - system has qgs
#  QGS_INCLUDE_DIR - the qgs include directory
#  QGS_LIBRARY - Link this to use qgs
#
include(CheckLibraryExists)

# reset vars
set(QGS_INCLUDE_DIR)
set(QGS_LIBRARY)

FIND_PATH(QGS_INCLUDE_DIR qgs.h
  /usr/local/include
  /usr/include
  ${GNUWIN32_DIR}/include
  ${KDE4_INCLUDE_DIR}
)

# ${KDE4_INCLUDE_DIR}/../lib IS UBER UGLY but i did not found a better way
find_library(QGS_LIBRARY NAMES qgs
  PATHS
  /usr/lib
  /usr/local/lib
  ${GNUWIN32_DIR}/lib
  ${KDE4_INCLUDE_DIR}/../lib
)

if(QGS_INCLUDE_DIR AND QGS_LIBRARY)
  set(QGS_FOUND TRUE)
endif(QGS_INCLUDE_DIR AND QGS_LIBRARY)

if (QGS_FOUND)
  if (NOT QGS_FIND_QUIETLY)
    message(STATUS "Found qgs: ${QGS_LIBRARY}")
  endif (NOT QGS_FIND_QUIETLY)
else (QGS_FOUND)
  if (QGS_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find QGS")
  endif (QGS_FIND_REQUIRED)
endif (QGS_FOUND)

MARK_AS_ADVANCED(QGS_INCLUDE_DIR QGS_LIBRARY)
