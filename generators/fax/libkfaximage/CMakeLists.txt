
########### next target ###############

set(kfaximage_LIB_SRCS kfaximage.cpp faxexpand.cpp faxinit.cpp )

kde4_automoc(${kfaximage_LIB_SRCS})

kde4_add_library(kfaximage SHARED ${kfaximage_LIB_SRCS})

target_link_libraries(kfaximage  ${KDE4_KDECORE_LIBS} )

set_target_properties(kfaximage PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS kfaximage DESTINATION ${LIB_INSTALL_DIR} )


########### install files ###############

install(FILES kfaximage.h DESTINATION ${INCLUDE_INSTALL_DIR})

