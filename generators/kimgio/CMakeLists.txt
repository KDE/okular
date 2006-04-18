kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/okular )


########### next target ###############

set(okularGenerator_kimgio_PART_SRCS generator_kimgio.cpp )

kde4_automoc(${okularGenerator_kimgio_PART_SRCS})

kde4_add_plugin(okularGenerator_kimgio WITH_PREFIX ${okularGenerator_kimgio_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_kimgio )

target_link_libraries(okularGenerator_kimgio  ${KDE4_KDECORE_LIBS} oKularcore ${KDE4_KDEUI_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} okularGenerator_kimgio )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  libokularGenerator_kimgio.desktop )

kde4_footer()



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = \
#    libokularGenerator_kimgio.la
#
#INCLUDES = -I$(srcdir)/../.. -I../../ $(all_includes)
#
#libokularGenerator_kimgio_la_LIBADD = ../../core/liboKularcore.la \
#	../../conf/liboKularconf.la $(LIB_KDEUI)
#libokularGenerator_kimgio_la_LDFLAGS = -module -avoid-version  $(KDE_PLUGIN)  $(all_libraries)
#libokularGenerator_kimgio_la_SOURCES = generator_kimgio.cpp
#
#METASOURCES = AUTO 
#
#kde_services_DATA = libokularGenerator_kimgio.desktop
