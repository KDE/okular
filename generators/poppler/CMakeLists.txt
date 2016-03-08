add_subdirectory( conf )

if (Poppler_VERSION VERSION_GREATER "0.23.99")
  set (HAVE_POPPLER_0_24 1)
endif()

if (Poppler_VERSION VERSION_GREATER "0.27.99")
  set (HAVE_POPPLER_0_28 1)
endif()

if (Poppler_VERSION VERSION_GREATER "0.35.99")
  set (HAVE_POPPLER_0_36 1)
endif()

if (Poppler_VERSION VERSION_GREATER "0.36.99")
  set (HAVE_POPPLER_0_37 1)
endif()

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/config-okular-poppler.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/config-okular-poppler.h
)

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

target_link_libraries(okularGenerator_poppler okularcore KF5::I18n KF5::Completion Poppler::Qt5 Qt5::Xml)

########### install files ###############

install( PROGRAMS okularApplication_pdf.desktop org.kde.mobile.okular_pdf.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

