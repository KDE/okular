# rdepend.mk -- rules for remaking the dependencies.
@MAINT@# 
@MAINT@# Have to use -M, not -MM, since we use <kpathsea/...> instead of
@MAINT@# "kpathsea/..." in the sources.  But that means we have to remove the
@MAINT@# directory prefixes and all the system include files.
@MAINT@# And <kpathsea/paths.h> is generated, not part of the distribution.
@MAINT@# 
@MAINT@# And, there's no need for any installer/user to ever run this, it can
@MAINT@# only cause trouble. So comment it out in the distribution.
@MAINT@# (It doesn't work when the source and build directories are different.)
@MAINT@ifndef c_auto_h_dir
@MAINT@c_auto_h_dir = .
@MAINT@endif
@MAINT@
@MAINT@depend depend.mk:: $(c_auto_h_dir)/c-auto.h \
@MAINT@  $(top_srcdir)/../make/rdepend.mk 
@MAINT@	$(CC) -M $(ALL_CPPFLAGS) -I$(c_auto_h_dir) *.c \
@MAINT@	  | sed -e 's,\(\.\./\)\+kpathsea/,$$(kpathsea_srcdir)/,g' \
@MAINT@	        -e 's,$$(kpathsea_srcdir)/c-auto.h,$$(kpathsea_dir)/c-auto.h,g' \
@MAINT@	        -e 's,$$(kpathsea_srcdir)/paths.h,$$(kpathsea_dir)/paths.h,g' \
@MAINT@	        -e 's,/usr[^ ]* ,,g' \
@MAINT@	        -e 's,/usr[^ ]*$$,,g' \
@MAINT@	        -e 's,dvi2xx.o,dvilj.o dvilj2p.o dvilj4.o dvilj4l.o,' \
@MAINT@	        -e 's,lex.yy,$$(LEX_OUTPUT_ROOT),g' \
@MAINT@	  | $(top_srcdir)/../unbackslsh.awk \
@MAINT@	  >depend.mk
@MAINT@# If kpathsea, we're making .lo library objects instead of .o's.
@MAINT@	pwd | grep -v kpathsea >/dev/null \
@MAINT@	  || (sed -e 's/\.o:/.lo:/' \
@MAINT@	          -e 's/kpsewhich.lo:/kpsewhich.o:/' \
@MAINT@	          -e 's/kpsestat.lo:/kpsestat.o:/' \
@MAINT@	          -e 's/access.lo:/access.o:/' \
@MAINT@	          -e 's/readlink.lo:/readlink.o:/' \
@MAINT@	      <depend.mk >depend-tmp.mk; mv depend-tmp.mk depend.mk)
@MAINT@.PHONY: depend

# Let's stick a rule for TAGS here, just in case someone wants them.
# (We don't put them in the distributions, to keep them smaller.)
TAGS: *.c *.h
	pwd | grep kpathsea >/dev/null && append=../kpathsea/TAGS; \
	  etags $$append *.[ch]

# Prevent GNU make 3.[59,63) from overflowing arg limit on system V.
.NOEXPORT:

# End of rdepend.mk.
