# rdepend.make -- rules for remaking the dependencies.
# Have to use -M, not -MM, since we use <kpathsea/...> instead of
# "kpathsea/..." in the sources.  But that means we have to remove the
# directory prefixes and all the system include files.
# And <kpathsea/paths.h> is generated, not part of the distribution.
depend depend.make:: c-auto.h $(top_srcdir)/../make/rdepend.make
	$(CC) -M $(CPPFLAGS) *.c \
	  | sed -e 's,\.\./kpathsea/,$$(kpathsea_srcdir)/,g' \
	        -e 's,$$(kpathsea_srcdir)/paths.h,paths.h,g' \
	        -e 's,/usr[^ ]* ,,g' \
	        -e 's,/usr[^ ]*$$,,g' \
	        -e 's,dvi2xx.o,dvilj.o dvilj2p.o dvilj4.o dvilj4l.o,' \
	  | grep -v '^ *\\$$' \
	  >depend.make
# End of rdepend.make.
