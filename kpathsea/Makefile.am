AUTOMAKE_OPTIONS = foreign

noinst_LTLIBRARIES = libkpathsea.la

# Makefile for kpathsea --kb@mail.tug.org.  Public domain.
version = @KPSEVERSION@

# The root of the tree.
texmf = $(prefix)/texmf

# TeX, MF, and MP source files.
texinputdir = $(texmf)/tex
mfinputdir = $(texmf)/mf
mpinputdir = $(texmf)/mp

# MakeTeXPK.site, texmf.cnf, etc.
web2cdir = $(texmf)/web2c

# The top-level font directory.
fontdir = $(texmf)/fonts

# Memory dumps (.fmt, .base, and .mem).
fmtdir = $(texmf)/web2c
basedir = $(texmf)/web2c
memdir = $(texmf)/web2c

# Pool files.
texpooldir = $(texmf)/web2c
mfpooldir = $(texmf)/web2c
mppooldir = $(texmf)/web2c

# If install_fonts=true, the PostScript/LaserJet TFM and VF files for
# the builtin fonts get installed in subdirectories of this directory,
# named for the typeface families of these directories. If you don't
# have the default directory setup, you will want to set
# install_fonts=false.  Ditto for install_macros.
install_fonts = false
install_macros = false

# Where the .map files from fontname are installed.
fontnamedir = $(texmf)/fontname

# Where the dvips configuration files get installed, and where
# psfonts.map is.
dvipsdir = $(texmf)/dvips
psheaderdir = $(dvipsdir)

# MakeTeXPK will go here to create dc*.
dcfontdir = $(fontdir)/public/dc

# MakeTeXPK will go here if it exists to create nonstandard CM fonts,
# e.g., cmr11. See ftp.cs.umb.edu:pub/tex/sauter.tar.gz. The Sauter
# files must be in your regular MFINPUTS.
sauterdir = $(fontdir)/public/sauter

# If a font can't be found close enough to its stated size, we look for
# each of these sizes in the order given.  This colon-separated list is
# overridden by the envvar TEXSIZES, and by a program-specific variable
# (e.g., XDVISIZES), and perhaps by a config file (e.g., in dvips).
default_texsizes = 300:600

# End of paths.make.

# makevars.make -- the directory names we pass.
# It's important that none of these values contain [ @%], for the sake
# of kpathsea/texmf.sed.
makevars = prefix=$(prefix) exec_prefix=$(exec_prefix) \
  platform=$(platform) \
  bindir=$(bindir) scriptdir=$(scriptdir) libdir=$(libdir) \
  datadir=$(datadir) infodir=$(infodir) includedir=$(includedir) \
  manext=$(manext) mandir=$(mandir) \
  texmf=$(texmf) web2cdir=$(web2cdir) \
  texinputdir=$(texinputdir) mfinputdir=$(mfinputdir) \
  mpinputdir=$(mpinputdir) \
  fontdir=$(fontdir) \
  fmtdir=$(fmtdir) basedir=$(basedir) \
  memdir=$(memdir) \
  texpooldir=$(texpooldir) mfpooldir=$(mfpooldir) \
  mppooldir=$(mppooldir) \
  install_fonts=$(install_fonts) \
  dvipsdir=$(dvipsdir) psheaderdir=$(psheaderdir) \
  default_texsizes='$(default_texsizes)'
# End of makevars.make.

# Do not override CPPFLAGS; change XCPPFLAGS, CFLAGS, XCFLAGS, or DEFS instead.
INCLUDES = -I. -I$(srcdir) \
	   -I$(kpathsea_parent) -I$(kpathsea_srcdir_parent) \
	   $(prog_cflags) 

AM_DEFS = -DDEFAULT_FONT_SIZES='\"$(default_texsizes)\"'

# This is so kpathsea will get remade automatically if you change
# something in it and recompile from the package directory.
kpathsea_parent = ..
kpathsea_dir = $(kpathsea_parent)/kpathsea
kpathsea_srcdir_parent = $(top_srcdir)/..
kpathsea_srcdir = $(kpathsea_srcdir_parent)/kpathsea
kpathsea = $(kpathsea_dir)/kpathsea.a

$(srcdir)/tex-file.c: $(kpathsea_dir)/paths.h

libkpathsea_la_SOURCES = tex-file.c absolute.c atou.c cnf.c concat.c concat3.c \
concatn.c db.c debug.c dir.c elt-dirs.c expand.c extend-fname.c \
file-p.c find-suffix.c fn.c fontmap.c getopt.c getopt1.c hash.c \
kdefault.c line.c magstep.c make-suffix.c path-elt.c \
pathsearch.c proginit.c progname.c readable.c rm-suffix.c \
str-list.c str-llist.c tex-glyph.c tex-hush.c tex-make.c \
tilde.c truncate.c uppercasify.c variable.c version.c xbasename.c \
xcalloc.c xdirname.c xfopen.c xfseek.c xftell.c xgetcwd.c xmalloc.c \
xopendir.c xputenv.c xrealloc.c xstat.c xstrdup.c $(LTLIBOBJS)


