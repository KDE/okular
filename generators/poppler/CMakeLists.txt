remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_poppler")

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

set(CMAKE_REQUIRED_LIBRARIES Poppler::Qt5 Qt5::Core Qt5::Gui)

check_cxx_source_compiles("
#include <poppler-qt5.h>
int main()
{
  Poppler::LinkOCGState *l = 0;
  return 0;
}
" HAVE_POPPLER_0_50)

check_cxx_source_compiles("
#include <poppler-qt5.h>
#include <poppler-form.h>
int main()
{
  Poppler::FormFieldButton *ff = 0;
  Poppler::Link *l = ff->additionalAction(Poppler::FormField::CalculateField);
  return 0;
}
" HAVE_POPPLER_0_53)

check_cxx_source_compiles("
#include <poppler-qt5.h>
int main()
{
  Poppler::Document::RenderHint hint = Poppler::Document::HideAnnotations;
  return 0;
}
" HAVE_POPPLER_0_60)

check_cxx_source_compiles("
#include <poppler-qt5.h>
#include <QImage>
int main()
{
  Poppler::Page *p;
  p->renderToImage(0, 0, 0, 0, 0, 0, Poppler::Page::Rotate0, nullptr, nullptr, QVariant());
  return 0;
}
" HAVE_POPPLER_0_62)

check_cxx_source_compiles("
#include <poppler-qt5.h>
#include <QImage>
int main()
{
  Poppler::Page *p;
  p->renderToImage(0, 0, 0, 0, 0, 0, Poppler::Page::Rotate0, nullptr, nullptr, nullptr, QVariant());
  return 0;
}
" HAVE_POPPLER_0_63)

check_cxx_source_compiles("
#include <poppler-qt5.h>
#include <poppler-form.h>
#include <poppler-link.h>
int main()
{
  Poppler::FormField *f;
  f->setReadOnly(true);
  f->setVisible(true);
  Poppler::Link *l;
  l->nextLinks();
}
" HAVE_POPPLER_0_64)

check_cxx_source_compiles("
#include <poppler-qt5.h>
#include <poppler-form.h>
int main()
{
  Poppler::FormField *f;
  f->additionalAction(Poppler::Annotation::CursorEnteringAction);
}
" HAVE_POPPLER_0_65)

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
   pdfsignatureinfo.cpp
)

ki18n_wrap_ui(okularGenerator_poppler_PART_SRCS
   conf/pdfsettingswidget.ui
)

kconfig_add_kcfg_files(okularGenerator_poppler_PART_SRCS conf/pdfsettings.kcfgc )

okular_add_generator(okularGenerator_poppler ${okularGenerator_poppler_PART_SRCS})

target_link_libraries(okularGenerator_poppler okularcore KF5::I18n KF5::Completion Poppler::Qt5 Qt5::Xml)

########### install files ###############
install( FILES okularPoppler.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_pdf.desktop org.kde.mobile.okular_pdf.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-poppler.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
