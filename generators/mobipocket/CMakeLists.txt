find_package(KDE4 REQUIRED)
find_package(Okular REQUIRED)
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES}
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${OKULAR_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/../libs/mobipocket
                   )

########### next target ###############

set(okularGenerator_mobi_PART_SRCS
  ${PROJECT_SOURCE_DIR}/../libs/mobipocket/mobipocket.cpp
  ${PROJECT_SOURCE_DIR}/../libs/mobipocket/decompressor.cpp
  mobidocument.cpp
  generator_mobi.cpp
  converter.cpp
)

kde4_add_plugin(okularGenerator_mobi ${okularGenerator_mobi_PART_SRCS})

target_link_libraries(okularGenerator_mobi ${OKULAR_LIBRARIES} ${mobi_LIBRARIES} ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY})

install(TARGETS okularGenerator_mobi DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES libokularGenerator_mobi.desktop okularMobi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_mobi.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