# Make variable substitutions for paths.h.
texmf.cnf: texmf.in texmf.sed
	sed -f texmf.sed $(srcdir)/texmf.in >$@

# The idea is to turn each var=value into s%@var@%value%g. Seems simpler
# to put the substitutions in a file than to play shell quoting games.
texmf.sed: Makefile
	rm -f texmf.sed
	echo $(makevars) \
	| tr ' ' '\012' \
	| sed -e 's/^/s%@/' -e 's/=/@%/' -e 's/$$/%g/' \
	>$@
# Insert $TEXMF in as many of the hardwired paths as possible.  We could
# use the slightly different rule 's%$(texmf)/%\$$TEXMF/%g' and avoid
# the need for the corrections below.  But if texmf.in is edited
# the heuristic may no longer work.
	echo 's%$(texmf)%\$$TEXMF%g' >>$@
# Now we repair the damage this may have caused.  Don't replace the rhs of
# the TEXMFMAIN assignment itself, then we'd end up with TEXMFMAIN = $TEXMF
# and TEXMF = $TEXMFMAIN.  The (commented) assignment of TEXMFLOCAL is
# likely to have been mangled as well.
	echo '/^ *TEXMFMAIN[ =]/s%\$$TEXMF%$(texmf)%' >>$@
	echo '/^[% ]*TEXMFLOCAL[ =]/s%\$$TEXMF%$(texmf)%' >>$@
	echo '/^[% ]*VARTEXMF[ =]/s%\$$TEXMF%$(texmf)%' >>$@
	echo '/^[% ]*TEXMFCNF[ =]/s%\$$TEXMF%$(texmf)%' >>$@
# And fill in the last clause of TEXMFCNF with an absolute path.
	echo '/^ *TEXMFCNF[ =]/s%@web2c@%$(web2cdir)%' >>$@

# First null out comments and leading/trailing whitespace, then remove
# lines that define invalid C identifiers, then remove blank lines and
# lines that define lowercase values (those are never path values). Each
# line remaining looks like
# <name> = <value>
# (but = and surrounding spaces are optional, hence can't remove all spaces).
# Change this to #ifndef DEFAULT_name@#define DEFAULT_name "value"@#endif,
# then change the @'s to newlines (sed isn't good at multiline replacements).
# 
# No backslash-newline escapes in the long sed replacement because that
# will turn into a space in the output.
# 
# Without the $TEXMF/prefix/etc. substitutions, if the cnf file was
# not found, the compile-time paths would be of little use, since TEXMF
# (etc.) wouldn't be defined. Alternatively, we could have a way to
# specify compile-time default values for variables in general, but I
# think it's better to keep the last-resort paths as simple as possible.
# 
# The definition of DEFAULT_TEXMF (and other variables)
# that winds up in the final paths.h will not be used.
#
# We don't want to rewrite paths.h when we have only changed comments
# in texmf.in that have no effect on paths.h, since that would cause
# almost everything to be rebuilt.
#
# We also replace all semicolons with colons in the paths.  Ultimately
# the path-splitting code should be changed to understand both.
$(kpathsea_dir)/paths.h: stamp-paths
stamp-paths: texmf.cnf
	echo "/* paths.h: Generated from texmf.cnf. */" >paths.tmp
	sed -e 's/%.*//' -e 's/^[ 	]*//' -e 's/[ 	]*$$//' texmf.cnf \
	| grep '^[ 	]*[A-Z0-9_]*[ 	=]' \
	| sed '/^$$/d' \
	| sed 's/^\([^ 	=]*\)[ 	]*=*[ 	]*\(.*\)/#ifndef DEFAULT_\1@#define DEFAULT_\1 "\2"@#endif/' \
	| tr '@' '\012' \
	| sed -e 's%\$$TEXMFMAIN%$(texmf)%g' \
	      -e 's%\$$TEXMF%$(texmf)%g' \
	      -e 's%\$$VARTEXFONTS%$(vartexfonts)%g' \
	      -e 's%\$$web2cdir%$(web2cdir)%g' \
	      -e 's%\$$prefix%$(prefix)%g' \
	| sed -e 's%;%:%g' \
	>>paths.tmp
	@if cmp -s paths.h paths.tmp 2>/dev/null; then \
	  echo "paths.h is unchanged"; \
	else \
	  echo "cp paths.tmp paths.h"; \
	  cp paths.tmp paths.h; \
	fi
	rm -f paths.tmp
	date >stamp-paths

# Need an extra definition for this. Dependencies included below.
tex-file.o:
	$(KPATHSEA_CC)  $(srcdir)/tex-file.c

check: kpsewhich
	./kpsewhich -expand-var '$$TEXMF'

# autoheader input file, not an includable C header. Bad name.
$(library).h: always
	rm -f $@
	echo '#include <kpathsea/config.h>' >$@
	ls -1 *.h \
	  | grep -v '\(acconfig\|config\|kpathsea\|win32lib\)\.h' \
	  | sed -e 's,^,#include <kpathsea/,' -e s',$$,>,' >>$@
	touch -r `ls -1t *.h | tail +2 | head -1` $@ 
always:
.PHONY: always



