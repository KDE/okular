kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(srcdir)/../../ $(all_includes)
#
#libgeneratorkimgio_la_LDFLAGS = $(all_libraries)
#libgeneratorkimgio_la_SOURCES = generator_kimgio.cpp
#
#noinst_LTLIBRARIES = libgeneratorkimgio.la
