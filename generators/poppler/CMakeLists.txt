
set(CMAKE_REQUIRED_INCLUDES ${POPPLER_INCLUDE_DIR} ${QT_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${POPPLER_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY})
check_cxx_source_compiles("
#include <poppler-qt4.h>
int main()
{
  Poppler::Document * doc = 0;
  (void)doc->scripts();
  return 0;
}
" HAVE_POPPLER_0_9)
check_cxx_source_compiles("
#include <poppler-qt4.h>
int main()
{
  Poppler::LinkDestination * dest = 0;
  const QString foo = dest->destinationName();
  return 0;
}
" HAVE_POPPLER_0_11)

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

kde4_add_plugin(okularGenerator_poppler ${okularGenerator_poppler_PART_SRCS})

target_link_libraries(okularGenerator_poppler ${POPPLER_LIBRARY} okularcore ${KDE4_KDEUI_LIBS} ${QT_QTXML_LIBRARY} ${ZLIB_LIBRARIES})

install(TARGETS okularGenerator_poppler DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_poppler.desktop okularPoppler.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_pdf.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

