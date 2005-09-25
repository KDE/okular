INCLUDES = -I$(top_srcdir)/kpdf/generators/fax/libkfaximage \
	-I$(top_srcdir) $(all_includes) -I$(top_srcdir)/kpdf

SUBDIRS = libkfaximage .

kde_module_LTLIBRARIES = \
    libokularGenerator_fax.la

#INCLUDES = -I$(srcdir)/../../ $(all_includes)

libokularGenerator_fax_la_LIBADD = $(top_builddir)/kpdf/core/libkpdfcore.la \
	$(top_builddir)/kpdf/conf/libkpdfconf.la $(LIB_KDEUI) libkfaximage/libkfaximage.la
libokularGenerator_fax_la_LDFLAGS = -module -avoid-version  $(KDE_PLUGIN)  $(all_libraries)
libokularGenerator_fax_la_SOURCES = faxrenderer.cpp

METASOURCES = AUTO 

kde_services_DATA = libokularGenerator_fax.desktop

KDE_ICON = kfaxview

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kfaxview.pot
