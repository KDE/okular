
# check for Poppler::PSConverter::setOutputDevice()
set(CMAKE_REQUIRED_INCLUDES ${POPPLER_INCLUDE_DIR} ${QT_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${POPPLER_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY})
check_cxx_source_compiles("
#include <poppler-qt4.h>
int main()
{
  Poppler::Document * doc = Poppler::Document::load(\"foo\");
  Poppler::PSConverter * psconv = doc->psConverter();
  psconv->setOutputDevice( 0 );
  return 0;
}
" POPPLER_HAVE_PSCONVERTER_SETOUTPUTDEVICE)
macro_bool_to_01(POPPLER_HAVE_PSCONVERTER_SETOUTPUTDEVICE _POPPLER_HAVE_PSCONVERTER_SETOUTPUTDEVICE)


configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/config-okular-poppler.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/config-okular-poppler.h
)

include_directories(
   ${POPPLER_INCLUDE_DIR}
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


########### next target ###############

set(okularGenerator_poppler_PART_SRCS
   generator_pdf.cpp
)

if (HAVE_POPPLER_0_6)
   set(okularGenerator_poppler_PART_SRCS
      ${okularGenerator_poppler_PART_SRCS}
      formfields.cpp
   )
endif (HAVE_POPPLER_0_6)


kde4_add_plugin(okularGenerator_poppler ${okularGenerator_poppler_PART_SRCS})

target_link_libraries(okularGenerator_poppler   ${POPPLER_LIBRARY} okularcore ${KDE4_KIO_LIBS} m )

install(TARGETS okularGenerator_poppler DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_poppler.desktop okularPoppler.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_pdf.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

