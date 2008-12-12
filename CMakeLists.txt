CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )


project(okular-mobi-generator)

ENABLE_TESTING()

find_package(KDE4 REQUIRED)
find_package(Strigi REQUIRED)
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${STRIGI_INCLUDE_DIR}
                   )

########### next target ###############

find_package(SharedMimeInfo REQUIRED)


set(okularGenerator_mobi_PART_SRCS
  converter.cpp
  mobipocket.cpp
  decompressor.cpp
  mobidocument.cpp
  generator_mobi.cpp
)

set(mobithumbnail_SRCS
  mobipocket.cpp
  decompressor.cpp
  mobithumbnail.cpp
)

set(mobianalyzer_SRCS
  mobipocket.cpp
  decompressor.cpp
  mobiendanalyzer.cpp
)

kde4_add_plugin(okularGenerator_mobi ${okularGenerator_mobi_PART_SRCS})
kde4_add_plugin(mobithumbnail ${mobithumbnail_SRCS})
kde4_add_library(mobi MODULE ${mobianalyzer_SRCS})

# FIXME: remove QtGui dependency on analyzer

target_link_libraries(mobi ${STRIGI_STREAMS_LIBRARY} ${STRIGI_STREAMANALYZER_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
target_link_libraries(okularGenerator_mobi okularcore ${mobi_LIBRARIES} ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY})
target_link_libraries(mobithumbnail  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${QT_QTGUI_LIBRARY} )

install(TARGETS mobithumbnail okularGenerator_mobi DESTINATION ${PLUGIN_INSTALL_DIR})

# install the module in the right directory so it is picked up
set_target_properties(mobi PROPERTIES PREFIX ${STRIGI_LINE_ANALYZER_PREFIX})


########### install files ###############

install( FILES mobithumbnail.desktop libokularGenerator_mobi.desktop okularMobi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_mobi.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install(TARGETS mobi LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strigi)

install(
  FILES x-mobipocket-ebook.xml
  DESTINATION ${XDG_MIME_INSTALL_DIR}
)
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
