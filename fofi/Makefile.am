INCLUDES = -I.. -I$(srcdir)/../goo $(all_includes)

libfofi_la_LDFLAGS = $(all_libraries)
libfofi_la_SOURCES = FoFiBase.cc FoFiEncodings.cc FoFiTrueType.cc \
    FoFiType1.cc FoFiType1C.cc
libfofi_la_COMPILE_FIRST = ../aconf.h

METASOURCES = AUTO

noinst_LTLIBRARIES = libfofi.la