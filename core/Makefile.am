SUBDIRS = generator_pdf

INCLUDES = -I$(srcdir)/generator_pdf -I$(srcdir)/.. -I$(srcdir)/../xpdf -I$(srcdir)/../xpdf/goo $(all_includes)

METASOURCES = AUTO

libkpdfcore_la_LIBADD = ./generator_pdf/libgeneratorpdf.la
libkpdfcore_la_SOURCES = document.cpp link.cpp page.cpp

noinst_LTLIBRARIES = libkpdfcore.la
