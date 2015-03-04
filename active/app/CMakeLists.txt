
include_directories(  ${QT_INCLUDES} )

add_subdirectory(src)

# QML-only thing
install(DIRECTORY package/ DESTINATION ${KDE_INSTALL_DATADIR}/plasma/packages/org.kde.active.documentviewer)


install(FILES active-documentviewer.desktop DESTINATION ${KDE_INSTALL_APPDIR})

