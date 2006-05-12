
add_subdirectory( generator_pdf ) 
add_subdirectory( generator_kimgio ) 




########### next target ###############


########### install files ###############





#original Makefile.am contents follow:

#SUBDIRS = generator_pdf generator_kimgio
#
#INCLUDES = -I$(srcdir)/generator_pdf -I$(srcdir)/.. -I$(top_builddir)/kpdf $(POPPLER_CFLAGS) $(all_includes)
#
#METASOURCES = AUTO
#
#libkpdfcore_la_LIBADD = ./generator_pdf/libgeneratorpdf.la ./generator_kimgio/libgeneratorkimgio.la
#libkpdfcore_la_SOURCES = document.cpp link.cpp page.cpp pagetransition.cpp
#
#noinst_LTLIBRARIES = libkpdfcore.la
#
#document.lo: ../conf/settings.h
#page.lo: ../conf/settings.h
