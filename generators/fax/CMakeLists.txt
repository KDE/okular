kde4_header()

add_subdirectory( libkfaximage )

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/okular ${CMAKE_BINARY_DIR}/okular ${CMAKE_SOURCE_DIR}/okular/generators/fax/libkfaximage )


########### next target ###############

set(okularGenerator_fax_PART_SRCS faxrenderer.cpp )

kde4_automoc(${okularGenerator_fax_PART_SRCS})

kde4_add_plugin(okularGenerator_fax WITH_PREFIX ${okularGenerator_fax_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_fax )

target_link_libraries(okularGenerator_fax  ${KDE4_KDECORE_LIBS} oKularcore ${KDE4_KDEUI_LIBS} kfaximage )

install_targets(${LIB_INSTALL_DIR}/kde4 okularGenerator_fax )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  libokularGenerator_fax.desktop )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(srcdir)/../../ -I$(srcdir)/libkfaximage $(all_includes) 
#
#SUBDIRS = libkfaximage .
#
#kde_module_LTLIBRARIES = \
#    libokularGenerator_fax.la
#
#libokularGenerator_fax_la_LIBADD = ../../core/liboKularcore.la \
#	../../conf/liboKularconf.la $(LIB_KDEUI) \
#	libkfaximage/libkfaximage.la
#
#libokularGenerator_fax_la_LDFLAGS = -module -avoid-version  $(KDE_PLUGIN)  $(all_libraries)
#libokularGenerator_fax_la_SOURCES = faxrenderer.cpp
#
#METASOURCES = AUTO 
#
#kde_services_DATA = libokularGenerator_fax.desktop
#
#KDE_ICON = kfaxview
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kfaxview.pot
