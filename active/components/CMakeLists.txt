project(okular)

add_definitions(${QT_DEFINITIONS})
include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   
   ${QIMAGEBLITZ_INCLUDES}
)

set(okular_SRCS
    okularplugin.cpp
    ${CMAKE_SOURCE_DIR}/ui/guiutils.cpp
    ${CMAKE_SOURCE_DIR}/ui/tocmodel.cpp
    pageitem.cpp
    documentitem.cpp
    thumbnailitem.cpp
    )

add_library(okularplugin SHARED ${okular_SRCS})
target_link_libraries(okularplugin
        Qt5::Quick
        Qt5::Qml
        Qt5::Core
        Qt5::Xml
        Qt5::Svg
        okularcore
        okularpart
        )

install(TARGETS okularplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/okular)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/okular)

#add_subdirectory(test)
