INCLUDES = libkfaximage $(all_includes) 

SUBDIRS = libkfaximage .

kde_module_LTLIBRARIES = \
    libokularGenerator_fax.la

#INCLUDES = -I$(srcdir)/../../ $(all_includes)

libokularGenerator_fax_la_LIBADD = ../../core/liboKularcore.la \
	../../conf/liboKularconf.la $(LIB_KDEUI) \
	libkfaximage/libkfaximage.la

libokularGenerator_fax_la_LDFLAGS = -module -avoid-version  $(KDE_PLUGIN)  $(all_libraries)
libokularGenerator_fax_la_SOURCES = faxrenderer.cpp

METASOURCES = AUTO 

kde_services_DATA = libokularGenerator_fax.desktop

KDE_ICON = kfaxview

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kfaxview.pot
