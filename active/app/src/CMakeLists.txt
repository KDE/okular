
set(active-documentviewer_SRCS
    main.cpp
)

kde4_add_executable(active-documentviewer ${active-documentviewer_SRCS})

find_package(ActiveApp REQUIRED)

include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} ${ACTIVEAPP_INCLUDE_DIR})

target_link_libraries(active-documentviewer
    ${QT_QTDECLARATIVE_LIBRARY}
    ${QT_QTOPENGL_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_PLASMA_LIBS}
    ${ACTIVEAPP_LIBRARIES}
)

install(TARGETS active-documentviewer ${INSTALL_TARGETS_DEFAULT_ARGS} )

