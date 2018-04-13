set(CMAKE_AUTORCC ON)
add_executable(okularkirigami main.cpp app.qrc)
target_link_libraries(okularkirigami Qt5::Widgets Qt5::Qml KF5::I18n)
install(TARGETS okularkirigami ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES package/metadata.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} RENAME org.kde.mobile.okular.desktop)

