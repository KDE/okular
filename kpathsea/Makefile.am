
INCLUDES = -I$(srcdir)/.. -I..

noinst_LTLIBRARIES = libkpathsea.la

libkpathsea_la_SOURCES = tex-file.c absolute.c basename.c cnf.c concat.c concat3.c      \
concatn.c db.c debug.c dir.c elt-dirs.c expand.c extend-fname.c file-p.c \
find-suffix.c fn.c fontmap.c getopt.c getopt1.c hash.c itoa.c kdefault.c \
line.c magstep.c make-suffix.c path-elt.c pathsearch.c proginit.c        \
progname.c putenv.c readable.c rm-suffix.c str-list.c str-llist.c        \
tex-glyph.c tex-make.c tilde.c truncate.c uppercasify.c variable.c       \
version.c xcalloc.c xfopen.c xmalloc.c xopendir.c xputenv.c xrealloc.c   \
xstat.c xstrdup.c selfdir.c dirname.c

AUTOMAKE_OPTIONS = foreign

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

# Besides the straight variable substitutions, also change all
# occurrences of `$(texmf)/' to `$TEXMF', so simply setting that envvar
# changes the root of the whole tree.
texmf.cnf: texmf.cnf.in texmf.sed
	sed -f texmf.sed -e 's%$(texmf)/%$$TEXMF/%g' $(srcdir)/texmf.cnf.in >$@

# The idea is to turn each var=value into s%@var@%value%g. Seems simpler
# to put the substitutions in a file than to play shell games.
texmf.sed: $(top_srcdir)/../make/makevars.make
	echo $(makevars) \
	| tr ' ' '\012' \
	| sed -e 's/^/s%@/' -e 's/=/@%/' -e 's/$$/%/' -e 's/$$/g/' \
	>$@

# First null out comments and leading/trailing whitespace, then remove
# lines that define invalid C identifiers, then remove blank lines. Each
# line remaining looks like
# <name> = <value>
# (but = and surrounding spaces are optional, hence can't remove all spaces).
# Change this to #ifndef DEFAULT_name@#define DEFAULT_name "value"@#endif,
# then change the @'s to newlines (sed isn't good at multiline replacements),
# then change $TEXMF back to $(texmf)
# 
# No backslash-newline escapes in the long sed replacement because that
# will turn into a space in the output.
# 
# Without the $TEXMF substitution, if the cnf file was not found for
# some reason, none of the compile-time paths would be of any use, since
# TEXMF wouldn't be defined. Alternatively, we could have a way to
# specify compile-time default values for variables in general, but I
# think it's better to keep the last-resort paths as simple as possible.
# 
# The definition of DEFAULT_TEXMF (and possibly other variables)
# that winds up in the final paths.h will not be used.
paths.h: texmf.cnf
	echo "/* paths.h: Generated from texmf.cnf `date`.  */" >$@
	sed -e 's/%.*//' -e 's/^[ 	]*//' -e 's/[ 	]*$$//' texmf.cnf \
	| grep '^[ 	]*[A-Za-z_]*[ 	=]' \
	| sed '/^$$/d' \
	| sed 's/^\([^ 	=]*\)[ 	]*=*[ 	]*\(.*\)/#ifndef DEFAULT_\1@#define DEFAULT_\1 "\2"@#endif/' \
	| tr @ '\012' \
	>>$@

# A few object files need extra definitions. The dependencies are below.
tex-file.o:
	$(LTCOMPILE) $(CPPFLAGS) -DDEFAULT_FONT_SIZES='"$(default_texsizes)"' \
	  $(CFLAGS) -c $(srcdir)/tex-file.c

distclean-local:
	rm -f paths.h texmf.cnf texmf.sed
	rm -f kpathsea.cps kpathsea.fn kpathsea.vr kpathsea.tp \
		kpathsea.ky kpathsea.pg kpathsea.cm kpathsea.fl \
		kpathsea.op kpathsea.toc kpathsea.log kpathsea.aux \
		kpathsea.cp mode2dpi.log kpathsea.info-1 \
		kpathsea.info-2 kpathsea.info kpathsea.info-3


$(srcdir)/tex-file.c: paths.h