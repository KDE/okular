
# check for Poppler::FormFieldButton as "0.7 check"
set(CMAKE_REQUIRED_INCLUDES ${POPPLER_INCLUDE_DIR} ${QT_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${POPPLER_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY})
check_cxx_source_compiles("
#include <poppler-qt4.h>
#include <poppler-form.h>
int main()
{
  Poppler::FormFieldButton * button = 0;
  button->buttonType();
  return 0;
}
" HAVE_POPPLER_0_7)

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

