project(okular)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   ${QIMAGEBLITZ_INCLUDES}
)

set(okular_SRCS
    okularplugin.cpp
    ${CMAKE_SOURCE_DIR}/ui/pagepainter.cpp
    ${CMAKE_SOURCE_DIR}/ui/guiutils.cpp
    ${CMAKE_SOURCE_DIR}/ui/tocmodel.cpp
    pageitem.cpp
    documentitem.cpp
    thumbnailitem.cpp
    )
    
kde4_add_kcfg_files(okular_SRCS ${CMAKE_SOURCE_DIR}/conf/settings.kcfgc )

qt4_automoc(${okular_SRCS})

kde4_add_library(okularplugin SHARED ${okular_SRCS})
target_link_libraries(okularplugin
        ${QT_QTCORE_LIBRARY}
        ${QT_QTGUI_LIBRARY}
        ${QT_QTDECLARATIVE_LIBRARY}
        ${QT_QTXML_LIBRARY}
        ${KDE4_KDECORE_LIBRARY}
        ${KDE4_KDEUI_LIBRARY}
        ${KDE4_KIO_LIBRARY}
        ${QIMAGEBLITZ_LIBRARIES}
        okularcore
        )

install(TARGETS okularplugin DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/okular)
install(FILES qmldir DESTINATION ${IMPORTS_INSTALL_DIR}/org/kde/okular)

#add_subdirectory(test)
