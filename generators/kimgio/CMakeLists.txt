include_directories(
   ${CMAKE_SOURCE_DIR}/okular
)


########### next target ###############

set(okularGenerator_kimgio_PART_SRCS generator_kimgio.cpp )

kde4_automoc(${okularGenerator_kimgio_PART_SRCS})

kde4_add_plugin(okularGenerator_kimgio WITH_PREFIX ${okularGenerator_kimgio_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_kimgio )

target_link_libraries(okularGenerator_kimgio  ${KDE4_KDECORE_LIBS} okularcore ${KDE4_KDEUI_LIBS} )

install(TARGETS okularGenerator_kimgio DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_kimgio.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = \
#    libokularGenerator_kimgio.la
#
#INCLUDES = -I$(srcdir)/../.. -I../../ $(all_includes)
#
#libokularGenerator_kimgio_la_LIBADD = ../../core/libokularcore.la \
#	../../conf/libokularconf.la $(LIB_KDEUI)
#libokularGenerator_kimgio_la_LDFLAGS = -module -avoid-version  $(KDE_PLUGIN)  $(all_libraries)
#libokularGenerator_kimgio_la_SOURCES = generator_kimgio.cpp
#
#METASOURCES = AUTO 
#
#kde_services_DATA = libokularGenerator_kimgio.desktop
