set(okular_SRCS
    okularplugin.cpp
    ${CMAKE_SOURCE_DIR}/gui/guiutils.cpp
    ${CMAKE_SOURCE_DIR}/gui/tocmodel.cpp
    ${CMAKE_SOURCE_DIR}/gui/signaturemodel.cpp
    ${CMAKE_SOURCE_DIR}/gui/signatureguiutils.cpp
    ${CMAKE_SOURCE_DIR}/gui/certificatemodel.cpp
    ${CMAKE_SOURCE_DIR}/gui/pagepainter.cpp
    ${CMAKE_SOURCE_DIR}/gui/debug_ui.cpp
    pageitem.cpp
    documentitem.cpp
    thumbnailitem.cpp
    okularsingleton.cpp
    )

kconfig_add_kcfg_files(okular_SRCS GENERATE_MOC ${CMAKE_SOURCE_DIR}/conf/settings_mobile.kcfgc)

add_library(okularplugin SHARED ${okular_SRCS})
set_target_properties(okularplugin PROPERTIES COMPILE_DEFINITIONS "okularpart_EXPORTS")
target_link_libraries(okularplugin
        Qt5::Quick
        Qt5::Qml
        Qt5::Core
        Qt5::Xml
        Qt5::Svg
        KF5::Bookmarks
        KF5::I18n
        KF5::Service
        okularcore
        )

if(ANDROID)
    target_link_libraries(okularplugin Qt5::AndroidExtras)
endif()

install(TARGETS okularplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/okular)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/okular)
install(FILES DocumentView.qml DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/okular)
install(FILES private/PageView.qml DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/okular/private)

#add_subdirectory(test)
