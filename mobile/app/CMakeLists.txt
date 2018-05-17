set(CMAKE_AUTORCC ON)
add_executable(okularkirigami main.cpp app.qrc)
target_link_libraries(okularkirigami Qt5::Widgets Qt5::Qml KF5::I18n)
if (ANDROID)
    find_package(Qt5 COMPONENTS AndroidExtras)
    target_sources(okularkirigami PRIVATE android.cpp)
    target_link_libraries(okularkirigami Qt5::AndroidExtras)
endif()

install(TARGETS okularkirigami ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES package/metadata.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} RENAME org.kde.okular.kirigami.desktop)
install( FILES org.kde.okular.kirigami.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR} )
