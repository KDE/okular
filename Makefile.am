SUBDIRS = . goo xpdf kpdf

COMPILE_BEFORE_xpdf = goo
COMPILE_BEFORE_kpdf = goo xpdf

noinst_HEADERS = aconf.h

aconf.h:
	ln -s ../config.h aconf.h
