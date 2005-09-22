INCLUDES = -I$(top_srcdir) $(all_includes)

lib_LTLIBRARIES = libkfaximage.la
libkfaximage_la_LDFLAGS = $(all_libraries) -no-undefined -avoid-version
libkfaximage_la_LIBADD  = $(LIB_KDECORE)
libkfaximage_la_SOURCES = kfaximage.cpp faxexpand.cpp faxinit.cpp

include_HEADERS = kfaximage.h
noinst_HEADERS = faxexpand.h

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) $(libkfaximage_la_SOURCES) -o $(podir)/libkfaximgage.pot
