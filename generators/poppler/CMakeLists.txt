include_directories(
   ${POPPLER_INCLUDE_DIR}/poppler
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_SOURCE_DIR}/okular
)


########### next target ###############

set(okularGenerator_poppler_PART_SRCS generator_pdf.cpp gp_outputdev.cpp pagetransition.cpp )

kde4_automoc(${okularGenerator_poppler_PART_SRCS})

kde4_add_plugin(okularGenerator_poppler WITH_PREFIX ${okularGenerator_poppler_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_poppler )

target_link_libraries(okularGenerator_poppler   ${POPPLER_LIBRARY} ${KDE4_KDECORE_LIBS} oKularcore ${KDE4_KDEPRINT_LIBS} ${KDE4_KDEUI_LIBS} m )

install_targets(${PLUGIN_INSTALL_DIR} okularGenerator_poppler )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  libokularGenerator_poppler.desktop )



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = \
#    libokularGenerator_poppler.la
#
#INCLUDES = -I$(srcdir)/../.. -I../.. $(POPPLER_CFLAGS) $(all_includes) 
#
#libokularGenerator_poppler_la_LIBADD = ../../core/liboKularcore.la ../../conf/liboKularconf.la \
#				    $(POPPLER_LIBS) $(LIB_KDEPRINT) $(LIB_KDEUI) -lm
#libokularGenerator_poppler_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#libokularGenerator_poppler_la_SOURCES = generator_pdf.cpp gp_outputdev.cpp pagetransition.cpp
#
#KDE_OPTIONS = nofinal
#
#METASOURCES = AUTO
#
#kde_services_DATA = libokularGenerator_poppler.desktop
#
#generator_pdf.lo: ../../conf/settings.h
