kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(kio_msits_PART_SRCS msits.cpp )

kde4_automoc(${kio_msits_PART_SRCS})

kde4_add_plugin(kio_msits ${kio_msits_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_msits )

target_link_libraries(kio_msits  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} chm )

install_targets(${LIB_INSTALL_DIR}/kde4 kio_msits )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  msits.protocol )

kde4_footer()



#original Makefile.am contents follow:

#METASOURCES = AUTO
#INCLUDES = $(QT_INCLUDES) -I$(CHM_INCLUDES) $(KDE_INCLUDES) $(all_includes)
#
#kde_module_LTLIBRARIES = kio_msits.la
#
#kio_msits_la_SOURCES = msits.cpp
#kio_msits_la_LIBADD = $(LIB_KIO) -lchm
#kio_msits_la_LDFLAGS = -module -avoid-version $(QT_LIBS) $(KDE_LDFLAGS) $(KDE_LIBS) $(KDE_PLUGIN)
#
#protocol_DATA = msits.protocol
#protocoldir = $(kde_servicesdir)
#
#
