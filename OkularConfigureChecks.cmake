
option(
   OKULAR_FORCE_DRM
   "Forces okular to check for DRM to decide if you can copy/print protected pdf. (default=no)"
   OFF
)
if (OKULAR_FORCE_DRM)
   set(_OKULAR_FORCE_DRM 1)
else (OKULAR_FORCE_DRM)
   set(_OKULAR_FORCE_DRM 0)
endif (OKULAR_FORCE_DRM)

macro_optional_find_package(QImageBlitz)
macro_log_feature(QIMAGEBLITZ_FOUND "QImageBlitz" "A a graphical effect and filter library for KDE4" "http://sourceforge.net/projects/qimageblitz" FALSE "kdesupport" "Required to build kolourpaint.")

macro_optional_find_package(Kate)
macro_log_feature(LIBKATE_FOUND "KATE" "A library for adding Kate support" "http://libkate.googlecode.com" FALSE "" "Required to export presentations as Kate streams from Okular.")

if (LIBKATE_FOUND)
   add_definitions(-DHAVE_KATE)
   include_directories(${LIBKATE_INCLUDE_DIR})
else (LIBKATE_FOUND)
   remove_definitions(-DHAVE_KATE)
endif (LIBKATE_FOUND)

# at the end, output the configuration
configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/config-okular.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/config-okular.h
)

