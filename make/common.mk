# common.mk -- used by all Makefiles.
SHELL = /bin/sh
@SET_MAKE@
top_srcdir = @top_srcdir@
srcdir = @srcdir@
VPATH = @srcdir@

CC = @CC@
CFLAGS = @CFLAGS@ $(XCFLAGS)
CPPFLAGS = @CPPFLAGS@ $(XCPPFLAGS)
DEFS = @DEFS@ $(XDEFS)

# Kpathsea needs this for compiling, programs need it for linking.
LIBTOOL = $(kpathsea_srcdir)/klibtool

# You can change [X]CPPFLAGS, [X]CFLAGS, or [X]DEFS, but
# please don't change ALL_CPPFLAGS or ALL_CFLAGS.
# prog_cflags is set by subdirectories of web2c.
ALL_CPPFLAGS = $(DEFS) -I. -I$(srcdir) \
  -I$(kpathsea_parent) -I$(kpathsea_srcdir_parent) \
  $(prog_cflags) $(CPPFLAGS)
ALL_CFLAGS = $(ALL_CPPFLAGS) $(CFLAGS) -c
compile = $(CC) $(ALL_CFLAGS)

.SUFFIXES:
.SUFFIXES: .c .o # in case the suffix list has been cleared, e.g., by web2c
.c.o:
	$(compile) $<

# Installation.
INSTALL = @INSTALL@
INSTALL_PROGRAM = @INSTALL_PROGRAM@
INSTALL_SCRIPT = $(INSTALL_PROGRAM)
INSTALL_DATA = @INSTALL_DATA@
INSTALL_LIBTOOL_LIBS = INSTALL_DATA='$(INSTALL_DATA)' $(LIBTOOL) install-lib
INSTALL_LIBTOOL_PROG = INSTALL_PROGRAM='$(INSTALL_PROGRAM)' $(LIBTOOL) install-prog

# Creating (symbolic) links.
LN = @LN_S@

# We use these for many things.
kpathsea_parent = ..
kpathsea_dir = $(kpathsea_parent)/kpathsea
kpathsea_srcdir_parent = $(top_srcdir)/..
kpathsea_srcdir = $(kpathsea_srcdir_parent)/kpathsea
kpathsea = $(kpathsea_dir)/libkpathsea.la

@MAINT@ifeq ($(CC), gcc)
@MAINT@XDEFS = -Wpointer-arith $(warn_more)
@MAINT@CFLAGS = -pipe -g $(XCFLAGS)
@MAINT@endif
# End of common.mk.
