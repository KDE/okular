CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )


project(okular-mobi-generator)

ENABLE_TESTING()

find_package(KDE4 REQUIRED)
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES}
                    ${CMAKE_CURRENT_SOURCE_DIR}
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

kde4_add_plugin(okularGenerator_mobi ${okularGenerator_mobi_PART_SRCS})
kde4_add_plugin(mobithumbnail ${mobithumbnail_SRCS})

target_link_libraries(okularGenerator_mobi okularcore ${mobi_LIBRARIES} ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY})
target_link_libraries(mobithumbnail  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${QT_QTGUI_LIBRARY} )

install(TARGETS mobithumbnail okularGenerator_mobi DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES mobithumbnail.desktop libokularGenerator_mobi.desktop okularMobi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_mobi.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

install(
  FILES x-mobipocket.xml
  DESTINATION ${XDG_MIME_INSTALL_DIR}
)
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
