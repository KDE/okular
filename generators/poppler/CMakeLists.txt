add_subdirectory( conf )

set(CMAKE_REQUIRED_INCLUDES ${POPPLER_INCLUDE_DIR} ${QT_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${POPPLER_LIBRARY}   Qt5::Xml)

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/config-okular-poppler.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/config-okular-poppler.h
)

# Can be multiple include directories for poppler
separate_arguments(POPPLER_INCLUDE_DIR)
foreach(_include_dir ${POPPLER_INCLUDE_DIR})
    include_directories(${_include_dir})
endforeach(_include_dir ${POPPLER_INCLUDE_DIR})

include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

########### next target ###############

find_package(ZLIB REQUIRED)

include_directories(
  synctex
  ${ZLIB_INCLUDE_DIR}
)

set(okularGenerator_poppler_PART_SRCS
   generator_pdf.cpp
   formfields.cpp
   annots.cpp
   synctex/synctex_parser.c
   synctex/synctex_parser_utils.c
)

ki18n_wrap_ui(okularGenerator_poppler_PART_SRCS
   conf/pdfsettingswidget.ui
)

kconfig_add_kcfg_files(okularGenerator_poppler_PART_SRCS conf/pdfsettings.kcfgc )

add_library(okularGenerator_poppler MODULE ${okularGenerator_poppler_PART_SRCS})

target_link_libraries(okularGenerator_poppler ${POPPLER_LIBRARY} okularcore Qt5::Xml Qt5::Widgets Qt5::Gui ${ZLIB_LIBRARIES})

install(TARGETS okularGenerator_poppler DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_poppler.desktop okularPoppler.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_pdf.desktop active-documentviewer_pdf.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

