# common.make -- used by all Makefiles.
SHELL = /bin/sh
@SET_MAKE@
top_srcdir = @top_srcdir@
srcdir = @srcdir@
VPATH = @srcdir@

CC = @CC@
# CFLAGS is used for both compilation and linking.
CFLAGS = @CFLAGS@ $(XCFLAGS)

# Do not override CPPFLAGS; change XCPPFLAGS, CFLAGS, XCFLAGS, or DEFS instead.
CPPFLAGS = $(XCPPFLAGS) -I. -I$(srcdir) \
	   -I$(kpathsea_parent) -I$(kpathsea_srcdir_parent) \
	   $(prog_cflags) @CPPFLAGS@ $(DEFS) 
.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<
.SUFFIXES: .c .o

# Installation.
INSTALL = @INSTALL@
INSTALL_PROGRAM = @INSTALL_PROGRAM@
INSTALL_DATA = @INSTALL_DATA@

# This is used to recursively copy a fonts/ or tex/ directory to
# $(fontdir) or $(texinputdir).
# The first arg is `.', and the second is the target directory.
CP_R = cp -r

# This is so kpathsea will get remade automatically if you change
# something in it and recompile from the package directory.
kpathsea_parent = ..
kpathsea_dir = $(kpathsea_parent)/kpathsea
kpathsea_srcdir_parent = $(top_srcdir)/..
kpathsea_srcdir = $(kpathsea_srcdir_parent)/kpathsea
kpathsea = $(kpathsea_dir)/kpathsea.a

##ifeq ($(CC), gcc)
##XDEFS = -Wall -Wpointer-arith $(warn_more)
##CFLAGS = -g $(XCFLAGS)
##endif
# End of common.make.
