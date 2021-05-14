remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_poppler")

add_subdirectory( conf )

set(CMAKE_REQUIRED_LIBRARIES Poppler::Qt5 Qt5::Core Qt5::Gui)

check_cxx_source_compiles("
#include <poppler-form.h>
int main()
{
    Poppler::FormFieldChoice *ffc = nullptr;
    ffc->choicesWithExportValues();
    return 0;
}
" HAVE_POPPLER_0_87)

check_cxx_source_compiles("
#include <poppler-qt5.h>
#include <poppler-form.h>
int main()
{
  auto f = &Poppler::PDFConverter::sign;
  auto g = &Poppler::setNSSDir;
}
" HAVE_POPPLER_SIGNING)

check_cxx_source_compiles("
#include <poppler-qt5.h>
#include <poppler-form.h>
int main()
{
    Poppler::PDFConverter::NewSignatureData pData;
    pData.setSignatureLeftText(QString());
}
" HAVE_POPPLER_FANCY_SIGNATURE)

check_cxx_source_compiles("
#include <poppler-qt5.h>
int main()
{
    Poppler::Document *doc = Poppler::Document::load(\"\", nullptr, nullptr);
    doc->xrefWasReconstructed();
    return 0;
}
" HAVE_POPPLER_RECONSTRUCTION_CALLBACK)

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/config-okular-poppler.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/config-okular-poppler.h
)

include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

########### next target ###############

set(okularGenerator_poppler_PART_SRCS
   generator_pdf.cpp
   formfields.cpp
   annots.cpp
   pdfsignatureutils.cpp
   pdfsettingswidget.cpp
)

ki18n_wrap_ui(okularGenerator_poppler_PART_SRCS
   conf/pdfsettingswidget.ui
)

kconfig_add_kcfg_files(okularGenerator_poppler_PART_SRCS GENERATE_MOC conf/pdfsettings.kcfgc )

okular_add_generator(okularGenerator_poppler ${okularGenerator_poppler_PART_SRCS})

target_link_libraries(okularGenerator_poppler okularcore KF5::I18n KF5::Completion KF5::KIOWidgets Poppler::Qt5 Qt5::Xml)

########### install files ###############
install( FILES okularPoppler.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_pdf.desktop org.kde.mobile.okular_pdf.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-poppler.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
