
set(active-documentviewer_SRCS
    main.cpp
)

add_executable(active-documentviewer ${active-documentviewer_SRCS})

find_package(ActiveApp REQUIRED)

include_directories(  ${QT_INCLUDES} ${ACTIVEAPP_INCLUDE_DIR})

target_link_libraries(active-documentviewer
    Qt5::Declarative
    Qt5::OpenGL
    
    ${KDE4_PLASMA_LIBS}
    ${ACTIVEAPP_LIBRARIES}
)

install(TARGETS active-documentviewer ${INSTALL_TARGETS_DEFAULT_ARGS} )

