INCLUDES = $(all_includes)

libgoo_la_LDFLAGS = $(all_libraries) -version-info 5:0:1 -no-undefined
libgoo_la_SOURCES = GHash.cc GList.cc GString.cc gfile.cc gmem.c gmempp.cc \
    parseargs.c
libgoo_la_DEPENDENCIES = aconf.h

noinst_LTLIBRARIES = libgoo.la
noinst_HEADERS     = GHash.h GList.h GString.h gfile.h gmem.h  gtypes.h \
    parseargs.h vms_dirent.h vms_sys_dirent.h vms_unix_time.h

METASOURCES = AUTO

aconf.h:
	ln -s $(top_builddir)/config.h aconf.h
