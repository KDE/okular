# - Try to find the chm library
# Once done this will define
#
#  CHM_FOUND - system has the chm library
#  CHM_INCLUDE_DIR - the chm include directory
#  CHM_LIBRARY - Link this to use the chm library
#
include(CheckLibraryExists)

# reset vars
set(CHM_INCLUDE_DIR)
set(CHM_LIBRARY)

FIND_PATH(CHM_INCLUDE_DIR chm_lib.h
  /usr/local/include
  /usr/include
  ${GNUWIN32_DIR}/include
)

#check_library_exists(chm ??? "" CHM_LIBRARY)

if(CHM_INCLUDE_DIR)
  set(CHM_FOUND TRUE)
endif(CHM_INCLUDE_DIR)

if (CHM_FOUND)
  if (NOT CHM_FIND_QUIETLY)
    message(STATUS "Found CHM: ${CHM_LIBRARY}")
  endif (NOT CHM_FIND_QUIETLY)
else (CHM_FOUND)
  if (CHM_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find CHM")
  endif (CHM_FIND_REQUIRED)
endif (CHM_FOUND)

MARK_AS_ADVANCED(CHM_INCLUDE_DIR CHM_LIBRARY)
