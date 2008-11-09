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
  mobidocument.cpp
  generator_mobi.cpp
)

kde4_add_plugin(okularGenerator_mobi ${okularGenerator_mobi_PART_SRCS})

target_link_libraries(okularGenerator_mobi okularcore ${mobi_LIBRARIES} ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY})

install(TARGETS okularGenerator_mobi DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_mobi.desktop okularMobi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_mobi.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

install(
  FILES x-mobipocket.xml
  DESTINATION ${XDG_MIME_INSTALL_DIR}
)
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
