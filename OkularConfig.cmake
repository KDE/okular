# get from the full path to OkularConfig.cmake up to the base dir dir:
get_filename_component( _okularBaseDir  ${CMAKE_CURRENT_LIST_FILE} PATH)
get_filename_component( _okularBaseDir  ${_okularBaseDir} PATH)
get_filename_component( _okularBaseDir  ${_okularBaseDir} PATH)
get_filename_component( _okularBaseDir  ${_okularBaseDir} PATH)


# find the full paths to the library and the includes:
find_path(OKULAR_INCLUDE_DIR okular/core/document.h
          HINTS ${_okularBaseDir}/include
          NO_DEFAULT_PATH)

find_library(OKULAR_CORE_LIBRARY okularcore 
             HINTS ${_okularBaseDir}/lib
             NO_DEFAULT_PATH)

set(OKULAR_LIBRARIES ${OKULAR_CORE_LIBRARY})

# Compat: the old FindOkular.cmake was setting OKULAR_FOUND and the
# new one sets Okular_FOUND.
if(OKULAR_INCLUDE_DIR AND OKULAR_CORE_LIBRARY)
  set(OKULAR_FOUND TRUE)
endif()

