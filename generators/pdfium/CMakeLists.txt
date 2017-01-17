include_directories(${PDFIUM_INCLUDE_DIR})

########### next target ###############

set(okularGenerator_pdfium_SRCS
  generator_pdfium.cpp
)

okular_add_generator(okularGenerator_pdfium ${okularGenerator_pdfium_SRCS})
target_link_libraries(okularGenerator_pdfium okularcore -Wl,--start-group ${PDFIUM_LIBRARIES_STATIC} -Wl,--end-group ${PDFIUM_LIBRARIES_SHARED_v8_libbase} ${PDFIUM_LIBRARIES_SHARED_v8_libplatform} ${PDFIUM_LIBRARIES_SHARED_v8} KF5::I18n Qt5::Widgets Qt5::Xml)


########### install files ###############
install( FILES okularPDFium.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_pdf.desktop org.kde.mobile.okular_pdf.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-pdfium.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
