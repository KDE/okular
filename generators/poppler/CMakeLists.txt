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

set(okularGenerator_poppler_PART_SRCS
   generator_pdf.cpp
   formfields.cpp
   annots.cpp
)

ki18n_wrap_ui(okularGenerator_poppler_PART_SRCS
   conf/pdfsettingswidget.ui
)

kconfig_add_kcfg_files(okularGenerator_poppler_PART_SRCS conf/pdfsettings.kcfgc )

okular_add_generator(okularGenerator_poppler ${okularGenerator_poppler_PART_SRCS})

target_link_libraries(okularGenerator_poppler ${POPPLER_LIBRARY} okularcore KF5::I18n Qt5::Xml KF5::Completion)

########### install files ###############

install( PROGRAMS okularApplication_pdf.desktop org.kde.mobile.okular_pdf.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

