add_subdirectory( kio-msits )

include_directories(
   ${CMAKE_SOURCE_DIR}/okular
   ${CMAKE_BINARY_DIR}/okular
   ${CHM_INCLUDE_DIR}
   ${CMAKE_SOURCE_DIR}/okular/generators/chm/lib
)


########### next target ###############

set(chmlib_SRCS
   ${CMAKE_SOURCE_DIR}/okular/generators/chm/lib/iconstorage.cpp
   ${CMAKE_SOURCE_DIR}/okular/generators/chm/lib/kchmtextencoding.cpp
   ${CMAKE_SOURCE_DIR}/okular/generators/chm/lib/kchmtreeviewitem.cpp
   ${CMAKE_SOURCE_DIR}/okular/generators/chm/lib/kchmurl.cpp
   ${CMAKE_SOURCE_DIR}/okular/generators/chm/lib/xchmfile.cpp
)

set(okularGenerator_chmlib_SRCS
   ${CMAKE_SOURCE_DIR}/okular/generators/chm/generator_chm.cpp
)

set(okularGenerator_chmlib_PART_SRCS ${chmlib_SRCS} ${okularGenerator_chmlib_SRCS} )

kde4_automoc(${okularGenerator_chmlib_PART_SRCS})

kde4_add_plugin(okularGenerator_chmlib WITH_PREFIX ${okularGenerator_chmlib_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_chmlib )

target_link_libraries(okularGenerator_chmlib  ${KDE4_KDECORE_LIBS} okularcore ${CHM_LIBRARY} ${KDE4_KDEPRINT_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KHTML_LIBS} )

install(TARGETS okularGenerator_chmlib DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_chmlib.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )



#original Makefile.am contents follow:

#SUBDIRS = lib . kio-msits 
#
##bin_PROGRAMS = helper
##helper_SOURCES = helper.cpp 
##helper_LDFLAGS = $(LIB_QT) 
##helper_LDADD = $(top_builddir)/kpdf/core/libkpdfcore.la $(top_builddir)/kpdf/conf/libkpdfconf.la lib/libchmfile.la
#
#kde_module_LTLIBRARIES = \
#  libokularGenerator_chmlib.la
#
#INCLUDES = -I$(srcdir)/../.. -I../.. $(all_includes)
#
#libokularGenerator_chmlib_la_LIBADD = ../../core/libokularcore.la \
#	../../conf/libokularconf.la \
#	lib/libchmfile.la \
#	$(LIB_KDEPRINT) $(LIB_KDEUI) $(LIB_KHTML) -lchm
#libokularGenerator_chmlib_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#libokularGenerator_chmlib_la_SOURCES = generator_t.cpp
#METASOURCES = AUTO
#KDE_OPTIONS = nofinal
#
#kde_services_DATA = libokularGenerator_chmlib.desktop
#
