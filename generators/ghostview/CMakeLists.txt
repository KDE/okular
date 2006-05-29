add_subdirectory( conf ) 

include_directories(
   ${CMAKE_SOURCE_DIR}/okular
   ${CMAKE_CURRENT_BINARY_DIR}/../..
)


########### next target ###############

set(kpdflibgsasyncgenerator_SRCS kpdflibgsasyncgenerator.cpp )

kde4_automoc(${kpdflibgsasyncgenerator_SRCS})

kde4_add_executable(kpdflibgsasyncgenerator ${kpdflibgsasyncgenerator_SRCS})

target_link_libraries(kpdflibgsasyncgenerator  ${KDE4_KDECORE_LIBS} qgs )

install_targets(/bin kpdflibgsasyncgenerator )


########### next target ###############

set(okularGenerator_ghostview_PART_SRCS 
   interpreter_cmd.cpp 
   internaldocument.cpp 
   generator_ghostview.cpp 
   gvlogwindow.cpp 
   dscparse_adapter.cpp 
   ps.c 
   dscparse.cpp )

kde4_add_ui3_files(okularGenerator_ghostview_PART_SRCS
   conf/gssettingswidget.ui
)

kde4_add_kcfg_files(okularGenerator_ghostview_PART_SRCS conf/gssettings.kcfgc )

kde4_automoc(${okularGenerator_ghostview_PART_SRCS})

kde4_add_plugin(okularGenerator_ghostview WITH_PREFIX ${okularGenerator_ghostview_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_ghostview )

target_link_libraries(okularGenerator_ghostview  ${KDE4_KDECORE_LIBS} oKularcore qgs ${KDE4_KDEUI_LIBRARY} ${KDE4_KDEPRINT_LIBRARY} )

install_targets(${PLUGIN_INSTALL_DIR} okularGenerator_ghostview )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  libokularGenerator_ghostview.desktop )



#original Makefile.am contents follow:

#SUBDIRS = conf .
#
#INCLUDES = -I$(srcdir)/../.. -I$(top_builddir)/kpdf $(all_includes) 
#
#METASOURCES = AUTO
#KDE_CXXFLAGS=-Wall
#
#bin_PROGRAMS = kpdflibgsasyncgenerator
#kpdflibgsasyncgenerator_SOURCES = kpdflibgsasyncgenerator.cpp
#kpdflibgsasyncgenerator_LDFLAGS = $(QT_LDFLAGS) $(all_libraries) -L$(LIB_GS_PATH)
#kpdflibgsasyncgenerator_LDADD = -lqgs $(LIB_QT) $(LIB_KDECORE)
#kde_module_LTLIBRARIES = libokularGenerator_ghostview.la
#
#libokularGenerator_ghostview_la_LIBADD = $(top_builddir)/kpdf/generators/ghostview/conf/libgssettings.la \
#	$(top_builddir)/kpdf/core/liboKularcore.la $(top_builddir)/kpdf/conf/liboKularconf.la -lqgs $(LIB_KDEUI) $(LIB_KDEPRINT) 
#libokularGenerator_ghostview_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -L$(LIB_GS_PATH)
#libokularGenerator_ghostview_la_SOURCES = interpreter_cmd.cpp internaldocument.cpp generator_ghostview.cpp gvlogwindow.cpp dscparse_adapter.cpp ps.c dscparse.cpp
##libokularGenerator_ghostview_la_METASOURCES = AUTO
#
#KDE_OPTIONS = nofinal
#
#
#kde_services_DATA = libokularGenerator_ghostview.desktop
#
#
