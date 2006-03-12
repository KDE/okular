kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(srcdir)/../.. -I$(top_builddir)/kpdf $(POPPLER_CFLAGS)  $(all_includes)
#
#libgeneratorpdf_la_LDFLAGS = $(all_libraries)
#libgeneratorpdf_la_SOURCES = generator_pdf.cpp gp_outputdev.cpp pagetransition.cpp
#
#noinst_LTLIBRARIES = libgeneratorpdf.la
#
#KDE_OPTIONS = nofinal
#
#generator_pdf.lo: ../../conf/settings.h
